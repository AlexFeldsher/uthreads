#include <iostream>
#include <stdlib.h> // for exit()
#include <sys/time.h>
#include <signal.h>
#include "scheduler.h"
#include "messages.h"

/**
 * The signal number used when the scheduler calls the switch thread function
 */
#define SCHED_SWITCH_SIG 0


//------------------------------------- Function declarations --------------------------------------------


/**
* Switch to the next ready thread in the scheduler ready list.
* @param sig signal id
*/
static void switchThread(int sig);

/**
 * Returns the next thread in the scheduler ready list
 */
static Thread* nextThread();


//------------------------------------------ Constructor -------------------------------------------------


/**
 * Constructor
 * Initializes arrays
 */
Scheduler::Scheduler()
{
	// initialize arrays
	int i, j;
	for (i = 0; i < MAX_THREAD_NUM; ++i)
	{
		for (j = 0; j < MAX_THREAD_NUM; ++j)
			syncMatrix[i][j] = 0;
		threadArray[i] = nullptr;
		numSyncedThreads[i] = 0;
	}
}


//---------------------------------------- Public Methods -------------------------------------------------


/**
 * Returns an instance of the scheduler object
 * @return instance of the scheduler
 */
Scheduler* Scheduler::instance()
{
	static Scheduler instance;
	return &instance;
}

/**
 * Set the length of a quantum in microseconds
 * @param quantum_usecs the length of a quantum in microseconds
 */
void Scheduler::setQuantumLength(int quantum_usecs)
{
	this->quantum_usecs = quantum_usecs;
}

/**
 * Add a thread to the scheduler thread list
 * Takes ownership of the thread
 * Assumes a valid thread is added
 * @param thread the thread to add to the scheduler thread list
 */
void Scheduler::add(Thread* thread)
{
	threadArray[thread->id] = thread;
	readyList.push_back(thread);

	if (thread->id == MAIN_THREAD_ID)
	{
		thread->state = RUNNING;
		running = thread;
		initializeTimer();
		switchThread(SCHED_SWITCH_SIG);
	}
}

/**
 * Returns a free thread ID
 * @return thread id number, if no available id's returns -1
 */
int Scheduler::id() const
{
	int i;
	// iterate over the thread array and find the first empty cell
	for (i = 0; i < MAX_THREAD_NUM; ++i)
		if (threadArray[i] == nullptr)
			return i;

	return -1;  // Thread pool is full
}

/**
 * Returns the number of quantum of the requested thread
 * @param tid thread id number
 * @return the number of quantums the thread made, -1 if tid doesn't exist
 */
int Scheduler::quantums(int tid) const
{
	if (tid < 0 || tid >= MAX_THREAD_NUM || threadArray[tid] == nullptr)
		return -1;

	return threadArray[tid]->nQuantum;
}

/**
 * Terminates the requested thread
 * @param tid the thread id to terminate
 * @return 0 if successful, otherwise -1
 * If a thread terminates itself or the main thread is terminated the function doesn't return
 */
int Scheduler::terminate(int tid)
{
	if (tid < 0 || tid >= MAX_THREAD_NUM || threadArray[tid] == nullptr)
		return -1;  // thread doesn't exist

	// if main thread is terminated free all memory and exit
	if (tid == MAIN_THREAD_ID)
		end();  // free memory and exit

	Thread* thread = threadArray[tid];

	if (running->id == tid)
		running = nullptr;

	// remove from thread array
	threadArray[tid] = nullptr;

	// remove blocks on synced threads
	unsync(tid);

	// remove from ready list
	removeFromReadyList(tid);

	// free allocated memory
	delete thread;

	// if the running thread was terminated then switch threads
	if (running == nullptr) {
		unblockTimerThreadSwitch(); // unblock timer signal
		switchThread(SCHED_SWITCH_SIG);
	}

	return 0;
}

