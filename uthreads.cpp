// ------------------------------ includes ------------------------------
#include <signal.h>
#include <sys/time.h>
#include <iostream>
#include <unordered_map>
#include <list>
#include <queue>
#include <unistd.h>
#include "uthreads.h"
#include "Thread.h"


// -------------------------- const definitions -------------------------
#define SUCCESS 0
#define FAILURE -1
#define SYS_CALL_ERROR "system error: "
#define THREAD_LIB_ERROR "thread library error: "
#define QUANTUM_COUNTER_INIT_VALUE 0
#define FIRST_AVAILABLE_ID 1
#define MAIN_THREAD_ID 0
#define THREAD_NOT_SLEEPING 0
#define LONG_JUMP 1
#define IS_MEMBER 1
#define USEC_FACTOR 1000000
#define GET_SECS(x) x/USEC_FACTOR
#define GET_USECS(x) x%USEC_FACTOR

// ------------------------- structs and enums -------------------------
/*
 * A comparator which compares the thread IDs.
 */
typedef struct
{
    /*
     * Returns true iff tid1 > tid2, false otherwise.
     */
    bool operator() (unsigned int tid1, unsigned int tid2)
    {
        return tid1 > tid2;
    }
} TidComparator;

/*
 * A comparator which compares threads according to their awaking time.
 */
typedef struct
{
    /*
     * Returns true iff t1's awaking time > t2's awaking time, false otherwise.
     */
    bool operator() (Thread* t1, Thread* t2)
    {
        return t1->getAwakeTime() > t2->getAwakeTime();
    }
} SleepComparator;

/*
 * The possible actions in use in the sigprocaction function.
 */
enum SigprocAction {BLOCK=0, UNBLOCK=1};


// --------------------------- forward declaration-----------------------
static void threadsHandler(int);
static void resetTimer(unsigned int);
static unsigned int getNextAvailableTid();
static void setMask(SigprocAction);
static int returnAndUnblock(int);
static Thread* createThread(void (*)(void), unsigned int);
static void createMainThread();
static void freeMemoryAndExit(unsigned int);
static Thread* getThread(int tid, std::string funcName);
static void removePendingSIGVTALRM();
static void setSigsetToSIGVTARM(sigset_t*);
static void checkDeletedThreads();



// --------------------------- global variables -------------------------
/* The quantum counter of the threads scheduler */
static unsigned int quantumCounter;
/* The threads control structures:
 * allThreads: contains all the threads in the scheduler.
 * readYThreads: contains the threads in READY state.
 * sleepThreads: contains the threads in SLEEPING state.
 * idsMinHeap: minimum heap which handles the threads IDs.
 * */
static std::unordered_map<unsigned int, Thread*> allThreads;
static std::list<Thread*> readyThreads;
static std::priority_queue<Thread*, std::vector<Thread*>, SleepComparator>
        sleepThreads;
static std::priority_queue<unsigned int, std::vector
        <unsigned int>, TidComparator> idsMinHeap;
/* The current running thread. */
static Thread* runningThread;
/* The number of quantums for each interval. */
static unsigned int quantumInterval;
/* Used in case the running thread (not the main) terminates himself. */
Thread* threadToDelete;
Thread* threadWaitToDelete;


// ---------------------------- static functions -------------------------
/*
 * The SIGVTALRM signal handler, responsible for all the threads transitions
 * made in each quantum (might be invoked manually also).
 */
static void threadsHandler(int sig)
{
    ++quantumCounter;

    checkDeletedThreads();  // check if there's a thread that has to be deleted.

    // inserting all the threads that finished their sleeping time.
    while (sleepThreads.size() != 0 && sleepThreads.top()->getAwakeTime() ==
                                               quantumCounter)
    {
        Thread* threadP = sleepThreads.top();
        threadP->setState(READY);
        readyThreads.push_back(threadP);
        sleepThreads.pop();
    }

    // if the running thread was terminated we set the running thread NULL, so
    // we don't push it to the ready threads.
    if (runningThread != NULL)
    {
        runningThread->incTotalRunningTime();

        // only the main is running - nothing more to do.
        if (readyThreads.size() == 0)
        {
            return;
        }

        // put the running (not sleeping) thread in the end of the ready list.
        if (runningThread->getState() == RUNNING)
        {
            readyThreads.push_back(runningThread);
            runningThread->setState(READY);
        }

        // sets the env of the running thread or returning from siglongjmp.
        if ( sigsetjmp(*runningThread->getEnvP(), SAVE_MASK) == LONG_JUMP)
        {
            return;
        }
    }

    // switching the running thread to the first thread among the ready threads.
    runningThread = readyThreads.front();
    runningThread->setState(RUNNING);
    readyThreads.pop_front();

    // ignoring SIGVTALRM that might be pending.
    removePendingSIGVTALRM();

    // resetting the virtual timer and jumping to the next thread.
    resetTimer(quantumInterval);

    siglongjmp(*runningThread->getEnvP(), LONG_JUMP);
}


