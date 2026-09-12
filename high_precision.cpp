#include "high_precision.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cassert>
#include <cctype>
#include <climits>
#include <cstring>

namespace hacker_wang {

    // ---------- 辅助 ----------
    void int_hp::trim() {
        while (digits.size() > 1 && digits.back() == 0)
            digits.pop_back();
    }

    void int_hp::normalize() {
        trim();
        if (digits.size() == 1 && digits[0] == 0)
            negative = false;
    }

    int int_hp::compare_abs(const int_hp& a, const int_hp& b) {
        if (a.digits.size() != b.digits.size())
            return a.digits.size() > b.digits.size() ? 1 : -1;
        for (size_t i = a.digits.size(); i > 0; --i) {
            size_t idx = i - 1;
            if (a.digits[idx] != b.digits[idx])
                return a.digits[idx] > b.digits[idx] ? 1 : -1;
        }
        return 0;
    }

    // ---------- 构造 ----------
    int_hp::int_hp() : digits(1, 0), negative(false) {}

    int_hp::int_hp(long long n) {
        // 用无符号处理 LLONG_MIN 取负的 UB
        unsigned long long u;
        if (n < 0) {
            negative = true;
            u = 0ULL - static_cast<unsigned long long>(n);
        } else {
            negative = false;
            u = static_cast<unsigned long long>(n);
        }
        if (u == 0) {
            digits.push_back(0);
        } else {
            while (u) {
                digits.push_back(static_cast<uint32_t>(u & 0xFFFFFFFFULL));
                u >>= 32;
            }
        }
    }

    int_hp::int_hp(const std::string& s) {
        parse_string(s, *this);
    }

    // ---------- 自增/自减 ----------
    int_hp& int_hp::operator++() { *this += 1; return *this; }
    int_hp int_hp::operator++(int) { int_hp temp = *this; ++(*this); return temp; }
    int_hp& int_hp::operator--() { *this -= 1; return *this; }
    int_hp int_hp::operator--(int) { int_hp temp = *this; --(*this); return temp; }

    int_hp abs(int_hp a) { a.negative = false; return a; }

    // ---------- 绝对值加减（位运算） ----------
    int_hp int_hp::add_abs(const int_hp& a, const int_hp& b) {
        int_hp result;
        result.digits.clear();
        const size_t an = a.digits.size();
        const size_t bn = b.digits.size();
        size_t n = std::max(an, bn);
        result.digits.reserve(n + 1);

        const uint32_t* ap = a.digits.data();
        const uint32_t* bp = b.digits.data();
        uint64_t carry = 0;
        size_t i = 0;
        const size_t common = std::min(an, bn);
        for (; i < common; ++i) {
            uint64_t sum = static_cast<uint64_t>(ap[i]) + bp[i] + carry;
            result.digits.push_back(static_cast<uint32_t>(sum));
            carry = sum >> 32;
        }
        for (; i < an; ++i) {
            uint64_t sum = static_cast<uint64_t>(ap[i]) + carry;
            result.digits.push_back(static_cast<uint32_t>(sum));
            carry = sum >> 32;
        }
        for (; i < bn; ++i) {
            uint64_t sum = static_cast<uint64_t>(bp[i]) + carry;
            result.digits.push_back(static_cast<uint32_t>(sum));
            carry = sum >> 32;
        }
        if (carry) result.digits.push_back(static_cast<uint32_t>(carry));
        result.negative = false;
        return result;
    }

    int_hp int_hp::sub_abs(const int_hp& a, const int_hp& b) {
        assert(compare_abs(a, b) >= 0);
        int_hp result;
        const size_t an = a.digits.size();
        const size_t bn = b.digits.size();
        result.digits.resize(an);

        const uint32_t* ap = a.digits.data();
        const uint32_t* bp = b.digits.data();
        uint32_t* rp = result.digits.data();

        uint64_t borrow = 0;
        size_t i = 0;
        for (; i < bn; ++i) {
            uint64_t diff = static_cast<uint64_t>(ap[i]) - bp[i] - borrow;
            rp[i] = static_cast<uint32_t>(diff);
            borrow = (diff >> 32) & 1ULL;
        }
        for (; i < an; ++i) {
            uint64_t diff = static_cast<uint64_t>(ap[i]) - borrow;
            rp[i] = static_cast<uint32_t>(diff);
            borrow = (diff >> 32) & 1ULL;
        }
        result.trim();
        result.negative = false;
        return result;
    }

