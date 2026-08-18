//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/spinMutex.h"
#include "pxr/base/tf/spinRWMutex.h"
#include "pxr/base/tf/stringUtils.h"

#include <atomic>
#include <thread>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_CLI;

static auto MakeMsgFn(bool verbose) {
    return [verbose](char const *msg) {
        if (verbose) {
            printf("  %s\n", msg);
        }
    };
}

////////////////////////////////////////////////////////////////////////
// TfSpinMutex

static bool
Test_TfSpinMutex(int argc, char *argv[])
{
    bool verbose = false;
    CLI::App app;
    app.add_flag("-v,--verbose", verbose, "Print activity messages");
    CLI11_PARSE(app, argc, argv);

    auto msg = MakeMsgFn(verbose);

    // Uncontended acquire/release
    {
        TfSpinMutex m;
        m.Acquire();
        m.Release();
        msg("uncontended Acquire/Release: ok");
    }

    // TryAcquire succeeds when free, fails when held
    {
        TfSpinMutex m;
        TF_AXIOM(m.TryAcquire());
        // Lock is held; a second TryAcquire from another thread must fail.
        bool otherFailed = false;
        std::thread t([&] { otherFailed = !m.TryAcquire(); });
        t.join();
        TF_AXIOM(otherFailed);
        m.Release();
        // Lock is released; TryAcquire must succeed again.
        TF_AXIOM(m.TryAcquire());
        m.Release();
        msg("TryAcquire: ok");
    }

    // ScopedLock acquire/release lifecycle
    {
        TfSpinMutex m;
        {
            TfSpinMutex::ScopedLock lock(m);
            // Lock is held; another thread must not be able to acquire.
            bool otherFailed = false;
            std::thread t([&] { otherFailed = !m.TryAcquire(); });
            t.join();
            TF_AXIOM(otherFailed);
        }
        // ScopedLock destructor released; another thread must now succeed.
        bool otherSucceeded = false;
        std::thread t([&] {
            otherSucceeded = m.TryAcquire();
            if (otherSucceeded) {
                m.Release();
            }
        });
        t.join();
        TF_AXIOM(otherSucceeded);
        msg("ScopedLock acquire/release lifecycle: ok");
    }

    // ScopedLock::Release and re-Acquire
    {
        TfSpinMutex m;
        TfSpinMutex::ScopedLock lock(m);
        lock.Release();
        // Lock is released; another thread must succeed.
        bool otherSucceeded = false;
        std::thread t([&] {
            otherSucceeded = m.TryAcquire();
            if (otherSucceeded) {
                m.Release();
            }
        });
        t.join();
        TF_AXIOM(otherSucceeded);
        // Re-acquire and verify we hold it again.
        lock.Acquire();
        bool otherFailed = false;
        std::thread t2([&] { otherFailed = !m.TryAcquire(); });
        t2.join();
        TF_AXIOM(otherFailed);
        msg("ScopedLock Release/re-Acquire: ok");
    }

    // ScopedLock::TryAcquire
    {
        TfSpinMutex m;
        TfSpinMutex::ScopedLock lock;
        TF_AXIOM(lock.TryAcquire(m));
        // Lock is held; another thread must fail.
        bool otherFailed = false;
        std::thread t([&] { otherFailed = !m.TryAcquire(); });
        t.join();
        TF_AXIOM(otherFailed);
        lock.Release();
        // Lock is released; a new ScopedLock TryAcquire must succeed.
        TfSpinMutex::ScopedLock lock2;
        TF_AXIOM(lock2.TryAcquire(m));
        msg("ScopedLock TryAcquire: ok");
    }

    // ScopedLock move construction
    {
        TfSpinMutex m;
        TfSpinMutex::ScopedLock lock1(m);
        TfSpinMutex::ScopedLock lock2(std::move(lock1));
        // lock2 holds the lock; lock1 is disassociated.
        bool otherFailed = false;
        std::thread t([&] { otherFailed = !m.TryAcquire(); });
        t.join();
        TF_AXIOM(otherFailed);
        // Destroying lock1 (disassociated) must be a no-op.
        { TfSpinMutex::ScopedLock discarded(std::move(lock1)); }
        // lock2 still holds.
        otherFailed = false;
        std::thread t2([&] { otherFailed = !m.TryAcquire(); });
        t2.join();
        TF_AXIOM(otherFailed);
        lock2.Release();
        TF_AXIOM(m.TryAcquire());
        m.Release();
        msg("ScopedLock move construction: ok");
    }

    // ScopedLock move assignment
    {
        TfSpinMutex m;
        TfSpinMutex::ScopedLock lock1(m);
        TfSpinMutex::ScopedLock lock2;
        lock2 = std::move(lock1);
        // lock2 holds the lock; lock1 is disassociated.
        bool otherFailed = false;
        std::thread t([&] { otherFailed = !m.TryAcquire(); });
        t.join();
        TF_AXIOM(otherFailed);
        lock2.Release();
        TF_AXIOM(m.TryAcquire());
        m.Release();
        msg("ScopedLock move assignment: ok");
    }

    // ScopedLock self-move-assignment is a no-op
    {
        TfSpinMutex m;
        TfSpinMutex::ScopedLock lock(m);
        TfSpinMutex::ScopedLock &alias = lock;
        lock = std::move(alias);
        // Lock must still be held.
        bool otherFailed = false;
        std::thread t([&] { otherFailed = !m.TryAcquire(); });
        t.join();
        TF_AXIOM(otherFailed);
        msg("ScopedLock self-move-assignment: ok");
    }

    // Multi-threaded mutual exclusion
    //
    // Use a non-atomic read-modify-write on a shared counter under the lock.
    // If mutual exclusion is correct, the final count must be exact.
    {
        static constexpr int NumThreads = 8;
        static constexpr int NumIters  = 100'000;

        TfSpinMutex m;
        int counter = 0;

        std::vector<std::thread> threads;
        threads.reserve(NumThreads);
        for (int i = 0; i < NumThreads; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < NumIters; ++j) {
                    TfSpinMutex::ScopedLock lock(m);
                    int v = counter;
                    counter = v + 1;
                }
            });
        }
        for (auto &t : threads) {
            t.join();
        }
        TF_AXIOM(counter == NumThreads * NumIters);
        msg("multi-threaded mutual exclusion: ok");
    }

    return true;
}

