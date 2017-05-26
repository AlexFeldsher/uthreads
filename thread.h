#ifndef UTHREADS_THREAD_H
#define UTHREADS_THREAD_H

#include "uthreads.h"   // for STACK_SIZE
#include <setjmp.h>


/**
 * Thread state enum
 */
enum State {READY, RUNNING, BLOCKED};

/**
 * Thread class
 */
struct Thread {

	/**
	 * The thread id (tid)
	 */
	const int id;

	/**
	 * Quantum counter
	 */
	unsigned int nQuantum = 0;

	/**
	 * The saved thread environment
	 */
	sigjmp_buf env;

	/**
	 * Thread stack
	 */
	char stack[STACK_SIZE] = {};

	/**
	 * The current state of the thread
	 */
	State state = READY;

	/**
	 * Thread constructor
	 * @param _id the thread id
	 * @param f the function the thread wraps
	 */
	Thread(int _id, void (*f)(void) = nullptr) : id(_id), func(f) {}

	/**
	 * Thread Destructor
	 */
	~Thread() {}

private:

	/**
	 * pointer to the function the thread wraps
	 */
	void (*func)(void);
};

#endif //UTHREADS_THREAD_H
