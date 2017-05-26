#ifndef UTHREADS_MESSAGES_H

#define UTHREADS_MESSAGES_H

/**
 * System error message header
 */
#define SYS_ERR_HEADER "system error: "

/**
 * Header for library error messages
 */
#define LIB_ERR_HEADER "thread library error: "

/**
 * Memory allocation error message
 */
#define SYS_ERR_MEM_ALLOC "failed memory allocation.\n"

/**
 * sigaction failure error message
 */
#define SYS_ERR_SIG_ACTION "failed to change a signal action.\n"

/**
 * setitimer failure error message
 */
#define SYS_ERR_TIMER "failed to set the value of the interval timer.\n"

/**
 * Failure to initialize signal set error message
 */
#define SYS_ERR_SIG_INIT "failed to initialize signal set.\n"

/**
 * Invalid quantum length error message
 */
#define LIB_ERR_QUANTUM "invalid quantum size.\n"

/**
 * Max number of threads exceeded error message
 */
#define LIB_ERR_MAX_THREAD "max thread number exceeded.\n"

/**
 * Failure to terminate thread error message
 */
#define LIB_ERR_TERMINATE "failed to terminate requested thread.\n"

/**
 * Failure to block thread error message
 */
#define LIB_ERR_BLOCK "failed to block requested thread.\n"

/**
 * Failure to resume thread error message
 */
#define LIB_ERR_RESUME "failed to resume requested thread.\n"

/**
 * Failure to sync thread error message
 */
#define LIB_ERR_SYNC "failed to sync requested thread.\n"

#endif //UTHREADS_MESSAGES_H
