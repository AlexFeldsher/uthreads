#include <iostream>
#include <signal.h>
#include "uthreads.h"
#include "thread.h"
#include "scheduler.h"
#include "messages.h"
#include "blackbox.h"

/**
 * Instance of the scheduler
 */
static Scheduler* scheduler = Scheduler::instance();

/**
 * Initialized the library.
 * @param quantum_usecs the length of a quantum in microseconds
 * @return 0 if successful, otherwise -1
 */
int uthread_init(int quantum_usecs)
{
	// validate parameter
	if (quantum_usecs <= 0)
	{
		std::cerr << LIB_ERR_HEADER << LIB_ERR_QUANTUM;
		return -1;
	}

	scheduler->setQuantumLength(quantum_usecs);   // set the quantum time in the scheduler

	// add the main thread to the scheduler
	Thread* mainThread;
	try {
		 mainThread = new Thread(MAIN_THREAD_ID);
	} catch (std::bad_alloc& e) {
		std::cerr << SYS_ERR_HEADER << SYS_ERR_MEM_ALLOC;
		exit(1);
	}

	scheduler->add(mainThread);

	return 0;
}

/**
 * Creates a thread for the given function.
 * @param f the function the thread should wrap
 * @return the id of the thread if successful, otherwise -1
 */
int uthread_spawn(void (*f)(void))
{
	// ignore timer signal in critical code
	scheduler->blockTimerThreadSwitch();

	int tid = scheduler->id();
	if (tid == -1)
	{
		std::cerr << LIB_ERR_HEADER << LIB_ERR_MAX_THREAD;
		return -1;  // number of threads exceed the limit
	}

	Thread* thread;
	try {
		thread = new Thread(tid, f);
	} catch (std::bad_alloc& e) {
		std::cerr << SYS_ERR_HEADER << SYS_ERR_MEM_ALLOC;
		exit(1);
	}

	// save the thread environment
	address_t sp, pc;
	sp = (address_t)(thread->stack) + STACK_SIZE - sizeof(address_t);
	pc = (address_t)f;
	sigsetjmp(thread->env, 1);
	(thread->env->__jmpbuf)[JB_SP] = translate_address(sp);
	(thread->env->__jmpbuf)[JB_PC] = translate_address(pc);
	if (sigemptyset(&(thread->env->__saved_mask)) == -1)
	{
		std::cerr << SYS_ERR_HEADER << SYS_ERR_SIG_INIT;
		exit(1);
	}

	// add thread to scheduler
	scheduler->add(thread);

    // unblock timer signal
	scheduler->unblockTimerThreadSwitch();

	return tid;
}

/**
 * Terminates the requested thread.
 * @param tid the thread id to terminate
 * @return 0 if terminated successfully, otherwise -1
 */
int uthread_terminate(int tid)
{
	// ignore timer signal in critical code
	scheduler->blockTimerThreadSwitch();

	int retVal = scheduler->terminate(tid);
	if (retVal == -1)
		std::cerr << LIB_ERR_HEADER << LIB_ERR_TERMINATE;

	scheduler->unblockTimerThreadSwitch();
	return retVal;
}

/**
 * Blocks the thread with the given id number
 * @param tid thread id to block
 * @return 0 if successful, otherwise -1
 */
int uthread_block(int tid)
{
	scheduler->blockTimerThreadSwitch();

	int retVal = scheduler->block(tid);
	if (retVal == -1)
		std::cerr << LIB_ERR_HEADER << LIB_ERR_BLOCK;

	scheduler->unblockTimerThreadSwitch();
	return retVal;
}

/**
 * Resumes the requested blocked thread and moves it to ready state
 * @param tid the thread to resume
 * @return 0 if successful, otherwise -1
 */
int uthread_resume(int tid)
{
	scheduler->blockTimerThreadSwitch();

	int retVal = scheduler->resume(tid);
	if (retVal == -1)
		std::cerr << LIB_ERR_HEADER << LIB_ERR_RESUME;

	scheduler->unblockTimerThreadSwitch();
	return retVal;
}

/**
 * Blocks the running thread until the thread with the given ID moved to running state
 * @param tid the thread to sync with the running thread
 * @return 0 if successful, otherwise -1
 */
int uthread_sync(int tid)
{
	scheduler->blockTimerThreadSwitch();

	int retVal = scheduler->sync(tid);
	if (retVal == -1)
		std::cerr << LIB_ERR_HEADER << LIB_ERR_SYNC;

	scheduler->unblockTimerThreadSwitch();
	return retVal;
}

/**
 * Returns the thread id of the calling thread
 * @return thread id number
 */
int uthread_get_tid()
{
	return scheduler->running->id;
}

/**
 * Returns the total number of quantums performed
 * @return the total number of quantums
 */
int uthread_get_total_quantums()
{
	return scheduler->totalQuantums;
}

/**
 * Returns the number of quantum the requested thread performed
 * @param tid thread id number
 * @return the number of quantums the requested thread performed, -1 if thread doesn't exist
 */
int uthread_get_quantums(int tid)
{
	return scheduler->quantums(tid);
}
