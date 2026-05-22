#ifndef XMIDI32_CRITICAL_H
#define XMIDI32_CRITICAL_H

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define XMIDI_ATOMIC uint32_t
#define XMIDI_AQUIRE() __asm__ __volatile__("" ::: "memory")
#elif defined(_MSC_VER)
#define XMIDI_ATOMIC volatile uint32_t
#define XMIDI_AQUIRE() _ReadWriteBarrier()
#else
#define XMIDI_ATOMIC volatile uint32_t
#define XMIDI_AQUIRE() do {} while (0)
#endif

static inline uint32_t xm32_atomic_cas(volatile uint32_t *addr,
                                       uint32_t expected, uint32_t desired) {
#if defined(__GNUC__) || defined(__clang__)
    uint32_t prev;
    __asm__ __volatile__(
        "lock; cmpxchg %3, %1"
        : "=a"(prev), "+m"(*addr)
        : "a"(expected), "r"(desired)
        : "memory", "cc");
    return prev;
#elif defined(_MSC_VER)
    return _InterlockedCompareExchange((volatile long *)addr,
                                         desired, expected);
#else
    uint32_t prev = *addr;
    if (prev == expected) *addr = desired;
    return prev;
#endif
}

static inline uint32_t xm32_atomic_add(volatile uint32_t *addr, uint32_t delta) {
#if defined(__GNUC__) || defined(__clang__)
    uint32_t result;
    __asm__ __volatile__(
        "lock; xadd %0, %1"
        : "=r"(result), "+m"(*addr)
        : "0"(delta)
        : "memory", "cc");
    return result;
#elif defined(_MSC_VER)
    return _InterlockedExchangeAdd((volatile long *)addr, delta) + delta;
#else
    uint32_t old;
    do { old = *addr; } while (xm32_atomic_cas(addr, old, old + delta) != old);
    return old;
#endif
}

static inline uint32_t xm32_atomic_xchg(volatile uint32_t *addr, uint32_t val) {
#if defined(__GNUC__) || defined(__clang__)
    uint32_t result;
    __asm__ __volatile__(
        "lock; xchg %0, %1"
        : "=r"(result), "+m"(*addr)
        : "0"(val)
        : "memory");
    return result;
#elif defined(_MSC_VER)
    return _InterlockedExchange((volatile long *)addr, val);
#else
    uint32_t old;
    do { old = *addr; } while (xm32_atomic_cas(addr, old, val) != old);
    return old;
#endif
}

static inline uint32_t xm32_try_enter(uint32_t volatile *service_active) {
    return xm32_atomic_cas(service_active, 0, 1);
}

static inline void xm32_leave(uint32_t volatile *service_active) {
    xm32_atomic_add(service_active, (uint32_t)-1);
}

static inline uint16_t xm32_atomic_add16(volatile uint16_t *addr, int16_t delta) {
#if defined(__GNUC__) || defined(__clang__)
    uint16_t result;
    __asm__ __volatile__(
        "lock; xadd %0, %1"
        : "=r"(result), "+m"(*addr)
        : "0"((uint16_t)delta)
        : "memory", "cc");
    return result;
#elif defined(_MSC_VER)
    return (uint16_t)_InterlockedExchangeAdd16((volatile short *)addr, delta) + delta;
#else
    uint16_t old;
    do { old = *addr; } while (xm32_atomic_cas((volatile uint32_t *)addr, old, old + delta) != old);
    return old;
#endif
}

static inline uint16_t xm32_atomic_inc16(volatile uint16_t *addr) {
    return xm32_atomic_add16(addr, 1);
}

static inline uint16_t xm32_atomic_dec16(volatile uint16_t *addr) {
    return xm32_atomic_add16(addr, -1);
}

static inline void xm32_atomic_store16(volatile uint16_t *addr, uint16_t val) {
#if defined(__GNUC__) || defined(__clang__)
    __asm__ __volatile__(
        "mov %0, %%ax\n\t"
        "lock; xchg %%ax, %1"
        : : "r"((uint16_t)val), "m"(*addr) : "ax", "memory");
#elif defined(_MSC_VER)
    _InterlockedExchange16((volatile short *)addr, (short)val);
#else
    xm32_atomic_cas((volatile uint32_t *)addr, 0xFFFF, val);
#endif
}

static inline uint32_t xm32_try_enter_callback(void) { return 1; }
static inline void xm32_leave_callback(void) {}

#endif
