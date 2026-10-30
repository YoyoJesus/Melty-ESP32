#pragma once
#include <stddef.h>
#include <stdint.h>

struct LogEntry { uint32_t ms; float rpm, radialG; uint16_t aUs, bUs; };
template <size_t N> class RunLog {
    static_assert(N > 0, "RunLog capacity must be positive");
public:
    void add(const LogEntry& e) {
        entries_[(start_ + size_) % N] = e;
        if (size_ < N) ++size_; else start_ = (start_ + 1) % N;
    }
    size_t size() const { return size_; }
    const LogEntry& at(size_t i) const { return entries_[(start_ + i) % N]; }
    void clear() { start_ = 0; size_ = 0; }
private:
    LogEntry entries_[N]{};
    size_t start_ = 0, size_ = 0;
};
