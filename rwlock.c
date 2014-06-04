#include "lockless_utils.h"
#include <stdio.h>

#define RW_WAIT_BIT     0
#define RW_WRITE_BIT    1
#define RW_READ_BIT     2

#define RW_WAIT     1
#define RW_WRITE    2
#define RW_READ     4

typedef unsigned rwlock;

static void wrlock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    while (1)
    {
        unsigned state = *l;
    
        /* No readers or writers? */
        if (state < RW_WRITE)
        {
            /* Turn off RW_WAIT, and turn on RW_WRITE */
            if (cmpxchg(l, state, RW_WRITE) == state) return;
            
            /* Someone else got there... time to wait */
            state = *l;
        }
        
        /* Turn on writer wait bit */
        if (!(state & RW_WAIT)) atomic_set_bit(l, RW_WAIT_BIT);
    
        /* Wait until can try to take the lock */
        while (*l > RW_WAIT) cpu_relax();
    }
}

static void wrunlock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    atomic_add(l, -RW_WRITE);
}

static int wrtrylock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    unsigned state = *l;
    
    if ((state < RW_WRITE) && (cmpxchg(l, state, state + RW_WRITE) == state)) return 0;
    
    return EBUSY;
}

static void rdlock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    while (1)
    {
        /* A writer exists? */
        while (*l & (RW_WAIT | RW_WRITE)) cpu_relax();
        
        /* Try to get read lock */
        if (!(atomic_xadd(l, RW_READ) & (RW_WAIT | RW_WRITE))) return;
            
        /* Undo */
        atomic_add(l, -RW_READ);
    }
}

static void rdunlock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    atomic_add(l, -RW_READ);
}

static int rdtrylock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    /* Try to get read lock */
    unsigned state = atomic_xadd(l, RW_READ);
            
    if (!(state & (RW_WAIT | RW_WRITE))) return 0;
            
    /* Undo */
    atomic_add(l, -RW_READ);
        
    return EBUSY;
}

/* Get a read lock, even if a writer is waiting */
static int rdforcelock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    /* Try to get read lock */
    unsigned state = atomic_xadd(l, RW_READ);
    
    /* We succeed even if a writer is waiting */
    if (!(state & RW_WRITE)) return 0;
            
    /* Undo */
    atomic_add(l, -RW_READ);
        
    return EBUSY;
}

/* Try to upgrade from a read to a write lock atomically */
static int rdtryupgradelock(rwlock *l)
{
    printf("%s:%d %s()", __FILE__, __LINE__, __func__);
    /* Someone else is trying (and will succeed) to upgrade to a write lock? */
    if (atomic_bitsetandtest(l, RW_WRITE_BIT)) return EBUSY;
    
    /* Don't count myself any more */
    atomic_add(l, -RW_READ);
    
    /* Wait until there are no more readers */
    while (*l > (RW_WAIT | RW_WRITE)) cpu_relax();
    
    return 0;
}

int main() {
    rwlock x = 0;
    rdlock(&x);
    printf("lock value = %x\n", x);
    rdlock(&x);
    printf("lock value = %x\n", x);
    rdunlock(&x);
    printf("lock value = %x\n", x);
    rdunlock(&x);
    printf("lock value = %x\n", x);
    wrlock(&x);
    printf("lock value = %x\n", x);
    wrunlock(&x);
    printf("lock value = %x\n", x);
//    wrlock(&x);
//    printf("lock value = %x\n", x);
//    rdlock(&x);
//    printf("lock value = %x\n", x);
//    rdunlock(&x);
//    printf("lock value = %x\n", x);
//    wrunlock(&x);
//    printf("lock value = %x\n", x);
//    return 0;
}

