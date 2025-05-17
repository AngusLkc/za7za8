#include <stdint.h>

#define RWLOCK_WLOCK  0xFFFFFFFF
#define WAIT_SHIFT    32

typedef struct {
    uint64_t state; //高32位:wait;低32位:lock
} rwlock_t;

void cpu_pause() {
    #ifdef __x86_64__
        __builtin_ia32_pause();
    #else
        __asm__ __volatile__("yield");
    #endif
}

//初始化读写锁,如果lock所指内存手动置0可不调此函数
void rwlock_init(rwlock_t *lock)
{
    __sync_lock_release(&lock->state);
}

//申请写锁
void rwlock_wlock(rwlock_t *lock)
{
    __sync_fetch_and_add(&lock->state, 1ULL << WAIT_SHIFT);
    while (1) {
        uint64_t stval = __sync_fetch_and_add(&lock->state, 0);
        uint32_t wait = (uint32_t)(stval >> WAIT_SHIFT);
        if ((uint32_t)stval == 0) {
            uint64_t newval = ((uint64_t)(wait - 1) << WAIT_SHIFT) | RWLOCK_WLOCK;
            if (__sync_bool_compare_and_swap(&lock->state, stval, newval))
                return;
        } else {
            cpu_pause();
        }
    }
}

//申请读锁
void rwlock_rlock(rwlock_t *lock)
{
    while (1) {
        uint64_t stval = __sync_fetch_and_add(&lock->state, 0);
        uint32_t wait = (uint32_t)(stval >> WAIT_SHIFT);
        if (wait != 0 || (uint32_t)stval == RWLOCK_WLOCK) {
            cpu_pause();
            continue;
        }
        uint64_t newval = stval + 1;
        if (__sync_bool_compare_and_swap(&lock->state, stval, newval))
            return;
    }
}

//释放读锁或写锁
void rwlock_unlock(rwlock_t *lock)
{
    while (1) {
        uint64_t stval = __sync_fetch_and_add(&lock->state, 0);
        if ((uint32_t)stval == RWLOCK_WLOCK) {
            uint64_t newval = stval & (0xFFFFFFFFULL << WAIT_SHIFT);
            if (__sync_bool_compare_and_swap(&lock->state, stval, newval))
                return;
        } else {
            uint64_t newval = stval - 1;
            if (__sync_bool_compare_and_swap(&lock->state, stval, newval))
                return;
        }
    }
}

//写锁降级为读锁
void rwlock_degrade(rwlock_t *lock)
{
    while (1) {
        uint64_t stval = __sync_fetch_and_add(&lock->state, 0);
        uint32_t wait = (uint32_t)(stval >> WAIT_SHIFT);
        if ((uint32_t)stval != RWLOCK_WLOCK)
            return;
        uint64_t newval = ((uint64_t)wait << WAIT_SHIFT) | 1;
        if (__sync_bool_compare_and_swap(&lock->state, stval, newval))
            return;
    }
}