    // ---------- 朴素乘法 ----------
    int_hp int_hp::naive_mul(const uint32_t* a, size_t an,
                             const uint32_t* b, size_t bn) {
        int_hp result;
        result.digits.assign(an + bn + 1, 0);
        uint32_t* rd = result.digits.data();
        for (size_t i = 0; i < an; ++i) {
            const uint64_t ai = a[i];
            uint64_t carry = 0;
            size_t k = i;
            for (size_t j = 0; j < bn; ++j, ++k) {
                uint64_t cur = static_cast<uint64_t>(rd[k]) + ai * b[j] + carry;
                rd[k] = static_cast<uint32_t>(cur);
                carry = cur >> 32;
            }
            while (carry) {
                uint64_t cur = static_cast<uint64_t>(rd[k]) + carry;
                rd[k] = static_cast<uint32_t>(cur);
                carry = cur >> 32;
                ++k;
            }
        }
        result.trim();
        return result;
    }

    // ================================================================
    //                        NTT 乘法基础设施
    // ================================================================
    namespace {

        // 三个 NTT 友好质数（均为 k·2^t + 1 形式），原根都是 3
        constexpr uint32_t NTT_M1 = 998244353u;    // 119·2^23 + 1
        constexpr uint32_t NTT_M2 = 1004535809u;   // 479·2^21 + 1
        constexpr uint32_t NTT_M3 = 469762049u;    //   7·2^26 + 1
        constexpr uint32_t NTT_G  = 3u;

        // NTT 长度上限（受 M2 的 2^21 限制）
        constexpr size_t NTT_MAX_N = (size_t)1 << 21;
        // 输入 limb 数的上限（保守取 2^20，避免二次幂长度超过 NTT_MAX_N）
        constexpr size_t NTT_MAX_INPUT = (size_t)1 << 20;
        // 触发 NTT 的阈值
        constexpr size_t NTT_THRESHOLD = 256;

        inline uint32_t mul_mod(uint32_t a, uint32_t b, uint32_t m) {
            return static_cast<uint32_t>(static_cast<uint64_t>(a) * b % m);
        }

        inline uint32_t pow_mod(uint32_t a, uint64_t e, uint32_t m) {
            uint32_t r = 1;
            a %= m;
            while (e) {
                if (e & 1) r = mul_mod(r, a, m);
                a = mul_mod(a, a, m);
                e >>= 1;
            }
            return r;
        }

        inline uint32_t inv_mod(uint32_t a, uint32_t m) {
            // m 是素数，费马小定理
            return pow_mod(a, m - 2, m);
        }

        // 标准基-2 Cooley-Tukey NTT（含 bit-reversal）
        void ntt(std::vector<uint32_t>& a, bool invert, uint32_t mod, uint32_t g) {
            const size_t n = a.size();
            for (size_t i = 1, j = 0; i < n; ++i) {
                size_t bit = n >> 1;
                for (; j & bit; bit >>= 1) j ^= bit;
                j ^= bit;
                if (i < j) std::swap(a[i], a[j]);
            }
            for (size_t len = 2; len <= n; len <<= 1) {
                uint32_t wlen = pow_mod(g, (mod - 1) / len, mod);
                if (invert) wlen = inv_mod(wlen, mod);
                const size_t half = len >> 1;
                for (size_t i = 0; i < n; i += len) {
                    uint32_t w = 1;
                    for (size_t j = 0; j < half; ++j) {
                        uint32_t u = a[i + j];
                        uint32_t v = mul_mod(a[i + j + half], w, mod);
                        uint32_t s = u + v;
                        if (s >= mod) s -= mod;
                        uint32_t d = (u >= v) ? (u - v) : (u + mod - v);
                        a[i + j] = s;
                        a[i + j + half] = d;
                        w = mul_mod(w, wlen, mod);
                    }
                }
            }
            if (invert) {
                uint32_t n_inv = inv_mod(static_cast<uint32_t>(n % mod), mod);
                for (auto& x : a) x = mul_mod(x, n_inv, mod);
            }
        }

