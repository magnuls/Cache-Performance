#ifndef THREADS_H
#define THREADS_H

#include <pthread/qos.h>

#include <algorithm>
#include <atomic>
#include <vector>

#include "types.h"

class SpinBarrier {
   public:
    explicit SpinBarrier(i32 n) : n_(n) {}

    void wait() {
        const i32 gen = generation_.load(std::memory_order_acquire);
        if (arrived_.fetch_add(1, std::memory_order_acq_rel) + 1 == n_) {
            arrived_.store(0, std::memory_order_relaxed);
            generation_.store(gen + 1, std::memory_order_release);
        } else {
            while (generation_.load(std::memory_order_acquire) == gen) {
            }
        }
    }

   private:
    const i32 n_;
    std::atomic<i32> arrived_{0};
    std::atomic<i32> generation_{0};
};

inline void pin_to_p_cores() {
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
}

inline std::vector<i32> thread_counts(i32 cpus_per_l2, i32 p_cores) {
    std::vector<i32> counts{1, 2, cpus_per_l2, p_cores};
    counts.erase(std::remove_if(counts.begin(), counts.end(),
                                [](i32 c) { return c <= 0; }),
                 counts.end());
    std::sort(counts.begin(), counts.end());
    counts.erase(std::unique(counts.begin(), counts.end()), counts.end());
    return counts;
}

#endif