/**
 * Blocks the requested thread
 * If the blocked thread is running then the thread is switched
 * @param tid the thread id to block
 * @return 0 if successful, otherwise -1
 */
int Scheduler::block(int tid)
{
	// thread doesn't exist or trying to block the main thread
	if (tid < 0 || tid >= MAX_THREAD_NUM || tid == MAIN_THREAD_ID || threadArray[tid] == nullptr)
		return -1;

	if (threadArray[tid]->state == BLOCKED)
		return 0;

	threadArray[tid]->state = BLOCKED;

	//remove from ready list
	removeFromReadyList(tid);

	// switch thread if the running thread was blocked
	if (threadArray[tid] == running)
	{
		unblockTimerThreadSwitch();
		switchThread(SCHED_SWITCH_SIG);
	}

	return 0;
}

/**
 * Moves a blocked thread back to ready list
 * @param tid the thread to move to ready list
 * @return 0 if successful, otherwise -1
 */
int Scheduler::resume(int tid)
{
	if (tid < 0 || tid >= MAX_THREAD_NUM || threadArray[tid] == nullptr)
		return -1;  // requested thread doesn't exist


	Thread* thread = threadArray[tid];
	thread->state = READY;        // change thread state to READY

	// don't put back in ready list if the thread is synced
	if (numSyncedThreads[tid] != 0)
		return 0;

	// make sure thread isn't in the ready list before adding it back
	auto b = readyList.begin();
	for (; b != readyList.end() && (*b)->id != tid; ++b)
		;

	if (b == readyList.end())
		readyList.push_back(threadArray[tid]);  // add thread to ready list

	return 0;
}

/**
 * Blocks the running thread until the requested thread state changes to RUNNING
 * @param tid thread id number
 * @return 0 if successful, otherwise -1
 */
int Scheduler::sync(int tid)
{
	if (tid < 0 || tid >= MAX_THREAD_NUM || threadArray[tid] == nullptr)
		return -1;  // thread doesn't exist
	if (running->id == MAIN_THREAD_ID)
		return -1;  // main thread can't sync

	// update the sync arrays
	if (syncMatrix[tid][running->id] != 1)  // don't count the same thread twice in sync arrays
	{
		syncMatrix[tid][running->id] = 1;
		numSyncedThreads[running->id]++;
	}

	unblockTimerThreadSwitch();
	switchThread(SCHED_SWITCH_SIG);

	return 0;
}

/**
 * Removes all blocks caused by a sync with the given thread
 */
void Scheduler::unsync(int tid)
{
	int i;
	for (i = 0; i < MAX_THREAD_NUM; ++i)
	{
		Thread* thread = threadArray[i];

		// remove sync blocks made by the given thread
		if (syncMatrix[tid][i] != 0)
			numSyncedThreads[i]--;
		syncMatrix[tid][i] = 0;

		// add non blocked threads back to ready list
		if (thread != nullptr && numSyncedThreads[i] == 0 && thread->state != BLOCKED && !inReadyList(i))
		{
			readyList.push_back(threadArray[i]);
		}

		// if thread was terminated other threads should not block this tid
		if (threadArray[tid] == nullptr)
		{
			syncMatrix[i][tid] = 0;
			numSyncedThreads[tid] = 0;
		}
	}
}

/**
 * Block the timer based thread switch
 * Ignores the SIGVTALRM signal
 */
void Scheduler::blockTimerThreadSwitch()
{
	if (signal(SIGVTALRM, SIG_IGN) == SIG_ERR)
	{
		std::cerr << SYS_ERR_HEADER << SYS_ERR_SIG_ACTION;
		exit(1);
	};
}

/**
 * Unblock the timer based thread switch
 * Restores the SIGVTALRM handler
 */
void Scheduler::unblockTimerThreadSwitch()
{
	if (signal(SIGVTALRM, switchThread) == SIG_ERR)
	{
		std::cerr << SYS_ERR_HEADER << SYS_ERR_SIG_ACTION;
		exit(1);
	};
}