        // CRT 预计算
        const uint64_t M1M2 = static_cast<uint64_t>(NTT_M1) * NTT_M2;
        const uint32_t M1_MOD_M2 = NTT_M1 % NTT_M2;
        const uint32_t INV_M1_MOD_M2 = inv_mod(M1_MOD_M2, NTT_M2);
        const uint32_t M1M2_MOD_M3 = static_cast<uint32_t>(M1M2 % NTT_M3);
        const uint32_t INV_M1M2_MOD_M3 = inv_mod(M1M2_MOD_M3, NTT_M3);
        const uint32_t M1_MOD_M3 = NTT_M1 % NTT_M3;

        // 合并 r1(mod M1), r2(mod M2), r3(mod M3) 得唯一 x ∈ [0, M1*M2*M3)
        inline unsigned __int128 crt3(uint32_t r1, uint32_t r2, uint32_t r3) {
            // 第一步：合并 M1, M2
            uint32_t d2 = (r2 >= M1_MOD_M2)
                            ? (r2 - M1_MOD_M2) : (r2 + NTT_M2 - M1_MOD_M2);
            // 注意：r1 < M1 < M2，所以 r1 % M2 == r1；用 M1_MOD_M2 是复用，但我们要的是 r1 % M2。
            // 由于 r1 < M2，实际 r1 % M2 = r1，所以下面使用 r1 即可。
            (void)d2;
            uint32_t t2 = mul_mod(r2 >= (r1 % NTT_M2) ? (r2 - r1 % NTT_M2)
                                                     : (r2 + NTT_M2 - r1 % NTT_M2),
                                  INV_M1_MOD_M2, NTT_M2);
            // 第二步：合并 r12 与 M3
            uint32_t r12_mod_m3 = static_cast<uint32_t>(
                (static_cast<uint64_t>(M1_MOD_M3) * t2 + r1) % NTT_M3);
            uint32_t t3 = mul_mod(r3 >= r12_mod_m3 ? (r3 - r12_mod_m3)
                                                   : (r3 + NTT_M3 - r12_mod_m3),
                                  INV_M1M2_MOD_M3, NTT_M3);
            return static_cast<unsigned __int128>(M1M2) * t3
                 + static_cast<unsigned __int128>(NTT_M1) * t2
                 + r1;
        }

    } // anonymous namespace

    // ---------- NTT 乘法 ----------
    int_hp int_hp::ntt_mul(const uint32_t* a, size_t an,
                           const uint32_t* b, size_t bn) {
        const size_t total = an + bn - 1;
        size_t n = 1;
        while (n < total) n <<= 1;

        // 万一超限，退化回 Karatsuba（此分支由上层保证不轻易触发）
        if (n > NTT_MAX_N)
            return karatsuba_mul(a, an, b, bn);

        std::vector<uint32_t> fa(n), fb(n);

        // --- M1 ---
        for (size_t i = 0; i < an; ++i) fa[i] = a[i] % NTT_M1;
        for (size_t i = 0; i < bn; ++i) fb[i] = b[i] % NTT_M1;
        ntt(fa, false, NTT_M1, NTT_G);
        ntt(fb, false, NTT_M1, NTT_G);
        for (size_t i = 0; i < n; ++i) fa[i] = mul_mod(fa[i], fb[i], NTT_M1);
        ntt(fa, true,  NTT_M1, NTT_G);

        // --- M2 ---
        std::vector<uint32_t> ga(n), gb(n);
        for (size_t i = 0; i < an; ++i) ga[i] = a[i] % NTT_M2;
        for (size_t i = 0; i < bn; ++i) gb[i] = b[i] % NTT_M2;
        ntt(ga, false, NTT_M2, NTT_G);
        ntt(gb, false, NTT_M2, NTT_G);
        for (size_t i = 0; i < n; ++i) ga[i] = mul_mod(ga[i], gb[i], NTT_M2);
        ntt(ga, true,  NTT_M2, NTT_G);

        // --- M3 ---
        std::vector<uint32_t> ha(n), hb(n);
        for (size_t i = 0; i < an; ++i) ha[i] = a[i] % NTT_M3;
        for (size_t i = 0; i < bn; ++i) hb[i] = b[i] % NTT_M3;
        ntt(ha, false, NTT_M3, NTT_G);
        ntt(hb, false, NTT_M3, NTT_G);
        for (size_t i = 0; i < n; ++i) ha[i] = mul_mod(ha[i], hb[i], NTT_M3);
        ntt(ha, true,  NTT_M3, NTT_G);

        // --- CRT 合并 + 32 位进位 ---
        int_hp result;
        result.digits.resize(total, 0);
        unsigned __int128 carry = 0;
        for (size_t i = 0; i < total; ++i) {
            unsigned __int128 x = crt3(fa[i], ga[i], ha[i]) + carry;
            result.digits[i] = static_cast<uint32_t>(x & 0xFFFFFFFFULL);
            carry = x >> 32;
        }
        while (carry) {
            result.digits.push_back(static_cast<uint32_t>(carry & 0xFFFFFFFFULL));
            carry >>= 32;
        }
        result.trim();
        return result;
    }