TF_ADD_REGTEST(TfSpinMutex);


////////////////////////////////////////////////////////////////////////
// TfSpinRWMutex

static bool
Test_TfSpinRWMutex(int argc, char *argv[])
{
    bool verbose = false;
    CLI::App app;
    app.add_flag("-v,--verbose", verbose, "Print activity messages");
    CLI11_PARSE(app, argc, argv);

    auto msg = MakeMsgFn(verbose);
    
    // Uncontended read lock/unlock
    {
        TfSpinRWMutex m;
        m.AcquireRead();
        m.ReleaseRead();
        msg("uncontended AcquireRead/ReleaseRead: ok");
    }

    // Uncontended write lock/unlock
    {
        TfSpinRWMutex m;
        m.AcquireWrite();
        m.ReleaseWrite();
        msg("uncontended AcquireWrite/ReleaseWrite: ok");
    }

    // Multiple readers can hold simultaneously
    //
    // Each reader increments an atomic counter after acquiring, then waits for
    // all readers to be inside before releasing.  If all NumReaders reach
    // insideCount == NumReaders, they were all concurrently held.
    {
        static constexpr int NumReaders = 8;
        TfSpinRWMutex m;
        std::atomic<int> insideCount(0);
        std::atomic<bool> release(false);

        std::vector<std::thread> threads;
        threads.reserve(NumReaders);
        for (int i = 0; i < NumReaders; ++i) {
            threads.emplace_back([&] {
                m.AcquireRead();
                ++insideCount;
                while (!release.load(std::memory_order_relaxed)) {
                    std::this_thread::yield();
                }
                m.ReleaseRead();
            });
        }

        while (insideCount.load() < NumReaders) {
            std::this_thread::yield();
        }
        // All NumReaders are simultaneously inside the read lock.
        TF_AXIOM(insideCount.load() == NumReaders);
        release.store(true);
        for (auto &t : threads) {
            t.join();
        }
        msg("multiple readers hold simultaneously: ok");
    }

    // TryAcquireRead fails when a writer holds the lock
    {
        TfSpinRWMutex m;
        m.AcquireWrite();
        bool readerFailed = false;
        std::thread t([&] { readerFailed = !m.TryAcquireRead(); });
        t.join();
        TF_AXIOM(readerFailed);
        m.ReleaseWrite();
        msg("TryAcquireRead fails under writer: ok");
    }

    // TryAcquireWrite fails when another writer holds the lock
    {
        TfSpinRWMutex m;
        m.AcquireWrite();
        bool writerFailed = false;
        std::thread t([&] { writerFailed = !m.TryAcquireWrite(); });
        t.join();
        TF_AXIOM(writerFailed);
        m.ReleaseWrite();
        msg("TryAcquireWrite fails under another writer: ok");
    }

    // TryAcquireWrite stakes a write claim and waits for readers
    // TryAcquireWrite succeeds (returns true) when no other writer is active,
    // but blocks until any current readers have released.  We verify that it
    // does ultimately return true after the reader releases.
    {
        TfSpinRWMutex m;
        m.AcquireRead();

        std::atomic<bool> writerAcquired(false);
        std::thread writer([&] {
            // No other writer active, so TryAcquireWrite stakes the claim
            // and blocks until the read lock above is released.
            bool result = m.TryAcquireWrite();
            writerAcquired.store(result);
            if (result) {
                m.ReleaseWrite();
            }
        });

        // Yield briefly to give the writer time to enter TryAcquireWrite and
        // stake its claim before we release the read lock.
        for (int i = 0; i < 1000; ++i) {
            std::this_thread::yield();
        }

        m.ReleaseRead();
        writer.join();
        TF_AXIOM(writerAcquired.load());
        msg("TryAcquireWrite stakes claim and waits for readers: ok");
    }

    // TryAcquireWriteIfReleased succeeds when mutex is not acquired
    {
        TfSpinRWMutex m;
        TF_AXIOM(m.TryAcquireWriteIfReleased());
        m.ReleaseWrite();
        msg("TryAcquireWriteIfReleased succeeds when free: ok");
    }

    // TryAcquireWriteIfReleased fails when a reader holds the lock
    {
        TfSpinRWMutex m;
        m.AcquireRead();
        bool failed = false;
        std::thread t([&] { failed = !m.TryAcquireWriteIfReleased(); });
        t.join();
        TF_AXIOM(failed);
        m.ReleaseRead();
        msg("TryAcquireWriteIfReleased fails under reader: ok");
    }

    // TryAcquireWriteIfReleased fails when a writer holds the lock
    {
        TfSpinRWMutex m;
        m.AcquireWrite();
        bool failed = false;
        std::thread t([&] { failed = !m.TryAcquireWriteIfReleased(); });
        t.join();
        TF_AXIOM(failed);
        m.ReleaseWrite();
        msg("TryAcquireWriteIfReleased fails under writer: ok");
    }

    // TryAcquireWriteIfReleased never blocks.
    //
    // With a reader active, the call must return false immediately.  A failure
    // to return is surfaced by the test harness as a hang.
    {
        TfSpinRWMutex m;
        m.AcquireRead();
        bool returnedFalse = false;
        std::thread t([&] {
            returnedFalse = !m.TryAcquireWriteIfReleased();
        });
        t.join(); // Would hang here if the call incorrectly waited.
        TF_AXIOM(returnedFalse);
        m.ReleaseRead();
        msg("TryAcquireWriteIfReleased does not block: ok");
    }

    // UpgradeToWriter: atomic path (sole reader, no contending writer)
    {
        TfSpinRWMutex m;
        m.AcquireRead();
        bool wasAtomic = m.UpgradeToWriter();
        TF_AXIOM(wasAtomic);
        // We now hold the write lock; readers must be blocked.
        bool readerFailed = false;
        std::thread t([&] { readerFailed = !m.TryAcquireRead(); });
        t.join();
        TF_AXIOM(readerFailed);
        m.ReleaseWrite();
        msg("UpgradeToWriter atomic path: ok");
    }

    // DowngradeToReader: write -> read, readers can join, writers cannot
    {
        TfSpinRWMutex m;
        m.AcquireWrite();
        bool result = m.DowngradeToReader();
        TF_AXIOM(result); // This implementation always returns true.
        // We hold a read lock; another reader must be able to join.
        bool readerJoined = false;
        std::thread t([&] {
            readerJoined = m.TryAcquireRead();
            if (readerJoined) {
                m.ReleaseRead();
            }
        });
        t.join();
        TF_AXIOM(readerJoined);
        // A writer must not be able to immediately acquire; use IfReleased to
        // avoid the "stakes a claim and waits" behavior of TryAcquireWrite.
        bool writerFailed = false;
        std::thread t2([&] { writerFailed = !m.TryAcquireWriteIfReleased(); });
        t2.join();
        TF_AXIOM(writerFailed);
        m.ReleaseRead();
        msg("DowngradeToReader: ok");
    }

    // ScopedLock: read and write acquire/release lifecycle
    {
        TfSpinRWMutex m;
        {
            TfSpinRWMutex::ScopedLock lock(m, /*write=*/false);
            // Read lock held; another reader must be able to join.
            bool readerJoined = false;
            std::thread t([&] {
                readerJoined = m.TryAcquireRead();
                if (readerJoined) {
                    m.ReleaseRead();
                }
            });
            t.join();
            TF_AXIOM(readerJoined);
        }
        {
            TfSpinRWMutex::ScopedLock lock(m, /*write=*/true);
            // Write lock held; readers must be blocked.
            bool readerFailed = false;
            std::thread t([&] { readerFailed = !m.TryAcquireRead(); });
            t.join();
            TF_AXIOM(readerFailed);
        }
        // Both locks released; mutex must be fully free.
        TF_AXIOM(m.TryAcquireWriteIfReleased());
        m.ReleaseWrite();
        msg("ScopedLock read/write lifecycle: ok");
    }

    // ScopedLock: UpgradeToWriter and DowngradeToReader
    {
        TfSpinRWMutex m;
        TfSpinRWMutex::ScopedLock lock(m, /*write=*/false);
        bool wasAtomic = lock.UpgradeToWriter();
        TF_AXIOM(wasAtomic);
        // Write lock held; readers must be blocked.
        bool readerFailed = false;
        std::thread t([&] { readerFailed = !m.TryAcquireRead(); });
        t.join();
        TF_AXIOM(readerFailed);
        // Downgrade back to read.
        bool downgradeResult = lock.DowngradeToReader();
        TF_AXIOM(downgradeResult);
        // Read lock held; another reader must be able to join.
        bool readerJoined = false;
        std::thread t2([&] {
            readerJoined = m.TryAcquireRead();
            if (readerJoined) {
                m.ReleaseRead();
            }
        });
        t2.join();
        TF_AXIOM(readerJoined);
        msg("ScopedLock UpgradeToWriter/DowngradeToReader: ok");
    }

    // ScopedLock: TryAcquireWriteIfReleased.
    //
    // Use the deferred-acquisition constructor to get a mutex-associated but
    // not-yet-acquired lock, then call TryAcquireWriteIfReleased directly.
    {
        TfSpinRWMutex m;
        TfSpinRWMutex::ScopedLock lock(m, TfSpinRWMutex::deferAcquire);
        // Mutex is free; TryAcquireWriteIfReleased must succeed.
        TF_AXIOM(lock.TryAcquireWriteIfReleased());
        // Write lock held; another thread must fail.
        bool otherFailed = false;
        std::thread t([&] { otherFailed = !m.TryAcquireWriteIfReleased(); });
        t.join();
        TF_AXIOM(otherFailed);
        lock.Release();
        // Mutex is free again; TryAcquireWriteIfReleased must succeed once
        // more.
        TF_AXIOM(lock.TryAcquireWriteIfReleased());
        msg("ScopedLock TryAcquireWriteIfReleased: ok");
    }

    // ScopedLock move construction
    {
        TfSpinRWMutex m;
        TfSpinRWMutex::ScopedLock lock1(m, /*write=*/true);
        TfSpinRWMutex::ScopedLock lock2(std::move(lock1));
        // lock2 holds the write lock; lock1 is unassociated.
        bool readerFailed = false;
        std::thread t([&] { readerFailed = !m.TryAcquireRead(); });
        t.join();
        TF_AXIOM(readerFailed);
        lock2.Release();
        TF_AXIOM(m.TryAcquireWriteIfReleased());
        m.ReleaseWrite();
        msg("ScopedLock move construction: ok");
    }

    // ScopedLock move assignment
    {
        TfSpinRWMutex m;
        TfSpinRWMutex::ScopedLock lock1(m, /*write=*/true);
        TfSpinRWMutex::ScopedLock lock2;
        lock2 = std::move(lock1);
        // lock2 holds the write lock; lock1 is unassociated.
        bool readerFailed = false;
        std::thread t([&] { readerFailed = !m.TryAcquireRead(); });
        t.join();
        TF_AXIOM(readerFailed);
        lock2.Release();
        TF_AXIOM(m.TryAcquireWriteIfReleased());
        m.ReleaseWrite();
        msg("ScopedLock move assignment: ok");
    }

    // Multi-threaded: write lock provides mutual exclusion.
    //
    // Non-atomic read-modify-write under write lock; exact final count proves
    // no two writers were ever simultaneously inside.
    {
        static constexpr int NumWriters = 4;
        static constexpr int NumIters   = 50'000;

        TfSpinRWMutex m;
        int counter = 0;

        std::vector<std::thread> threads;
        threads.reserve(NumWriters);
        for (int i = 0; i < NumWriters; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < NumIters; ++j) {
                    TfSpinRWMutex::ScopedLock lock(m, /*write=*/true);
                    int v = counter;
                    counter = v + 1;
                }
            });
        }
        for (auto &t : threads) {
            t.join();
        }
        TF_AXIOM(counter == NumWriters * NumIters);
        msg("multi-threaded write exclusion: ok");
    }

    // Multi-threaded: mixed readers and writers, data integrity.
    //
    // Writers increment a counter by 2 under a write lock.  Readers verify the
    // counter is always even (i.e. never observed mid-update).  An exact final
    // count additionally verifies writer mutual exclusion.
    {
        static constexpr int NumWriters     = 2;
        static constexpr int NumReaders     = 6;
        static constexpr int NumWriterIters = 20'000;

        TfSpinRWMutex m;
        int counter = 0;
        std::atomic<bool> done(false);
        std::atomic<bool> readError(false);

        std::vector<std::thread> threads;
        threads.reserve(NumWriters + NumReaders);

        for (int i = 0; i < NumWriters; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < NumWriterIters; ++j) {
                    TfSpinRWMutex::ScopedLock lock(m, /*write=*/true);
                    int v = counter;
                    counter = v + 2;
                }
            });
        }

        for (int i = 0; i < NumReaders; ++i) {
            threads.emplace_back([&] {
                while (!done.load(std::memory_order_relaxed)) {
                    TfSpinRWMutex::ScopedLock lock(m, /*write=*/false);
                    if (counter % 2 != 0) {
                        readError.store(true);
                    }
                }
            });
        }

        for (int i = 0; i < NumWriters; ++i) {
            threads[i].join();
        }
        done.store(true);
        for (int i = NumWriters; i < NumWriters + NumReaders; ++i) {
            threads[i].join();
        }

        TF_AXIOM(!readError.load());
        TF_AXIOM(counter == NumWriters * NumWriterIters * 2);
        msg("multi-threaded mixed read/write integrity: ok");
    }

    return true;
}

TF_ADD_REGTEST(TfSpinRWMutex);