//------------------------------------- Private Methods --------------------------------------------


/**
 * Initialize the quantum timer
 */
void Scheduler::initializeTimer()
{
	struct sigaction sa;
	struct itimerval timer;
	sa.sa_handler = &switchThread;

	// initialize variables
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGVTALRM, &sa, NULL) < 0) {
		std::cerr << SYS_ERR_HEADER << SYS_ERR_SIG_ACTION;
		exit(1);
	}

	// Configure the timer to expire after 1 sec... */
	timer.it_value.tv_sec = 0;        // first time interval, seconds part
	timer.it_value.tv_usec = quantum_usecs;        // first time interval, microseconds part

	// configure the timer to expire every 3 sec after that.
	timer.it_interval.tv_sec = 0;    // following time intervals, seconds part
	timer.it_interval.tv_usec = quantum_usecs;    // following time intervals, microseconds part

	// Start a virtual timer. It counts down whenever this process is executing.
	if (setitimer(ITIMER_VIRTUAL, &timer, NULL)) {
		std::cerr << SYS_ERR_HEADER << SYS_ERR_TIMER;
		exit(1);
	}
}

/**
 * Check if a thread is in the ready list
 * @param tid the thread to search in the ready list
 * @return true if in the ready list, otherwise false
 */
bool Scheduler::inReadyList(int tid) const
{
	auto b = readyList.begin();
	for (; b != readyList.end() && (*b)->id != tid; ++b)
		;
	if (b == readyList.end())
		return false;

	return true;
}

/**
 * Removes the requested thread from the ready list
 * @param tid the id of the thread to remove from the ready list
 */
 void Scheduler::removeFromReadyList(int tid)
{
	for (auto b = readyList.begin(); b != readyList.end(); ++b)
	{
		if ((*b)->id == tid)
		{
			readyList.erase(b);
			break;
		}
	}
}

/**
 * Free all the allocated memory and terminate the program with exit()
 * Called when main thread is terminated
 */
void Scheduler::end()
{
	// free all allocated memory
	unsigned int i;
	for (i = 1; i < MAX_THREAD_NUM; ++i)
	{
		if (threadArray[i] != nullptr)
			delete threadArray[i];
	}
	delete threadArray[MAIN_THREAD_ID];  // free main thread
	exit(0);
}


//------------------------------------- Static functions --------------------------------------------


/**
* Jump to the next ready thread in the scheduler ready list.
* @param sig signal id
*/
static void switchThread(int sig)
{
	static Scheduler* scheduler = Scheduler::instance();
	scheduler->blockTimerThreadSwitch();    // block timer signal in critical code

	Thread* running = scheduler->running;

	// if running thread wasn't terminated unsync threads
	if (running != nullptr)
		scheduler->unsync(running->id);

	// switch threads
	Thread* next = nextThread();
	Thread* prevThread = running;
	if (prevThread != nullptr && prevThread->state != BLOCKED)
		prevThread->state = READY;
	scheduler->running = next;
	scheduler->running->state = RUNNING;

	// update quantum counters
	scheduler->running->nQuantum++;
	scheduler->totalQuantums++;

	if (prevThread != nullptr && sigsetjmp(prevThread->env, 1))
		return;

	scheduler->unblockTimerThreadSwitch();  // unblock timer signal
	siglongjmp(scheduler->running->env, 1);
}

/**
 * Returns the next thread in the ready list
 * @return the next thread in the ready list, nullptr if list is empty
 */
static Thread* nextThread()
{
	static Scheduler* scheduler = Scheduler::instance();
	if (scheduler->readyList.empty())
		return nullptr;

	// return only a non numSyncedThreads thread
	Thread *next;
	do {
		next = scheduler->readyList.front();
		scheduler->readyList.pop_front();   // remove thread from ready list
	} while (next->state == BLOCKED);

	return next;
}