    // ---------- Karatsuba（小规模）；超过阈值调用 NTT ----------
    static const size_t KARATSUBA_THRESHOLD = 48;

    int_hp int_hp::karatsuba_mul(const uint32_t* a, size_t an,
                                 const uint32_t* b, size_t bn) {
        const size_t n = std::max(an, bn);
        const size_t m = std::min(an, bn);

        // 超大：优先走 NTT
        if (m >= NTT_THRESHOLD && n <= NTT_MAX_INPUT) {
            return ntt_mul(a, an, b, bn);
        }
        // 小规模：朴素
        if (n <= KARATSUBA_THRESHOLD) {
            return naive_mul(a, an, b, bn);
        }

        // Karatsuba 分治（若 n 超过 NTT_MAX_INPUT，递归到子块仍会走 NTT）
        const size_t half = n / 2;
        const size_t a0_len = std::min(an, half);
        const size_t a1_len = an - a0_len;
        const size_t b0_len = std::min(bn, half);
        const size_t b1_len = bn - b0_len;

        const uint32_t* a0 = a;
        const uint32_t* a1 = a + a0_len;
        const uint32_t* b0 = b;
        const uint32_t* b1 = b + b0_len;

        int_hp z0 = karatsuba_mul(a0, a0_len, b0, b0_len);
        int_hp z2 = karatsuba_mul(a1, a1_len, b1, b1_len);

        int_hp sum_a, sum_b;
        {
            sum_a.digits.clear();
            sum_a.digits.reserve(std::max(a0_len, a1_len) + 1);
            uint64_t carry = 0;
            size_t i = 0;
            const size_t c = std::min(a0_len, a1_len);
            for (; i < c; ++i) {
                uint64_t s = static_cast<uint64_t>(a0[i]) + a1[i] + carry;
                sum_a.digits.push_back(static_cast<uint32_t>(s));
                carry = s >> 32;
            }
            for (; i < a0_len; ++i) {
                uint64_t s = static_cast<uint64_t>(a0[i]) + carry;
                sum_a.digits.push_back(static_cast<uint32_t>(s));
                carry = s >> 32;
            }
            for (; i < a1_len; ++i) {
                uint64_t s = static_cast<uint64_t>(a1[i]) + carry;
                sum_a.digits.push_back(static_cast<uint32_t>(s));
                carry = s >> 32;
            }
            if (carry) sum_a.digits.push_back(static_cast<uint32_t>(carry));
            sum_a.trim();
        }
        {
            sum_b.digits.clear();
            sum_b.digits.reserve(std::max(b0_len, b1_len) + 1);
            uint64_t carry = 0;
            size_t i = 0;
            const size_t c = std::min(b0_len, b1_len);
            for (; i < c; ++i) {
                uint64_t s = static_cast<uint64_t>(b0[i]) + b1[i] + carry;
                sum_b.digits.push_back(static_cast<uint32_t>(s));
                carry = s >> 32;
            }
            for (; i < b0_len; ++i) {
                uint64_t s = static_cast<uint64_t>(b0[i]) + carry;
                sum_b.digits.push_back(static_cast<uint32_t>(s));
                carry = s >> 32;
            }
            for (; i < b1_len; ++i) {
                uint64_t s = static_cast<uint64_t>(b1[i]) + carry;
                sum_b.digits.push_back(static_cast<uint32_t>(s));
                carry = s >> 32;
            }
            if (carry) sum_b.digits.push_back(static_cast<uint32_t>(carry));
            sum_b.trim();
        }

        int_hp z1 = karatsuba_mul(sum_a.digits.data(), sum_a.digits.size(),
                                  sum_b.digits.data(), sum_b.digits.size());
        z1 = sub_abs(z1, z0);
        z1 = sub_abs(z1, z2);

        // 左移合并：z0 + (z1 << half) + (z2 << 2*half)
        {
            const size_t old = z0.digits.size();
            z0.digits.resize(old + half);
            std::memmove(z0.digits.data() + half, z0.digits.data(),
                         old * sizeof(uint32_t));
            std::memset(z0.digits.data(), 0, half * sizeof(uint32_t));
        }
        {
            const size_t old = z2.digits.size();
            z2.digits.resize(old + 2 * half);
            std::memmove(z2.digits.data() + 2 * half, z2.digits.data(),
                         old * sizeof(uint32_t));
            std::memset(z2.digits.data(), 0, 2 * half * sizeof(uint32_t));
        }
        {
            const size_t old = z1.digits.size();
            z1.digits.resize(old + half);
            std::memmove(z1.digits.data() + half, z1.digits.data(),
                         old * sizeof(uint32_t));
            std::memset(z1.digits.data(), 0, half * sizeof(uint32_t));
        }

        int_hp result = add_abs(add_abs(z0, z1), z2);
        result.trim();
        return result;
    }