/*
 * Check if there is thread that has to be terminated.
 */
static void checkDeletedThreads()
{
    if (threadToDelete != NULL)
    {
        delete threadToDelete;
    }
    threadToDelete = threadWaitToDelete;
    threadWaitToDelete = NULL;
}


/*
 * Resets the virtual timer.
 */
static void resetTimer(unsigned int quantum_usecs)
{


    struct itimerval timer;

    // configure the timer to expire after quantum usecs.
    timer.it_value.tv_sec = GET_SECS(quantum_usecs);
    timer.it_value.tv_usec = GET_USECS(quantum_usecs);
    timer.it_interval.tv_sec = GET_SECS(quantum_usecs);
    timer.it_interval.tv_usec = GET_USECS(quantum_usecs);

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer (ITIMER_VIRTUAL, &timer, NULL))
    {
        std::cerr << SYS_CALL_ERROR << "setittimer failed." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }
}


/*
 * Returns the next available ID from the idsMinHeap.
 */
static unsigned int getNextAvailableTid()
{
    unsigned int nextId = idsMinHeap.top();
    idsMinHeap.pop();
    // keeping the min-heap non-empty.
    if (idsMinHeap.empty())
    {
        idsMinHeap.push(nextId + 1);
    }
    return nextId;
}


/*
 * Blocking or unblocking SIGVTALRM in each thread's state transition function
 * so the function won't be interrupted by the SIGVTALRM signal.
 */
static void setMask(SigprocAction action)
{
    sigset_t sset;
    setSigsetToSIGVTARM(&sset);

    int sigProcSuccess = 0;

    // blocking or unblocking the SIGVTALRM according to action variable.
    switch (action)
    {
        case (BLOCK):
            sigProcSuccess = sigprocmask(SIG_SETMASK, &sset, NULL);
            break;
        case (UNBLOCK):
            sigProcSuccess = sigprocmask(SIG_UNBLOCK, &sset, NULL);
            break;
    }

    if (sigProcSuccess != SUCCESS)
    {
        std::cerr << SYS_CALL_ERROR << "failed to sigprocmask." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }
}


/*
 * Unblocking the SIGVTALRM and returning returnStatus.
 */
static int returnAndUnblock(int returnStatus)
{
    setMask(UNBLOCK);
    return returnStatus;
}


/*
 * Creates a new thread according to the tid and puts it in the allthreads
 * unordered-map. Exiting if allocation failed.
 */
static Thread* createThread(void (*f)(void), unsigned int tid)
{
    Thread* threadP;
    try
    {
        threadP = new Thread(f, tid);
    }
    catch (std::bad_alloc& eba)
    {
        std::cerr << SYS_CALL_ERROR << "allocation failure." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }
    allThreads[threadP->getTid()] = threadP;
    return threadP;
}


/*
 * Creates the main thread.
 */
static void createMainThread()
{
    Thread* mainThreadP = createThread(NULL, MAIN_THREAD_ID);
    mainThreadP->setState(RUNNING);
    runningThread = mainThreadP;
}


/*
 * Releasing all the memory used in the program and exiting the program
 * according to exitStatus.
 */
static void freeMemoryAndExit(unsigned int exitStatus)
{
    // deleting all the threads except maybe the running thread which isn't the
    // main thread.
    for (auto it = allThreads.begin(); it != allThreads.end(); ++it)
    {
        Thread* threadP = it->second;
        if (threadP != runningThread || threadP->getTid() == MAIN_THREAD_ID)
        {
            delete it->second;
        }
    }

    // deleting the thread that might have been terminated a quantum before.
    if (threadToDelete != NULL)
    {
        delete threadToDelete;
    }

    exit(exitStatus);
}


/*
 * Returning the Thread with the given id.
 * If the thread doesn't exist returns NULL and prints an error message.
 */
static Thread* getThread(int tid, std::string funcName)
{
    auto it = allThreads.find(tid);
    // if the thread doesn't exist.
    if (it == allThreads.end())
    {
        std::cerr << THREAD_LIB_ERROR << funcName <<
                " - no thread with this id." << std::endl;
        return NULL;
    }
    return it->second;
}


