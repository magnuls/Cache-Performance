#ifndef CHASE_H
#define CHASE_H
// Pointer-chase primitives shared by the byte-sized sweeps.

#include <algorithm>
#include <chrono>

#include "measurement.h"
#include "node.h"
#include "types.h"

/*
 * fill_array -> identity-links (node->next = &node) the array in place
 */
inline void fill_array(Node* arr, i64 count) {
    for (i64 i{}; i < count; ++i) {
        arr[i] = Node();
        arr[i].next = &arr[i];
    }
}

/*
 * warm_loop -> walks the chain untimed once, so cold misses and
 * page faults occur off the clock
 */
inline void warm_loop(Node* arr, i64 count) {
    Node* temp = &arr[0];
    while (--count >= 0) {
        temp = temp->next;
    }
    asm volatile("" ::"r"(temp) : "memory");
}

/*
 * total_accesses -> number of accesses for a given array size
 */
inline i64 total_accesses(i64 arr_size) {
    i64 passes = 10;
    i64 k_min = 10000000, k_max = 50000000;
    return std::clamp(passes * arr_size, k_min, k_max);
}

/*
 * timed_access -> runs one timed chase, returns ns per access
 */
inline f64 timed_access(Node* arr, i64 num_accesses, Sweep s = Sweep::Size) {
    if (s == Sweep::Size || s == Sweep::Write) {
        Node* p = arr;
        const bool write = (s == Sweep::Write);
        const auto start{std::chrono::steady_clock::now()};
        for (i64 i = 0; i < num_accesses; ++i) {
            if (write)
                p->writeto = static_cast<u64>(i);
            p = p->next;
        }
        const auto finish{std::chrono::steady_clock::now()};
        // volatile assignment so compiler dosen't frick me
        asm volatile("" ::"r"(p) : "memory");
        return std::chrono::duration<f64, std::nano>(finish - start).count() /
               static_cast<f64>(num_accesses);
    } else {
        // Needs logic
        return F64_MIN;
    }
}

#endif
