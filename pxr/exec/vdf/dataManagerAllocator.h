//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DATA_MANAGER_ALLOCATOR_H
#define PXR_EXEC_VDF_DATA_MANAGER_ALLOCATOR_H

#include "pxr/pxr.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/threadLimits.h"

#include <tbb/concurrent_queue.h>
#include <tbb/task_arena.h>

#include <algorithm>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNetwork;

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_DataManagerAllocator
///
/// \brief An allocator, which returns executor data containers for use by
/// executor data managers.
///
/// The allocator maintains an exponential moving average of the number of
/// allocated executor data instances in order to estimate future allocation
/// demand. Once the estimate falls below the number of available executor data
/// instances on the free list, calling Deallocate()  will release memory.
/// Note, that the estimated demand will taper off over time, in order to
/// increase chances of being able to satisfy recurring peak demands.
///
template < typename T >
class Vdf_DataManagerAllocator
{
public:
    /// Noncopyable.
    ///
    Vdf_DataManagerAllocator(const Vdf_DataManagerAllocator &) = delete;
    Vdf_DataManagerAllocator &operator=(
        const Vdf_DataManagerAllocator &) = delete;

    /// Constructor.
    ///
    Vdf_DataManagerAllocator();

    /// Destructor.
    ///
    ~Vdf_DataManagerAllocator();

    /// Allocate a new executor data instance.
    ///
    T *Allocate(const VdfNetwork &network);

    /// Deallocate an executor data instance. This may not immediatelly free
    /// the memory associated with the executor data instance.
    ///
    void DeallocateLater(T *data);

    /// Deallocate an executor data instance. This call immediatelly frees
    /// the memory associated with the executor data instance.
    ///
    void DeallocateNow(T *data);

    /// Clears all executor data instances on the allocators internal free list
    /// and therefore frees all the memory associated with these containers.
    ///
    void Clear();

private:

    // Updates the exponential moving average of allocations, to estimate
    // allocation demand.
    uint32_t _UpdateEstimated();

    // Returns whether or not we should delete the data or reuse it based on
    // the estimated demand and what we currently have available.
    bool _ShouldDeleteData(uint32_t numAllocated);

    // Drop data
    static void _ReleaseAndDropData(T *data);

    // Reuse data
    void _ReleaseAndReuseData(T *data);

    // Wait for all pending background deallocations.
    void _WaitForDeallocations();

    // The weight applied to new samples for the exponential moving average.
    static const float _emaWeight;

    // The exponential moving average of the number of allocations. This
    // is used for garbage collection.
    std::atomic<float> _emaAllocated;

    // The number of outstanding allocations.
    std::atomic<uint32_t> _numAllocated;

    // The number of allocations that will eventually be available on the
    // queue. Note, this value is not reflective of the size of the queue.
    // It contains allocations, which are still being processed by background
    // threads, and have not yet been pushed on the queue.
    std::atomic<uint32_t> _numPending;

    // The free list containing available executor data instances.
    typedef tbb::concurrent_queue<T *> _Queue;
    _Queue _available;

    // Task arena to isolate work performed by this allocator.
    tbb::task_arena _taskArena;

    // Work dispatcher to synchronize tasks created by this allocator.
    WorkDispatcher _workDispatcher;
};

// The weight applied to new samples of the exponential moving average. The
// weight controls how fast the number of allocations signal tapers off, and
// hence how quickly deallocated instances will be garbage collected.
// 
// A larger number will make the EMA taper of quicker, i.e. memory will be
// reclaimed much more eagerly.
//
// A smaller number will keep deallocated containers available for longer
// periods of time, increasing the chances of being able to fulfill sudden
// increases in demand for allocations (e.g. temporary executors).
//
// Must be a positive value between 0 and 1.
template < typename T >
const float Vdf_DataManagerAllocator<T>::_emaWeight = 0.01f;

///////////////////////////////////////////////////////////////////////////////

template < typename T >
Vdf_DataManagerAllocator<T>::Vdf_DataManagerAllocator() :
    _emaAllocated(),
    _numAllocated(),
    _numPending()
{
    // Initialize the task arena greedily. We expect this to be the first time
    // the arena is being initialized. Note, that in order to maintain TBBs
    // starvation resistance guarantees, we need to allow the arena to have at
    // least one worker.
    if (TF_VERIFY(!_taskArena.is_active())) {
        _taskArena.initialize(std::max<int>(2, WorkGetConcurrencyLimit()));
    }
}

template < typename T >
Vdf_DataManagerAllocator<T>::~Vdf_DataManagerAllocator()
{
    // Wait for all pending deallocations to complete.
    _WaitForDeallocations();

    // Free all the memory.
    Clear();
}

template < typename T >
T *
Vdf_DataManagerAllocator<T>::Allocate(const VdfNetwork &network)
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Vdf", "Vdf_DataManagerAllocator::Allocate");

    // Increment the number of allocations made
    ++_numAllocated;
    
    // Try to grab an existing executor data  instance from the free list,
    // or allocate a new one if required.
    T *data;
    if (!_available.try_pop(data)) {
        data = new T();
    }

    // Update the number of containers pending availability, if one was taken
    // from the free list. Update _numPending AFTER grabbing from the list to
    // avoid unsigned integer underflow.
    else {
        --_numPending;
    }

    // Make sure the container is appropriately sized.
    data->Resize(network);
   
    // Return the container.
    return data;
}

