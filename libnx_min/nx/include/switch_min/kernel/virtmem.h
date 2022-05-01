/**
 * @file virtmem.h
 * @brief Virtual memory mapping utilities
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../types.h"

/// Address space reservation type (see \ref virtmemAddReservation)
typedef struct VirtmemReservation VirtmemReservation;

void virtmemSetup(void);

/// Locks the virtual memory manager mutex.
void virtmemLock(void);

/// Unlocks the virtual memory manager mutex.
void virtmemUnlock(void);

/**
 * @brief Finds a random slice of free general purpose address space.
 * @param size Desired size of the slice (rounded up to page alignment).
 * @param guard_size Desired size of the unmapped guard areas surrounding the slice  (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 * @note The virtual memory manager mutex must be held during the find-and-map process (see \ref virtmemLock and \ref virtmemUnlock).
 */
void* virtmemFindAslr(size_t size, size_t guard_size);

/**
 * @brief Reserves a slice of general purpose address space.
 * @param size The size of the slice of address space that will be reserved (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 */
void* virtmemReserve(size_t size);

/**
 * @brief Relinquishes a slice of address space reserved with virtmemReserve (currently no-op).
 * @param addr Pointer to the slice.
 * @param size Size of the slice.
 */
void  virtmemFree(void* addr, size_t size);

/**
 * @brief Reserves a slice of address space inside the stack memory mapping region (for use with svcMapMemory).
 * @param size The size of the slice of address space that will be reserved (rounded up to page alignment).
 * @return Pointer to the slice of address space, or NULL on failure.
 */
void* virtmemReserveStack(size_t size);

/**
 * @brief Relinquishes a slice of address space reserved with virtmemReserveStack (currently no-op).
 * @param addr Pointer to the slice.
 * @param size Size of the slice.
 */
void  virtmemFreeStack(void* addr, size_t size);