    int_hp int_hp::multiply_abs(const int_hp& a, const int_hp& b) {
        if (a.digits.size() == 1 && a.digits[0] == 0) return int_hp();
        if (b.digits.size() == 1 && b.digits[0] == 0) return int_hp();
        return karatsuba_mul(a.digits.data(), a.digits.size(),
                             b.digits.data(), b.digits.size());
    }

    // ---------- 单肢乘除 ----------
    int_hp int_hp::multiply_small(const int_hp& a, uint32_t b) {
        if (b == 0) return int_hp();
        if (b == 1) return a;
        int_hp result;
        result.digits.clear();
        result.digits.reserve(a.digits.size() + 1);
        uint64_t carry = 0;
        for (size_t i = 0; i < a.digits.size(); ++i) {
            uint64_t cur = static_cast<uint64_t>(a.digits[i]) * b + carry;
            result.digits.push_back(static_cast<uint32_t>(cur));
            carry = cur >> 32;
        }
        if (carry) result.digits.push_back(static_cast<uint32_t>(carry));
        result.trim();
        result.negative = a.negative;
        return result;
    }

    int_hp int_hp::divide_small(const int_hp& a, uint32_t b, uint32_t& rem) {
        if (b == 0) throw std::domain_error("Division by zero");
        if (b == 1) { rem = 0; return a; }
        int_hp quotient;
        quotient.digits.resize(a.digits.size());
        uint64_t remainder = 0;
        for (size_t i = a.digits.size(); i > 0; --i) {
            size_t idx = i - 1;
            uint64_t cur = (remainder << 32) | a.digits[idx];
            quotient.digits[idx] = static_cast<uint32_t>(cur / b);
            remainder = cur % b;
        }
        quotient.trim();
        rem = static_cast<uint32_t>(remainder);
        quotient.negative = a.negative;
        return quotient;
    }

