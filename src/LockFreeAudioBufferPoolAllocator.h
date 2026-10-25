//=============================================================================
// LockFreeAudioBufferPoolAllocator.h
//
// Master Studio Pro Suite v30.0 - Apex Colossus Sovereign Diamond Jubilee Release
// Real-Time Lock-Free 64-Byte Cache-Aligned Slab Memory Pool Allocator.
//
// Features:
// - Zero OS Heap Allocation during real-time audio playback & processing
// - 64-byte CPU cache-line aligned slab buffers (alignas(64))
// - Lock-free, wait-free O(1) atomic index stack / free-list
// - Thread-safe SPSC/MPMC recycling without mutex locks or system calls
// - Allocation starvation telemetry, peak usage tracking, and buffer health monitor
// - RAII ScopedAudioBuffer helper for automatic scope-based buffer recycling
//=============================================================================

#pragma once

#include <vector>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

namespace Jazz {

class JZLockFreeAudioBufferPoolAllocator {
public:
    explicit JZLockFreeAudioBufferPoolAllocator(size_t slabCount = 64, size_t samplesPerSlab = 2048);
    ~JZLockFreeAudioBufferPoolAllocator();

    // Pool Lifecycle
    void init(size_t slabCount, size_t samplesPerSlab);
    void shutdown();

    // Real-Time Lock-Free Allocation (Audio Thread Safe)
    float* acquireBuffer(size_t minSamples = 0);
    bool releaseBuffer(float* buffer);

    // Telemetry & Diagnostics
    size_t getTotalCapacity() const { return mSlabCount; }
    size_t getSamplesPerSlab() const { return mSamplesPerSlab; }
    size_t getAvailableCount() const;
    size_t getPeakAllocatedCount() const { return mPeakAllocated.load(std::memory_order_relaxed); }
    uint64_t getStarvationCount() const { return mStarvationCount.load(std::memory_order_relaxed); }
    uint64_t getTotalAllocations() const { return mTotalAllocations.load(std::memory_order_relaxed); }

    // Validation
    static bool isCacheAligned(const void* ptr, size_t alignment = 64) {
        return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
    }

    // RAII Smart Buffer Handle
    class ScopedAudioBuffer {
    public:
        ScopedAudioBuffer(JZLockFreeAudioBufferPoolAllocator& pool, size_t minSamples = 0)
            : mPool(pool)
            , mPtr(pool.acquireBuffer(minSamples))
        {}

        ~ScopedAudioBuffer() {
            if (mPtr) {
                mPool.releaseBuffer(mPtr);
            }
        }

        // Non-copyable, movable
        ScopedAudioBuffer(const ScopedAudioBuffer&) = delete;
        ScopedAudioBuffer& operator=(const ScopedAudioBuffer&) = delete;

        ScopedAudioBuffer(ScopedAudioBuffer&& other) noexcept
            : mPool(other.mPool)
            , mPtr(other.mPtr)
        {
            other.mPtr = nullptr;
        }

        float* get() const { return mPtr; }
        float* data() const { return mPtr; }
        bool isValid() const { return mPtr != nullptr; }
        explicit operator bool() const { return mPtr != nullptr; }

    private:
        JZLockFreeAudioBufferPoolAllocator& mPool;
        float* mPtr;
    };

private:
    size_t mSlabCount;
    size_t mSamplesPerSlab;
    size_t mBytesPerSlab;

    // Cache-aligned raw slab storage
    uint8_t* mRawMemory;
    size_t mRawMemoryAllocatedSize;
    std::vector<float*> mSlabPointers;

    // Lock-free stack of free buffer slab indices
    // Using atomic head index and free-list index array
    std::unique_ptr<std::atomic<int32_t>[]> mFreeStack;
    std::atomic<int32_t> mStackHead;

    // Telemetry
    std::atomic<size_t> mCurrentlyAllocated;
    std::atomic<size_t> mPeakAllocated;
    std::atomic<uint64_t> mStarvationCount;
    std::atomic<uint64_t> mTotalAllocations;

    void cleanup();
};

} // namespace Jazz
