#include "types.h"
#include "result.h"
#include "services/fatal.h"
#include "kernel/mutex.h"
#include "kernel/random.h"
#include "kernel/svc.h"
#include "kernel/virtmem.h"


#define RANDOM_MAX_ATTEMPTS 0x200

#define NX_INLINE __attribute__((always_inline)) static inline

typedef struct {
    uintptr_t start;
    uintptr_t end;
} MemRegion;

static Mutex g_VirtmemMutex;

static MemRegion g_AliasRegion;
static MemRegion g_HeapRegion;
static MemRegion g_AslrRegion;
static MemRegion g_StackRegion;

typedef struct {
    u64  start;
    u64  end;
} VirtualRegion;

struct VirtmemReservation {
    VirtmemReservation *next;
    VirtmemReservation *prev;
    MemRegion region;
};

enum {
    REGION_STACK=0,
    REGION_HEAP=1,
    REGION_LEGACY_ALIAS=2,
    REGION_MAX
};

static VirtualRegion g_AddressSpace;
static VirtualRegion g_Region[REGION_MAX];
static u64 g_CurrentAddr;
static u64 g_CurrentMapAddr;
static Mutex g_VirtMemMutex;

static VirtmemReservation *g_Reservations;

uintptr_t __attribute__((weak)) __libnx_virtmem_rng(void) {
    return (uintptr_t)randomGet64();
}

static Result _memregionInitWithInfo(MemRegion* r, InfoType id0_addr, InfoType id0_sz) {
    u64 base;
    Result rc = svcGetInfo(&base, id0_addr, CUR_PROCESS_HANDLE, 0);

    if (R_SUCCEEDED(rc)) {
        u64 size;
        rc = svcGetInfo(&size, id0_sz, CUR_PROCESS_HANDLE, 0);

        if (R_SUCCEEDED(rc)) {
            r->start = base;
            r->end   = base + size;
        }
    }

    return rc;
}

NX_INLINE bool _memregionOverlaps(MemRegion* r, uintptr_t start, uintptr_t end) {
    return start < r->end && r->start < end;
}

NX_INLINE bool _memregionIsMapped(uintptr_t start, uintptr_t end, uintptr_t guard, uintptr_t* out_end) {
    // Adjust start/end by the desired guard size.
    start -= guard;
    end += guard;

    // Query memory properties.
    MemoryInfo meminfo;
    u32 pageinfo;
    Result rc = svcQueryMemory(&meminfo, &pageinfo, start);
    if (R_FAILED(rc))
        return false;

    // Return true if there's anything mapped.
    uintptr_t memend = meminfo.addr + meminfo.size;
    if (meminfo.type != MemType_Unmapped || end > memend) {
        if (out_end) *out_end = memend + guard;
        return true;
    }

    return false;
}

NX_INLINE bool _memregionIsReserved(uintptr_t start, uintptr_t end, uintptr_t guard, uintptr_t* out_end) {
    // Adjust start/end by the desired guard size.
    start -= guard;
    end += guard;

    // Go through each reservation and check if any of them overlap the desired address range.
    for (VirtmemReservation *rv = g_Reservations; rv; rv = rv->next) {
        if (_memregionOverlaps(&rv->region, start, end)) {
            if (out_end) *out_end = rv->region.end + guard;
            return true;
        }
    }

    return false;
}

static void* _memregionFindRandom(MemRegion* r, size_t size, size_t guard_size) {
    // Page align the sizes.
    size = (size + 0xFFF) &~ 0xFFF;
    guard_size = (guard_size + 0xFFF) &~ 0xFFF;

    // Ensure the requested size isn't greater than the memory region itself...
    uintptr_t region_size = r->end - r->start;
    if (size > region_size)
        return NULL;

    // Main allocation loop.
    uintptr_t aslr_max_page_offset = (region_size - size) >> 12;
    for (unsigned i = 0; i < RANDOM_MAX_ATTEMPTS; i ++) {
        // Calculate a random memory range outside reserved areas.
        uintptr_t cur_addr;
        for (;;) {
            uintptr_t page_offset = __libnx_virtmem_rng() % (aslr_max_page_offset + 1);
            cur_addr = (uintptr_t)r->start + (page_offset << 12);

            // Avoid mapping within the alias region.
            if (_memregionOverlaps(&g_AliasRegion, cur_addr, cur_addr + size))
                continue;

            // Avoid mapping within the heap region.
            if (_memregionOverlaps(&g_HeapRegion, cur_addr, cur_addr + size))
                continue;

            // Found it.
            break;
        }

        // Check that there isn't anything mapped at the desired memory range.
        if (_memregionIsMapped(cur_addr, cur_addr + size, guard_size, NULL))
            continue;

        // Check that the desired memory range doesn't overlap any reservations.
        if (_memregionIsReserved(cur_addr, cur_addr + size, guard_size, NULL))
            continue;

        // We found a suitable address!
        return (void*)cur_addr;
    }

    return NULL;
}

static Result _GetRegionFromInfo(VirtualRegion* r, u64 id0_addr, u32 id0_sz) {
    u64 base;
    Result rc = svcGetInfo(&base, id0_addr, CUR_PROCESS_HANDLE, 0);

    if (R_SUCCEEDED(rc)) {
        u64 size;
        rc = svcGetInfo(&size, id0_sz, CUR_PROCESS_HANDLE, 0);

        if (R_SUCCEEDED(rc)) {
            r->start = base;
            r->end   = base + size;
        }
    }

    return rc;
}

static inline bool _InRegion(VirtualRegion* r, u64 addr) {
    return (addr >= r->start) && (addr < r->end);
}