    // ---------- 通用除法（Knuth D） ----------
    std::pair<int_hp, int_hp> int_hp::divmod_abs(const int_hp& a, const int_hp& b) {
        if (compare_abs(a, b) < 0)
            return {int_hp(), a};
        if (b.digits.size() == 1) {
            uint32_t rem;
            int_hp q = divide_small(a, b.digits[0], rem);
            int_hp r; r.digits = { rem };
            return {q, r};
        }

        uint32_t shift = 0;
        uint64_t scale = 1;
        uint32_t v_high = b.digits.back();
        while (v_high < (1U << 31)) {
            v_high <<= 1;
            ++shift;
            scale <<= 1;
        }

        int_hp divisor = b;
        int_hp dividend = a;
        if (shift > 0) {
            uint64_t carry = 0;
            for (size_t i = 0; i < divisor.digits.size(); ++i) {
                uint64_t cur = static_cast<uint64_t>(divisor.digits[i]) * scale + carry;
                divisor.digits[i] = static_cast<uint32_t>(cur);
                carry = cur >> 32;
            }
            if (carry) divisor.digits.push_back(static_cast<uint32_t>(carry));
            carry = 0;
            for (size_t i = 0; i < dividend.digits.size(); ++i) {
                uint64_t cur = static_cast<uint64_t>(dividend.digits[i]) * scale + carry;
                dividend.digits[i] = static_cast<uint32_t>(cur);
                carry = cur >> 32;
            }
            if (carry) dividend.digits.push_back(static_cast<uint32_t>(carry));
        }

        size_t n = divisor.digits.size();
        size_t m = dividend.digits.size() - n + 1;

        int_hp quotient;
        quotient.digits.resize(m, 0);

        std::vector<uint32_t> U(dividend.digits.begin(), dividend.digits.end());
        U.push_back(0);

        for (size_t j = m; j > 0; --j) {
            size_t idx = j - 1;

            uint64_t u0 = U[idx + n];
            uint64_t u1 = U[idx + n - 1];
            uint64_t v0 = divisor.digits[n - 1];
            uint64_t q_hat = (u0 * BASE + u1) / v0;
            if (q_hat >= BASE) q_hat = BASE - 1;

            while (n >= 2) {
                uint64_t v1 = divisor.digits[n - 2];
                unsigned __int128 lhs = (unsigned __int128)q_hat * v1;
                unsigned __int128 rhs =
                    (unsigned __int128)(u0 * BASE + u1 - q_hat * v0) * BASE
                    + U[idx + n - 2];
                if (lhs <= rhs) break;
                --q_hat;
            }

            int64_t borrow = 0;
            for (size_t i = 0; i < n; ++i) {
                unsigned __int128 product =
                    (unsigned __int128)q_hat * divisor.digits[i];
                uint64_t prod_low  = (uint64_t)(product % BASE);
                uint64_t prod_high = (uint64_t)(product / BASE);

                int64_t diff = (int64_t)U[idx + i] - borrow - (int64_t)prod_low;
                if (diff < 0) {
                    diff += BASE;
                    borrow = (int64_t)prod_high + 1;
                } else {
                    borrow = (int64_t)prod_high;
                }
                U[idx + i] = (uint32_t)diff;
            }
            int64_t final_diff = (int64_t)U[idx + n] - borrow;
            if (final_diff < 0) {
                --q_hat;
                uint64_t carry = 0;
                for (size_t i = 0; i < n; ++i) {
                    uint64_t cur = (uint64_t)U[idx + i] + divisor.digits[i] + carry;
                    U[idx + i] = (uint32_t)cur;
                    carry = cur >> 32;
                }
                U[idx + n] = (uint32_t)((uint64_t)U[idx + n] + carry);
            } else {
                U[idx + n] = (uint32_t)final_diff;
            }

            quotient.digits[idx] = (uint32_t)q_hat;
        }

        quotient.trim();

        int_hp remainder;
        remainder.digits.assign(U.begin(), U.begin() + n);
        remainder.trim();

        if (shift > 0) {
            uint64_t carry = 0;
            for (size_t i = remainder.digits.size(); i > 0; --i) {
                size_t idx = i - 1;
                uint64_t cur = carry * BASE + remainder.digits[idx];
                remainder.digits[idx] = (uint32_t)(cur / scale);
                carry = cur % scale;
            }
            remainder.trim();
        }

        return {quotient, remainder};
    }

    // ---------- 外部运算符 ----------
    int_hp operator*(const int_hp& a, const int_hp& b) {
        if (a.digits.size() == 1 && a.digits[0] == 0) return int_hp();
        if (b.digits.size() == 1 && b.digits[0] == 0) return int_hp();
        int_hp result = int_hp::multiply_abs(a, b);
        result.negative = (a.negative != b.negative);
        result.normalize();
        return result;
    }