/*
 * Calling this function checks if SIGVTALRM is pending, and if it's discard it.
 */
static void removePendingSIGVTALRM()
{
    sigset_t sset;
    if (sigemptyset(&sset) == FAILURE)
    {
        std::cerr << SYS_CALL_ERROR << "sigemptyset failure." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }

    if (sigpending(&sset) == FAILURE)
    {
        std::cerr << SYS_CALL_ERROR << "sigpending failure." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }

    int isMember = sigismember(&sset, SIGVTALRM);
    if (isMember == FAILURE)
    {
        std::cerr << SYS_CALL_ERROR << "sigismember failure." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }
    else if (isMember == IS_MEMBER)            // SIGVTALRM is pending.
    {
        int sig;
        setSigsetToSIGVTARM(&sset);
        if (sigwait(&sset, &sig) != SUCCESS)
        {
            std::cerr << SYS_CALL_ERROR << "sigwait failure." << std::endl;
            freeMemoryAndExit(EXIT_FAILURE);
        }
    }
}


/*
 * Set the sigset to contain only SIGVTALRM.
 */
static void setSigsetToSIGVTARM(sigset_t* ssetP)
{
    if (sigemptyset(ssetP) == FAILURE)
    {
        std::cerr << SYS_CALL_ERROR << "sigemptyset failure." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }

    if (sigaddset(ssetP, SIGVTALRM) == FAILURE)
    {
        std::cerr << SYS_CALL_ERROR << "sigaddset failure." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }
}


// ------------------------------ functions -----------------------------
/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs)
{
    // only positive quantum is allowed.
    if (quantum_usecs <= 0)
    {
        std::cerr << THREAD_LIB_ERROR <<
                "uthread_init - the quantum should be positive." << std::endl;
        return FAILURE;
    }

    quantumInterval = (unsigned int) quantum_usecs;
    quantumCounter = QUANTUM_COUNTER_INIT_VALUE;

    // initializing the global variables.
    threadToDelete = NULL;
    threadWaitToDelete = NULL;

    // initializing the id's min-heap.
    idsMinHeap.push(FIRST_AVAILABLE_ID);

    // create the main thread.
    createMainThread();

    // install threadsHandler as the signal handler for SIGVTALRM.
    struct sigaction sa;
    sa.sa_handler = &threadsHandler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGVTALRM, &sa, NULL) < SUCCESS)
    {
        std::cerr << SYS_CALL_ERROR << "sigaction failed." << std::endl;
        freeMemoryAndExit(EXIT_FAILURE);
    }

    // setting the virtual timer.
    resetTimer(quantumInterval);
    return SUCCESS;
}


