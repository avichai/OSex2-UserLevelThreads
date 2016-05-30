#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H



// ------------------------------ includes ------------------------------
#include <setjmp.h>
#include <signal.h>
#include <iostream>


// -------------------------- const definitions -------------------------
#define SAVE_MASK 1
#define JB_SP 6
#define JB_PC 7
#define STACK_SIZE 4096 /* stack size per thread (in bytes) */


// --------------------------- typedefs ---------------------------------
typedef unsigned long address_t;


// -------------------------- enums and classes -------------------------
/*
 * The possible states of a thread.
 */
enum State {READY=0, SLEEPING, RUNNING, BLOCKED};


/*
 * A class which represents a user-level thread.
 */
class Thread
{
public:
    /**
     * The Constructor, creates a user-level thread.
     * If f is NULL, then the program counter is set to default (by sigsetjmp).
     * @param f the entry point of the thread.
     * @param tid the thread ID.
     */
    Thread(void (*f)(void), unsigned int tid);

    /*
     * Getters and setters of the thread.
     */
    inline unsigned int getTotalRunningTime() { return _totalRunningTime; }
    inline void incTotalRunningTime() { ++_totalRunningTime; }
    inline unsigned int getTid() { return _tid; }
    inline unsigned int getAwakeTime() { return _awakeTime; }
    inline void setAwakeTime(unsigned int awakeTime) { _awakeTime = awakeTime; }
    inline State getState() { return _state; }
    inline void setState(State state) { _state = state; }
    inline sigjmp_buf* getEnvP() { return &_env; }


private:
    unsigned int _tid;               /* the ID of the thread. */
    State _state;                    /* the state of the thread. */
    unsigned int _awakeTime;         /* in quantum units. */
    unsigned int _totalRunningTime;  /* in quantum units. */
    sigjmp_buf _env;                 /* the environment of the thread. */
    char _stack[STACK_SIZE];         /* the stack of the thread. */
};



#endif //THREADS_THREAD_H