    int_hp operator/(int_hp a, int_hp b) {
        if (b.digits.size() == 1 && b.digits[0] == 0)
            throw std::domain_error("Division by zero");
        if (a.digits.size() == 1 && a.digits[0] == 0) return int_hp();
        bool neg = (a.negative != b.negative);
        a.negative = false; b.negative = false;
        auto [q, r] = int_hp::divmod_abs(a, b);
        q.negative = neg;
        q.normalize();
        return q;
    }

    int_hp operator%(const int_hp& a, const int_hp& b) {
        if (b.digits.size() == 1 && b.digits[0] == 0)
            throw std::domain_error("Modulo by zero");
        int_hp abs_a = a; abs_a.negative = false;
        int_hp abs_b = b; abs_b.negative = false;
        auto [q, r] = int_hp::divmod_abs(abs_a, abs_b);
        r.negative = a.negative;
        r.normalize();
        return r;
    }

    int_hp operator^(int_hp a, int_hp b) {
        if (b.negative)
            throw std::runtime_error("Negative exponent not supported");
        if (b.digits.size() == 1 && b.digits[0] == 0)
            return int_hp(1);

        bool base_neg = a.negative;
        a.negative = false;
        bool exp_odd = (b.digits[0] & 1);

        int_hp result(1);
        while (true) {
            if (b.digits.size() == 1 && b.digits[0] == 0) break;
            if (b.digits[0] & 1)
                result = result * a;
            a = a * a;
            uint64_t carry = 0;
            for (size_t i = b.digits.size(); i > 0; --i) {
                size_t idx = i - 1;
                uint64_t cur = (carry << 32) | b.digits[idx];
                b.digits[idx] = (uint32_t)(cur >> 1);
                carry = cur & 1;
            }
            if (b.digits.size() > 1 && b.digits.back() == 0)
                b.trim();
        }

        if (base_neg && exp_odd)
            result.negative = true;
        return result;
    }

    // ---------- 加减运算符 ----------
    int_hp operator+(int_hp a, int_hp b) {
        if (a.negative == b.negative) {
            int_hp result = int_hp::add_abs(a, b);
            result.negative = a.negative;
            return result;
        } else {
            if (a.negative) {
                a.negative = false;
                if (int_hp::compare_abs(b, a) >= 0)
                    return int_hp::sub_abs(b, a);
                else {
                    int_hp result = int_hp::sub_abs(a, b);
                    result.negative = true;
                    return result;
                }
            } else {
                b.negative = false;
                if (int_hp::compare_abs(a, b) >= 0)
                    return int_hp::sub_abs(a, b);
                else {
                    int_hp result = int_hp::sub_abs(b, a);
                    result.negative = true;
                    return result;
                }
            }
        }
    }

    int_hp operator-(int_hp a) {
        if (!(a.digits.size() == 1 && a.digits[0] == 0))
            a.negative = !a.negative;
        return a;
    }

    int_hp operator-(int_hp a, int_hp b) {
        b.negative = !b.negative;
        return a + b;
    }

    // ---------- 比较 ----------
    bool operator==(const int_hp& a, const int_hp& b) {
        return a.negative == b.negative && a.digits == b.digits;
    }
    bool operator!=(const int_hp& a, const int_hp& b) { return !(a == b); }
    bool operator>(const int_hp& a, const int_hp& b) {
        if (a.negative != b.negative) return b.negative;
        int cmp = int_hp::compare_abs(a, b);
        return a.negative ? cmp < 0 : cmp > 0;
    }
    bool operator<(const int_hp& a, const int_hp& b) { return b > a; }
    bool operator>=(const int_hp& a, const int_hp& b) { return !(a < b); }
    bool operator<=(const int_hp& a, const int_hp& b) { return !(a > b); }

    // ---------- 复合赋值 ----------
    int_hp& operator+=(int_hp& a, const int_hp& b) { a = a + b; return a; }
    int_hp& operator-=(int_hp& a, const int_hp& b) { a = a - b; return a; }
    int_hp& operator*=(int_hp& a, const int_hp& b) { a = a * b; return a; }
    int_hp& operator/=(int_hp& a, const int_hp& b) { a = a / b; return a; }
    int_hp& operator%=(int_hp& a, const int_hp& b) { a = a % b; return a; }
    int_hp& operator^=(int_hp& a, const int_hp& b) { a = a ^ b; return a; }

