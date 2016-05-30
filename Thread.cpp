// ------------------------------ includes ------------------------------
#include "Thread.h"

// -------------------------- const definitions -------------------------
#define INITIAL_AWAKE_TIME 0
#define INITIAL_TOTAL_RUNNING_TIME 0


// ---------------------------- static functions -------------------------
/*
 * A translation is required when using an address of a variable.
 * Use this as a black box in your code.
 */
static address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
            "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}


// ---------------------------- class methods -----------------------------
/**
 * The Constructor, creates a user-level thread.
 * If f is NULL, then the program counter is set to default (by sigsetjmp).
 * @param f the entry point of the thread.
 * @param tid the thread ID.
 */
Thread::Thread(void (*f)(void), unsigned int tid)
{

    _tid = tid;
    _state = READY;
    _awakeTime = INITIAL_AWAKE_TIME;
    _totalRunningTime = INITIAL_TOTAL_RUNNING_TIME;
    sigsetjmp(_env, SAVE_MASK);

    /* Sets the PC and the SP of the thread, if the program counter (f) is
     * NULL, sets the PC to the default value stored by sigsetjmp. */
    if (f != NULL)
    {
        address_t sp = (address_t)_stack + STACK_SIZE - sizeof(address_t);
        _env->__jmpbuf[JB_SP] = translate_address(sp);

        address_t pc = (address_t)f;
        _env->__jmpbuf[JB_PC] = translate_address(pc);
    }
    sigemptyset(&_env->__saved_mask);
}