/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void)) {
    setMask(BLOCK);

    // verify the pointer parameter f.
    if (f == NULL)
    {
        std::cerr << THREAD_LIB_ERROR <<
        "uthread_spawn - the thread entry point is NULL." << std::endl;
        return returnAndUnblock(FAILURE);
    }

    // can't create more then MAX_THREAD_NUM threads.
    if (allThreads.size() == MAX_THREAD_NUM)
    {
        std::cerr << THREAD_LIB_ERROR <<
               "uthread_spawn - number of threads exceeded limit." << std::endl;
        return returnAndUnblock(FAILURE);
    }

    Thread *threadP = createThread(f, getNextAvailableTid());
    readyThreads.push_back(threadP);

    return returnAndUnblock(threadP->getTid());
}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered as an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{
    setMask(BLOCK);

    // cant't terminate the main thread.
    if (tid == MAIN_THREAD_ID)
    {
        freeMemoryAndExit(EXIT_SUCCESS);
    }

    Thread* threadP;
    if ((threadP = getThread(tid, "uthread_terminate")) == NULL)
    {
        return returnAndUnblock(FAILURE);
    }

    State threadState = threadP->getState();

    // if the thread is sleeping.
    if (threadState == SLEEPING)
    {
        std::cerr << THREAD_LIB_ERROR <<
          "uthread_terminate - can't terminate a sleeping thread." << std::endl;
        return returnAndUnblock(FAILURE);
    }

    // else, removing the thread from all the relevant control structures.
    // Returning the thread id to the min heap.
    idsMinHeap.push((unsigned  int)tid);

    allThreads.erase(tid);

    if(threadState == READY)
    {
        readyThreads.remove(threadP);
    }

    if (threadState == RUNNING)
    {
        threadWaitToDelete = runningThread;
        runningThread = NULL;
        threadsHandler(SIGVTALRM);          // switching threads manually.
    }

    delete threadP;

    return returnAndUnblock(SUCCESS);
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED or SLEEPING states has no
 * effect and is not considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
    setMask(BLOCK);

    // can't to block the main thread.
    if ( tid == MAIN_THREAD_ID)
    {
        std::cerr << THREAD_LIB_ERROR <<
                "uthread_block - can't block the main thread." << std::endl;
        return returnAndUnblock(FAILURE);
    }

    Thread* threadP;
    if ((threadP = getThread(tid, "uthread_block")) == NULL)
    {
        return returnAndUnblock(FAILURE);
    }
    State threadState = threadP->getState();

    if (threadState == READY || threadState == RUNNING)
    {
        threadP->setState(BLOCKED);
        if (threadState == READY)
        {
            readyThreads.remove(threadP);
        }
        else            // running state.
        {
            setMask(UNBLOCK);
            threadsHandler(SIGVTALRM);      // switching threads manually.
        }
    }

        return returnAndUnblock(SUCCESS);
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in the RUNNING, READY or SLEEPING
 * state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{
    setMask(BLOCK);

    Thread* threadP;
    if ((threadP = getThread(tid, "uthread_resume")) == NULL)
    {
        return returnAndUnblock(FAILURE);
    }

    if (threadP->getState() == BLOCKED)
    {
        threadP->setState(READY);
        readyThreads.push_back(threadP);
    }

    return returnAndUnblock(SUCCESS);
}


/*
 * Description: This function puts the RUNNING thread to sleep for a period
 * of num_quantums (not including the current quantum) after which it is moved
 * to the READY state. num_quantums must be a positive number. It is an error
 * to try to put the main thread (tid==0) to sleep. Immediately after a thread
 * transitions to the SLEEPING state a scheduling decision should be made.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums)
{
    setMask(BLOCK);

    // only positive quantum is allowed.
    if (num_quantums <= 0)
    {
        std::cerr << THREAD_LIB_ERROR <<
                "uthread_sleep - number of quantums to sleep must be positive."
                << std::endl;
        return returnAndUnblock(FAILURE);
    }

    unsigned int runningThreadId = runningThread->getTid();
    // can't put the main thread to sleep.
    if ( runningThreadId == MAIN_THREAD_ID)
    {
        std::cerr << THREAD_LIB_ERROR <<
                "uthread_sleep - can't put to sleep the main thread."
                << std::endl;
        return returnAndUnblock(FAILURE);
    }

    // the thread with runningThreadId must exist since it's the running thread.
    Thread* threadP = allThreads.find(runningThreadId)->second;
    threadP->setState(SLEEPING);
    // including the current quantum.
    threadP->setAwakeTime(quantumCounter + num_quantums + 1);

    sleepThreads.push(threadP);

    setMask(UNBLOCK);
    threadsHandler(SIGVTALRM);              // switching threads manually.

    return SUCCESS;
}


/*
 * Description: This function returns the number of quantums until the thread
 * with id tid wakes up including the current quantum. If no thread with ID
 * tid exists it is considered as an error. If the thread is not sleeping,
 * the function should return 0.
 * Return value: Number of quantums (including current quantum) until wakeup.
*/
int uthread_get_time_until_wakeup(int tid)
{
    Thread* threadP;
    if ((threadP = getThread(tid, "uthread_get_time_until_wakeup")) == NULL)
    {
        return returnAndUnblock(FAILURE);
    }

    if (threadP->getState() != SLEEPING)
    {
        return THREAD_NOT_SLEEPING;
    }

    return threadP->getAwakeTime() - quantumCounter;
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid()
{
    return runningThread->getTid();
}


/*
 * Description: This function returns the total number of quantums that were
 * started since the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
    return quantumCounter + 1;      // including the current quantum.
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered as an error.
 * Return value: On success, return the number of quantums of the thread with ID
 * tid. On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
    setMask(BLOCK);

    if (allThreads.find(tid) == allThreads.end())
    {
        std::cerr << THREAD_LIB_ERROR <<
                "uthread_get_quantums - no thread with this id." << std::endl;
        return FAILURE;
    }

    unsigned int runningTime = allThreads[tid]->getTotalRunningTime();
    int ret = runningThread->getTid() ==
                      (unsigned int)tid ? runningTime + 1 : runningTime;
    return returnAndUnblock(ret);
}