    // ================================================================
    //             分治十进制 IO（O(M(n)·log n)）
    // ================================================================
    namespace {

        // pow10_tab[i] = 10^(9 * 2^i)
        std::vector<int_hp>& pow10_tab() {
            static std::vector<int_hp> tab;
            if (tab.empty()) {
                tab.push_back(int_hp(1000000000LL));   // 10^9
            }
            return tab;
        }

        void ensure_tab(size_t i) {
            auto& tab = pow10_tab();
            while (tab.size() <= i)
                tab.push_back(tab.back() * tab.back());
        }

        size_t split_index(size_t n_limbs) {
            auto& tab = pow10_tab();
            size_t i = 0;
            while (true) {
                if (i + 1 >= tab.size()) {
                    if (tab[i].size() * 2 > n_limbs) return i;
                    tab.push_back(tab.back() * tab.back());
                }
                if (tab[i + 1].size() * 2 <= n_limbs) ++i;
                else return i;
            }
        }

    } // anonymous namespace

    void int_hp::to_string_dc(const int_hp& n, std::string& out) {
        if (n.size() <= 2) {
            uint64_t v = n[0];
            if (n.size() >= 2)
                v |= static_cast<uint64_t>(n[1]) << 32;
            out += std::to_string(v);
            return;
        }

        size_t i = split_index(n.size());
        ensure_tab(i);
        const int_hp& p = pow10_tab()[i];

        int_hp q = n / p;
        int_hp r = n % p;

        to_string_dc(q, out);

        std::string r_str;
        r_str.reserve(32);
        to_string_dc(r, r_str);

        size_t width = static_cast<size_t>(9) << i;   // 9 * 2^i
        if (r_str.size() < width)
            out.append(width - r_str.size(), '0');
        out += r_str;
    }

    int_hp int_hp::parse_digits_dc(const std::string& s, size_t lo, size_t hi) {
        const size_t len = hi - lo;
        if (len <= 18) {
            uint64_t v = 0;
            for (size_t k = lo; k < hi; ++k)
                v = v * 10 + static_cast<uint64_t>(s[k] - '0');
            return int_hp(static_cast<long long>(v));
        }

        size_t q = 0;
        while ((static_cast<size_t>(9) << (q + 1)) <= len) ++q;
        size_t i = q - 1;
        size_t right_len = static_cast<size_t>(9) << i;
        ensure_tab(i);

        size_t mid = hi - right_len;
        int_hp left  = parse_digits_dc(s, lo, mid);
        int_hp right = parse_digits_dc(s, mid, hi);
        return left * pow10_tab()[i] + right;
    }

    // ---------- IO ----------
    void int_hp::parse_string(const std::string& s, int_hp& hp) {
        hp = 0;
        size_t pos = 0;
        bool neg = false;
        if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
            neg = (s[0] == '-');
            pos = 1;
        }
        for (size_t i = pos; i < s.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(s[i])))
                throw std::invalid_argument("int_hp: invalid character");
        }
        if (pos >= s.size()) {
            hp = 0;
            hp.negative = false;
            return;
        }

        size_t start = pos;
        while (start < s.size() && s[start] == '0') ++start;
        if (start == s.size()) {
            hp = 0;
            hp.negative = false;
            return;
        }

        hp = parse_digits_dc(s, start, s.size());
        hp.negative = neg;
        hp.normalize();
    }

    std::string int_hp::to_string(const int_hp& hp) {
        if (hp.digits.size() == 1 && hp.digits[0] == 0)
            return "0";

        std::string result;
        result.reserve(hp.digits.size() * 10 + 2);
        if (hp.negative) result += '-';

        int_hp temp = hp;
        temp.negative = false;
        to_string_dc(temp, result);
        return result;
    }

    std::istream& operator>>(std::istream& is, int_hp& hp) {
        std::string s;
        is >> s;
        int_hp::parse_string(s, hp);
        return is;
    }

    std::ostream& operator<<(std::ostream& os, const int_hp& hp) {
        os << int_hp::to_string(hp);
        return os;
    }

} // namespace hacker_wang
