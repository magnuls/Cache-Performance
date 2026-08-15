#ifndef NODE_H
#define NODE_H
// The chased element, sized and aligned to one cache line.

#include "types.h"

// The error will happen if you are not on an apple device
#ifndef __APPLE__
#error "cache_bench targets Apple Silicon"
#endif

#ifndef CACHE_LINE
#define CACHE_LINE 128
#endif

// Each node is 128 bytes (one cache line)
inline constexpr i64 kcache_line_size = CACHE_LINE;
struct alignas(kcache_line_size) Node {
    Node* next;
    u64 writeto = 1u;
};

static_assert(sizeof(Node) == kcache_line_size);

#endif