void virtmemSetup(void) {
    if (R_FAILED(_memregionInitWithInfo(&g_AslrRegion, InfoType_AslrRegionAddress, InfoType_AslrRegionSize))) {
        // 1.0.0 doesn't expose address space size so we have to do this dirty hack to detect it.
        // Forgive me.

        Result rc = svcUnmapMemory((void*) 0xFFFFFFFFFFFFE000ULL, (void*) 0xFFFFFE000ull, 0x1000);

        if (rc == 0xD401) {
            // Invalid src-address error means that a valid 36-bit address was rejected.
            // Thus we are 32-bit.
            g_AddressSpace.start = 0x200000ull;
            g_AddressSpace.end   = 0x100000000ull;

            g_Region[REGION_STACK].start = 0x200000ull;
            g_Region[REGION_STACK].end = 0x40000000ull;
        }
        else if (rc == 0xDC01) {
            // Invalid dst-address error means our 36-bit src-address was valid.
            // Thus we are 36-bit.
            g_AddressSpace.start = 0x8000000ull;
            g_AddressSpace.end   = 0x1000000000ull;

            g_Region[REGION_STACK].start = 0x8000000ull;
            g_Region[REGION_STACK].end = 0x80000000ull;
        }
        else {
            // Wat.
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_WeirdKernel));
        }
    } else {
        if (R_FAILED(_GetRegionFromInfo(&g_Region[REGION_STACK], InfoType_StackRegionAddress, InfoType_StackRegionSize))) {
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGetInfo_Stack));
        }
    }

    if (R_FAILED(_GetRegionFromInfo(&g_Region[REGION_HEAP], InfoType_HeapRegionAddress, InfoType_HeapRegionSize))) {
        fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadGetInfo_Heap));
    }

    _GetRegionFromInfo(&g_Region[REGION_LEGACY_ALIAS], InfoType_AliasRegionAddress, InfoType_AliasRegionSize);
}

void virtmemLock(void) {
    mutexLock(&g_VirtmemMutex);
}

void virtmemUnlock(void) {
    mutexUnlock(&g_VirtmemMutex);
}

void* virtmemFindAslr(size_t size, size_t guard_size) {
    if (!mutexIsLockedByCurrentThread(&g_VirtmemMutex)) return NULL;
    _memregionInitWithInfo(&g_AslrRegion, InfoType_AslrRegionAddress, InfoType_AslrRegionSize);
    return _memregionFindRandom(&g_AslrRegion, size, guard_size);
}

void* virtmemReserve(size_t size) {
    Result rc;
    MemoryInfo meminfo;
    u32 pageinfo;
    size_t i;

    size = (size + 0xFFF) &~ 0xFFF;

    mutexLock(&g_VirtMemMutex);
    u64 addr = g_CurrentAddr;

    while (1)
    {
        // Add a guard page.
        addr += 0x1000;

        // If we go outside address space, let's go back to start.
        if (!_InRegion(&g_AddressSpace, addr)) {
            addr = g_AddressSpace.start;
        }
        // Query information about address.
        rc = svcQueryMemory(&meminfo, &pageinfo, addr);

        if (R_FAILED(rc)) {
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadQueryMemory));
        }

        if (meminfo.type != 0) {
            // Address is already taken, let's move past it.
            addr = meminfo.addr + meminfo.size;
            continue;
        }

        if (size > meminfo.size) {
            // We can't fit in this region, let's move past it.
            addr = meminfo.addr + meminfo.size;
            continue;
        }

        // Check if we end up in a reserved region.
        for(i=0; i<REGION_MAX; i++)
        {
            u64 end = addr + size - 1;

            if (_InRegion(&g_Region[i], addr) || _InRegion(&g_Region[i], end)) {
                break;
            }
        }

        // Did we?
        if (i != REGION_MAX) {
            addr = g_Region[i].end;
            continue;
        }

        // Not in a reserved region, we're good to go!
        break;
    }

    g_CurrentAddr = addr + size;

    mutexUnlock(&g_VirtMemMutex);
    return (void*) addr;
}

void  virtmemFree(void* addr, size_t size) {
    IGNORE_ARG(addr);
    IGNORE_ARG(size);
}

void* virtmemReserveStack(size_t size)
{
    Result rc;
    MemoryInfo meminfo;
    u32 pageinfo;

    size = (size + 0xFFF) &~ 0xFFF;

    mutexLock(&g_VirtMemMutex);
    u64 addr = g_CurrentMapAddr;

    while (1)
    {
        // Add a guard page.
        addr += 0x1000;

        // Make sure we stay inside the reserved map region.
        if (!_InRegion(&g_Region[REGION_STACK], addr)) {
            addr = g_Region[REGION_STACK].start;
        }

        // Query information about address.
        rc = svcQueryMemory(&meminfo, &pageinfo, addr);

        if (R_FAILED(rc)) {
            fatalSimple(MAKERESULT(Module_Libnx, LibnxError_BadQueryMemory));
        }

        if (meminfo.type != 0) {
            // Address is already taken, let's move past it.
            addr = meminfo.addr + meminfo.size;
            continue;
        }

        if (size > meminfo.size) {
            // We can't fit in this region, let's move past it.
            addr = meminfo.addr + meminfo.size;
            continue;
        }

        break;
    }

    g_CurrentMapAddr = addr + size;

    mutexUnlock(&g_VirtMemMutex);
    return (void*) addr;
}

void virtmemFreeStack(void* addr, size_t size) {
    IGNORE_ARG(addr);
    IGNORE_ARG(size);
}
