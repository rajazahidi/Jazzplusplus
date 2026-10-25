//=============================================================================
// LockFreeAudioBufferPoolAllocator.cpp
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// Real-Time Lock-Free 64-Byte Cache-Aligned Slab Memory Pool Allocator.
//=============================================================================

#include "LockFreeAudioBufferPoolAllocator.h"
#include <cstdint>
#include <algorithm>

namespace Jazz {

JZLockFreeAudioBufferPoolAllocator::JZLockFreeAudioBufferPoolAllocator(size_t slabCount, size_t samplesPerSlab)
    : mSlabCount(0)
    , mSamplesPerSlab(0)
    , mBytesPerSlab(0)
    , mRawMemory(nullptr)
    , mRawMemoryAllocatedSize(0)
    , mStackHead(0)
    , mCurrentlyAllocated(0)
    , mPeakAllocated(0)
    , mStarvationCount(0)
    , mTotalAllocations(0)
{
    init(slabCount, samplesPerSlab);
}

JZLockFreeAudioBufferPoolAllocator::~JZLockFreeAudioBufferPoolAllocator() {
    cleanup();
}

void JZLockFreeAudioBufferPoolAllocator::cleanup() {
    if (mRawMemory) {
        delete[] mRawMemory;
        mRawMemory = nullptr;
    }
    mSlabPointers.clear();
    mFreeStack.reset();
    mSlabCount = 0;
    mSamplesPerSlab = 0;
    mBytesPerSlab = 0;
    mStackHead.store(0, std::memory_order_relaxed);
    mCurrentlyAllocated.store(0, std::memory_order_relaxed);
}

void JZLockFreeAudioBufferPoolAllocator::shutdown() {
    cleanup();
}

void JZLockFreeAudioBufferPoolAllocator::init(size_t slabCount, size_t samplesPerSlab) {
    cleanup();

    mSlabCount = std::max<size_t>(4, slabCount);
    mSamplesPerSlab = std::max<size_t>(64, samplesPerSlab);

    // Each slab size aligned to 64 bytes
    size_t rawBytes = mSamplesPerSlab * sizeof(float);
    mBytesPerSlab = (rawBytes + 63) & ~size_t(63);

    // Allocate continuous memory block with extra 64 bytes for cache alignment
    mRawMemoryAllocatedSize = (mSlabCount * mBytesPerSlab) + 64;
    mRawMemory = new uint8_t[mRawMemoryAllocatedSize];
    std::memset(mRawMemory, 0, mRawMemoryAllocatedSize);

    // Find first 64-byte aligned boundary
    uintptr_t rawAddr = reinterpret_cast<uintptr_t>(mRawMemory);
    uintptr_t alignedStart = (rawAddr + 63) & ~uintptr_t(63);

    // Populate slab pointers
    mSlabPointers.resize(mSlabCount);
    for (size_t i = 0; i < mSlabCount; ++i) {
        uintptr_t slabAddr = alignedStart + (i * mBytesPerSlab);
        mSlabPointers[i] = reinterpret_cast<float*>(slabAddr);
    }

    // Allocate lock-free atomic index stack
    mFreeStack = std::make_unique<std::atomic<int32_t>[]>(mSlabCount);
    for (size_t i = 0; i < mSlabCount; ++i) {
        mFreeStack[i].store(static_cast<int32_t>(i), std::memory_order_relaxed);
    }

    mStackHead.store(static_cast<int32_t>(mSlabCount), std::memory_order_release);
    mCurrentlyAllocated.store(0, std::memory_order_relaxed);
    mPeakAllocated.store(0, std::memory_order_relaxed);
    mStarvationCount.store(0, std::memory_order_relaxed);
    mTotalAllocations.store(0, std::memory_order_relaxed);
}

float* JZLockFreeAudioBufferPoolAllocator::acquireBuffer(size_t minSamples) {
    if (minSamples > mSamplesPerSlab || mSlabCount == 0) {
        mStarvationCount.fetch_add(1, std::memory_order_relaxed);
        return nullptr;
    }

    int32_t currentHead = mStackHead.load(std::memory_order_relaxed);
    while (currentHead > 0) {
        if (mStackHead.compare_exchange_weak(currentHead, currentHead - 1,
                                             std::memory_order_acquire,
                                             std::memory_order_relaxed)) {
            int32_t slabIndex = mFreeStack[currentHead - 1].load(std::memory_order_relaxed);

            mTotalAllocations.fetch_add(1, std::memory_order_relaxed);
            size_t allocNow = mCurrentlyAllocated.fetch_add(1, std::memory_order_relaxed) + 1;

            // Update peak allocation
            size_t curPeak = mPeakAllocated.load(std::memory_order_relaxed);
            while (allocNow > curPeak && !mPeakAllocated.compare_exchange_weak(curPeak, allocNow, std::memory_order_relaxed)) {
                // spin to update peak
            }

            return mSlabPointers[slabIndex];
        }
    }

    // Stack is empty -> Pool starvation event
    mStarvationCount.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}

bool JZLockFreeAudioBufferPoolAllocator::releaseBuffer(float* buffer) {
    if (!buffer || mSlabCount == 0 || mSlabPointers.empty()) return false;

    // Check bounds
    uintptr_t bufAddr = reinterpret_cast<uintptr_t>(buffer);
    uintptr_t startAddr = reinterpret_cast<uintptr_t>(mSlabPointers.front());
    uintptr_t endAddr = startAddr + (mSlabCount * mBytesPerSlab);

    if (bufAddr < startAddr || bufAddr >= endAddr) {
        return false; // Pointer does not belong to this pool
    }

    size_t offset = bufAddr - startAddr;
    if ((offset % mBytesPerSlab) != 0) {
        return false; // Misaligned pointer
    }

    int32_t slabIndex = static_cast<int32_t>(offset / mBytesPerSlab);

    int32_t currentHead = mStackHead.load(std::memory_order_relaxed);
    while (currentHead < static_cast<int32_t>(mSlabCount)) {
        mFreeStack[currentHead].store(slabIndex, std::memory_order_relaxed);
        if (mStackHead.compare_exchange_weak(currentHead, currentHead + 1,
                                             std::memory_order_release,
                                             std::memory_order_relaxed)) {
            mCurrentlyAllocated.fetch_sub(1, std::memory_order_relaxed);
            return true;
        }
    }

    return false;
}

size_t JZLockFreeAudioBufferPoolAllocator::getAvailableCount() const {
    int32_t head = mStackHead.load(std::memory_order_relaxed);
    return (head > 0) ? static_cast<size_t>(head) : 0;
}

} // namespace Jazz