template < typename T >
void
Vdf_DataManagerAllocator<T>::DeallocateLater(T *data)
{
    if (!data) {
        return;
    }

    // Update the number of outstanding allocations.
    const uint32_t numAllocated = _numAllocated.fetch_sub(1);

    // Create a task, which releases the data in the background, and optionally
    // enqueues it on the free list for reuse.
    const bool shouldDelete = _ShouldDeleteData(numAllocated);

    // Enqueue the task. TBB's non-starvation guarantee will make sure that
    // there is at least one worker to execute the task, even if we are limited
    // to single-threaded mode.
    _taskArena.execute([this, data, shouldDelete] {
        _workDispatcher.Run([=]() {
            if (shouldDelete) {
                _ReleaseAndDropData(data);
            } else {
                _ReleaseAndReuseData(data);
            }
            return nullptr;
        });
    });

    // If it looks like we are deallocating the last allocation, let's not
    // release that data in the background. The reason why we do this is
    // because once we destruct the last executor (and therefore deallocate
    // the last executor data instance), there is a good chance that the
    // process will be exiting. It would be bad for a singleton allocator to
    // have background tasks running during exit, unless we can guarantee that
    // its destructor will be called.
    if (numAllocated == 1) {
        _WaitForDeallocations();
    }
}

template < typename T >
void
Vdf_DataManagerAllocator<T>::DeallocateNow(T *data)
{
    if (!data) {
        return;
    }

    // Update the number of outstanding allocations.
    const uint32_t numAllocated = _numAllocated.fetch_sub(1);

    if (_ShouldDeleteData(numAllocated)) {

        // Drop
        _ReleaseAndDropData(data);

    } else {

        // Reuse
        _ReleaseAndReuseData(data);

    }

}

template < typename T >
void
Vdf_DataManagerAllocator<T>::Clear()
{
    // Delete all executor data instances on the free list.
    T *data;
    while (_available.try_pop(data)) {
        delete data;
    }
}

template < typename T >
uint32_t
Vdf_DataManagerAllocator<T>::_UpdateEstimated()
{
    // Loop control variables.
    float currentEMA;
    float newEMA;

    // Do the CAS loop.
    do {

        // Do the atomic reads just once.
        const float sample = _numAllocated;
        currentEMA = _emaAllocated;

        // Apply the new exponential moving average. Note, we only compute the
        // EMA on the falling edges of the signal. This makes it so that the
        // number of allocations signal tapers off over time. Smoothing on the
        // rising edge is undesirable.
        newEMA = sample < currentEMA
            ? currentEMA * (1.0f - _emaWeight) + sample * _emaWeight
            : sample;

        // Compare and swap.
    } while (!_emaAllocated.compare_exchange_strong(currentEMA, newEMA));

    // Return the new value as an integer.
    return static_cast<uint32_t>(std::ceil(newEMA));
}

template < typename T >
bool 
Vdf_DataManagerAllocator<T>::_ShouldDeleteData(uint32_t numAllocated)
{
    // Update the exponential moving average of allocations to estimate the
    // future allocation demand.
    const uint32_t emaAllocated = _UpdateEstimated();

    // If we have enough pending containers to fulfill the estimated future
    // allocation demand, we will simply delete the data, instead of pushing
    // it on the free list.
    return (_numPending + numAllocated) > emaAllocated;
}

template < typename T >
void 
Vdf_DataManagerAllocator<T>::_ReleaseAndDropData(T *data)
{
    TRACE_FUNCTION();

    // If this data is not requested to be reused, we will simply free all
    // the memory associated with it.
    delete data;

}

template < typename T >
void 
Vdf_DataManagerAllocator<T>::_ReleaseAndReuseData(T *data)
{
    TRACE_FUNCTION();

    // We will clear the data, prep it for reuse and push it on the free list.
    // Note, this will not reclaim all the memory associated with the data.
    data->Clear();

    // Update _numPending BEFORE pushing onto the free list to avoid
    // unsigned integer underflow in the corresponding allocation code.
    ++_numPending;

    // Push the data on the free list.
    _available.push(data);
}

template < typename T >
void
Vdf_DataManagerAllocator<T>::_WaitForDeallocations()
{
    TRACE_FUNCTION();

    // One of the dispatched tasks may want to acquire the python GIL in
    // order to destruct python objects. If the calling thread is already
    // holding the lock this will result in a deadlock, unless we
    // temporarily release the GIL here.
    TF_PY_ALLOW_THREADS_IN_SCOPE();
    
    // Wait for all the executor data containers to be released.
    WorkDispatcher *dispatcher = &_workDispatcher;
    _taskArena.execute([dispatcher] {
        dispatcher->Wait();
    });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
