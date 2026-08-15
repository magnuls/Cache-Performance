#ifndef SATTOLO_H
#define SATTOLO_H
// Generic single-cycle shuffle. Knows nothing about caches.

#include <random>
#include <utility>

#include "types.h"

/*
 * sattolo -> shuffles next pointers in place, producing a single
 * cycle through all elements. Works with any T that has a
 * T* next member.
 * Precondition: arr must be identity-linked (arr[i].next == &arr[i]).
 */
template<typename T>
void sattolo(T* arr, i64 count, std::mt19937_64& engine) {
    for (i64 i = 0; i < count - 1; ++i) {
        std::uniform_int_distribution<i64> dist(i + 1, count - 1);
        std::swap(arr[i].next, arr[dist(engine)].next);
    }
}

#endif
