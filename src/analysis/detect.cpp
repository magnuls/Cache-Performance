#include "analysis/detect.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

bool is_pow2(i64 v) { return v > 0 && (v & (v - 1)) == 0; }

std::vector<Measurement> power_of_two_points(
    const std::vector<Measurement>& points) {
    std::vector<Measurement> out;
    for (const Measurement& m : points)
        if (is_pow2(m.x)) out.push_back(m);
    return out;
}

std::pair<i64, i64> steepest_step(const std::vector<Measurement>& points,
                                  i64 lo, i64 hi) {
    std::pair<i64, i64> best{lo, hi};
    f64 best_ratio = 0.0;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const Measurement& a = points[i];
        const Measurement& b = points[i + 1];
        if (a.x < lo || b.x > hi || a.ns_per_access <= 0.0) continue;
        const f64 ratio = b.ns_per_access / a.ns_per_access;
        if (ratio > best_ratio) {
            best_ratio = ratio;
            best = {a.x, b.x};
        }
    }
    return best;
}

} // namespace

std::vector<Cliff> find_cliffs(const std::vector<Measurement>& points,
                               f64 min_ratio) {
    std::vector<f64> ratios;
    for (size_t i = 0; i + 1 < points.size(); ++i) {
        const f64 a = points[i].ns_per_access;
        const f64 b = points[i + 1].ns_per_access;
        ratios.push_back(a > 0.0 ? b / a : 0.0);
    }

    std::vector<Cliff> cliffs;
    for (size_t i = 0; i < ratios.size(); ++i) {
        if (ratios[i] < min_ratio) continue;
        if (i > 0 && ratios[i] < ratios[i - 1]) continue;
        if (i + 1 < ratios.size() && ratios[i] <= ratios[i + 1]) continue;
        cliffs.push_back(Cliff{points[i].x, points[i + 1].x, ratios[i]});
    }
    return cliffs;
}

std::vector<i64> midpoints(i64 lo, i64 hi, i64 n, i64 granularity) {
    std::vector<i64> out;
    if (lo <= 0 || hi <= lo || n <= 0 || granularity <= 0) return out;
    const f64 span = static_cast<f64>(hi) / static_cast<f64>(lo);
    for (i64 k = 1; k <= n; ++k) {
        const f64 exact =
            lo * std::pow(span, static_cast<f64>(k) / (n + 1));
        const i64 v = (static_cast<i64>(exact) / granularity) * granularity;
        if (v <= lo || v >= hi) continue;
        if (!out.empty() && out.back() == v) continue;
        out.push_back(v);
    }
    return out;
}

Detected detect_geometry(const SweepResult& size_sweep,
                         const SweepResult& false_sharing, f64 min_ratio) {
    Detected d;

    const std::vector<Measurement> coarse =
        power_of_two_points(size_sweep.points);
    std::vector<f64> ratios;
    for (size_t i = 0; i + 1 < coarse.size(); ++i) {
        const f64 a = coarse[i].ns_per_access;
        ratios.push_back(a > 0.0 ? coarse[i + 1].ns_per_access / a : 0.0);
    }
    auto next_cliff = [&](size_t from) {
        size_t k = from;
        while (k < ratios.size() && ratios[k] < min_ratio) ++k;
        return k;
    };
    auto place = [&](size_t k, i64& lo, i64& hi) {
        const auto step =
            steepest_step(size_sweep.points, coarse[k].x, coarse[k + 1].x);
        lo = step.first;
        hi = step.second;
    };

    size_t k = next_cliff(0);
    if (k < ratios.size()) {
        place(k, d.l1d, d.l1d_next);
        while (k < ratios.size() && ratios[k] >= min_ratio) ++k;
        k = next_cliff(k);
        if (k < ratios.size()) place(k, d.l2, d.l2_next);
    }

    const std::vector<Measurement>& fs = false_sharing.points;
    f64 best_drop = min_ratio;
    for (size_t i = 0; i + 1 < fs.size(); ++i) {
        if (fs[i + 1].ns_per_access <= 0.0) continue;
        const f64 drop = fs[i].ns_per_access / fs[i + 1].ns_per_access;
        if (drop >= best_drop) {
            best_drop = drop;
            d.line_size = fs[i + 1].x;
        }
    }
    return d;
}
