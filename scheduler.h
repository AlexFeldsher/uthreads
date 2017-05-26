#ifndef UTHREADS_SCHEDULER_H
#define UTHREADS_SCHEDULER_H

#include <deque>
#include "uthreads.h"   // for MAX_THREAD_NUM
#include "thread.h"


/**
 * Default thread id for the main thread
 */
#define MAIN_THREAD_ID 0

/**
 * Singleton class.
 * Round Robin thread scheduler.
 */
struct Scheduler {

	/**
	 * Holds all the existing threads, cell index == tid
	 */
	Thread* threadArray[MAX_THREAD_NUM];

	/**
	 * Matrix of synced threads
	 * blocking[1][4] != 0 -> thread 1 blocks thread 4
	 */
	char syncMatrix[MAX_THREAD_NUM][MAX_THREAD_NUM];

	/**
	 * Holds the number of threads synced to a certain thread
	 * blocked[4] = number of threads synced with thread 4
	 * thread #4 can't run until all the synced thread ran
	 */
	int numSyncedThreads[MAX_THREAD_NUM];

	/**
	 * Queue of ready threads
	 */
	std::deque<Thread*> readyList;

	/**
	 * The current running thread
	 */
	Thread* running;

	/**
	 * Counter of the total number of quantums performed
	 */
	int totalQuantums = 0;

	/**
	 * Returns an instance of the scheduler object
	 * @return instance of the scheduler
	 */
	static Scheduler* instance();

	/**
	 * Set the length of a quantum in microseconds
	 * @param quantum_usecs the length of a quantum in microseconds
	 */
	void setQuantumLength(int quantum_usecs);

	/**
	 * Add a thread to the scheduler thread list
	 * Takes ownership of the thread
	 * Assumes a valid thread is added
	 * @param thread the thread to add to the scheduler thread list
	 * @return 0 if successful, otherwise -1
	 */
	void add(Thread* thread);

	/**
	 * Returns a free thread ID
	 * @return thread id number, if no available id's returns -1
	 */
	int id() const;

	/**
	 * Returns the number of quantum of the requested thread
	 * @param tid thread id number
	 * @return the number of quantums the thread made
	 */
	int quantums(int tid) const;

	/**
	 * Terminates the requested thread
	 * @param tid the thread id to terminate
	 * @return 0 if successful, otherwise -1
	 * If a thread terminates itself or the main thread is terminated the function doesn't return
	 */
	int terminate(int tid);

	/**
	 * Blocks the requested thread
	 * If the blocked thread is running then the thread is switched
	 * @param tid the thread id to block
	 * @return 0 if successful, otherwise -1
	 */
	int block(int tid);

	/**
	 * Moves a blocked thread back to ready list
	 * @param tid the thread to move to ready list
	 * @return 0 if successful, otherwise -1
	 */
	int resume(int tid);

	/**
	 * Blocks the running thread until the requested thread state changes to RUNNING
	 * @param tid thread id number
	 * @return 0 if successful, otherwise -1
	 */
	int sync(int tid);

	/**
	 * Removes all blocks caused by a sync with the given thread
	 */
	void unsync(int tid);

	/**
	 * Block the timer based thread switch
	 * Use before critical code
	 */
	void blockTimerThreadSwitch();

	/**
	 * Unblock the timer based thread switch
	 * Use after critical code is done
	 */
	void unblockTimerThreadSwitch();

private:

	/**
	 * The length of a quantum in microseconds
	 */
	int quantum_usecs = 0;

	/**
	 * Initialized the quantum timer
	 */
	void initializeTimer();

	/**
	 * Scheduler constructor
	 */
	Scheduler();

	/**
	 * Check if a thread is in the ready list
	 * @param tid the thread to search in the ready list
	 * @return true if in the ready list, otherwise false
	 */
	bool inReadyList(int tid) const;

	/**
	 * Remove the requested thread from the ready list
	 * @param tid the id of the thread to remove from the ready list
	 */
	void removeFromReadyList(int tid);

	/**
	 * Free all the allocated memory and terminate the program with exit()
	 */
	void end();

};

#endif //UTHREADS_SCHEDULER_H
