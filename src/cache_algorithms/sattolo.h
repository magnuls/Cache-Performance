#ifndef SATTOLO_H
#define SATTOLO_H
// Generic single-cycle shuffle. Knows nothing about caches.

#include <numeric>
#include <random>
#include <utility>
#include <vector>

#include "types.h"

/*
 * sattolo -> shuffles next pointers in place, producing a single
 * cycle through all elements. Works with any T that has a
 * T* next member.
 * Precondition: arr must be identity-linked (arr[i].next == &arr[i]).
 * This is for array of type t and shuffles in place
 */
template<typename T>
void sattolo(T* arr, i64 count, std::mt19937_64& engine) {
    for (i64 i = 0; i < count - 1; ++i) {
        std::uniform_int_distribution<i64> dist(i + 1, count - 1);
        std::swap(arr[i].next, arr[dist(engine)].next);
    }
}

/*
 * sattolo_cycle -> returns a successor table: next[i] is the slot
 * visited after slot i, producing a single cycle through all slots.
 *
 * Returns a shuffled arrays of size 'count' of type 'u32'
 */
inline std::vector<u32> sattolo_cycle(i64 count, std::mt19937_64& engine) {
    std::vector<u32> order(count);
    std::iota(order.begin(), order.end(), 0);
    for (i64 i = count - 1; i > 0; --i) {
        std::uniform_int_distribution<i64> dist(0, i - 1);
        std::swap(order[i], order[dist(engine)]);
    }

    std::vector<u32> next(count);
    for (i64 i = 0; i < count; ++i) {
        next[order[i]] = order[(i + 1) % count];
    }
    return next;
}

#endif
