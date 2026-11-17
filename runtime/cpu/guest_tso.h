// SpeedBreaker runtime. GPL-3.0-or-later (see COPYING).
//
// SB_GUEST_TSO=ON (recomp/CMakeLists.txt; default for ProStreet): the recompiled
// code's guest memory accesses ordered as on x86 (total store order), not
// left to ARM64's weaker model. The games' lock-free handoffs between
// threads (Most Wanted's and Carbon's render queue: game/render_queue.cpp)
// have no barriers and rely on stores becoming visible in program order,
// which x86 and the 360 give and ARM64 doesn't. Aligned loads become
// load-acquires and aligned stores store-releases (ldapr/stlr: the
// instructions need natural alignment; an unaligned access stays plain).
// Force-included before ppc_context.h, whose macros are #ifndef'd.
#pragma once
#include <cstdint>

template <typename T>
inline T SbGuestLoad(const uint8_t* p)
{
    if ((reinterpret_cast<uintptr_t>(p) & (sizeof(T) - 1)) == 0)
        return __atomic_load_n(reinterpret_cast<const T*>(p), __ATOMIC_ACQUIRE);
    return *reinterpret_cast<const volatile T*>(p);
}

template <typename T>
inline void SbGuestStore(uint8_t* p, T v)
{
    if ((reinterpret_cast<uintptr_t>(p) & (sizeof(T) - 1)) == 0)
        __atomic_store_n(reinterpret_cast<T*>(p), v, __ATOMIC_RELEASE);
    else
        *reinterpret_cast<volatile T*>(p) = v;
}

#define PPC_LOAD_U8(x) SbGuestLoad<uint8_t>(base + (x))
#define PPC_LOAD_U16(x) __builtin_bswap16(SbGuestLoad<uint16_t>(base + (x)))
#define PPC_LOAD_U32(x) __builtin_bswap32(SbGuestLoad<uint32_t>(base + (x)))
#define PPC_LOAD_U64(x) __builtin_bswap64(SbGuestLoad<uint64_t>(base + (x)))
#define PPC_STORE_U8(x, y) SbGuestStore<uint8_t>(base + (x), uint8_t(y))
#define PPC_STORE_U16(x, y) SbGuestStore<uint16_t>(base + (x), __builtin_bswap16(uint16_t(y)))
#define PPC_STORE_U32(x, y) SbGuestStore<uint32_t>(base + (x), __builtin_bswap32(uint32_t(y)))
#define PPC_STORE_U64(x, y) SbGuestStore<uint64_t>(base + (x), __builtin_bswap64(uint64_t(y)))
