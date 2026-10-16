#pragma once
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace lss {

class BitVec {
public:
    BitVec() = default;
    explicit BitVec(std::size_t nbits) : nbits_(nbits), words_((nbits + 63) / 64, 0) {}

    std::size_t size() const { return nbits_; }
    const std::vector<std::uint64_t>& words() const { return words_; }

    bool test(std::size_t i) const { return (words_[i >> 6] >> (i & 63)) & 1ULL; }
    void set(std::size_t i) { words_[i >> 6] |= (1ULL << (i & 63)); }
    void reset(std::size_t i) { words_[i >> 6] &= ~(1ULL << (i & 63)); }
    void assign(std::size_t i, bool v) { v ? set(i) : reset(i); }
    void flip(std::size_t i) { words_[i >> 6] ^= (1ULL << (i & 63)); }

    std::size_t count() const {
        std::size_t c = 0;
        for (std::uint64_t w : words_) c += static_cast<std::size_t>(std::popcount(w));
        return c;
    }
    bool any() const {
        for (std::uint64_t w : words_) if (w) return true;
        return false;
    }
    bool none() const { return !any(); }

    BitVec& operator|=(const BitVec& o) {
        for (std::size_t i = 0; i < words_.size(); ++i) words_[i] |= o.words_[i];
        return *this;
    }
    BitVec& operator&=(const BitVec& o) {
        for (std::size_t i = 0; i < words_.size(); ++i) words_[i] &= o.words_[i];
        return *this;
    }
    BitVec& operator^=(const BitVec& o) {
        for (std::size_t i = 0; i < words_.size(); ++i) words_[i] ^= o.words_[i];
        return *this;
    }
    BitVec& subtract(const BitVec& o) {
        for (std::size_t i = 0; i < words_.size(); ++i) words_[i] &= ~o.words_[i];
        return *this;
    }

    bool subset_of(const BitVec& o) const {
        for (std::size_t i = 0; i < words_.size(); ++i)
            if (words_[i] & ~o.words_[i]) return false;
        return true;
    }
    bool intersects(const BitVec& o) const {
        for (std::size_t i = 0; i < words_.size(); ++i)
            if (words_[i] & o.words_[i]) return true;
        return false;
    }

    std::size_t find_first() const { return find_from(0); }
    std::size_t find_next(std::size_t i) const { return find_from(i + 1); }
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    bool operator==(const BitVec& o) const { return nbits_ == o.nbits_ && words_ == o.words_; }
    bool operator!=(const BitVec& o) const { return !(*this == o); }
    bool operator<(const BitVec& o) const {
        if (nbits_ != o.nbits_) return nbits_ < o.nbits_;
        for (std::size_t i = words_.size(); i-- > 0;)
            if (words_[i] != o.words_[i]) return words_[i] < o.words_[i];
        return false;
    }

    std::size_t hash() const {
        std::size_t h = 1469598103934665603ULL ^ nbits_;
        for (std::uint64_t w : words_) {
            h ^= static_cast<std::size_t>(w) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return h;
    }

private:
    std::size_t find_from(std::size_t i) const {
        if (i >= nbits_) return npos;
        std::size_t w = i >> 6;
        std::uint64_t cur = words_[w] & (~0ULL << (i & 63));
        while (true) {
            if (cur) return (w << 6) + static_cast<std::size_t>(std::countr_zero(cur));
            if (++w >= words_.size()) return npos;
            cur = words_[w];
        }
    }

    std::size_t nbits_ = 0;
    std::vector<std::uint64_t> words_;
};

} // namespace lss
