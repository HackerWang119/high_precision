#pragma once
#ifndef HIGH_PRECISION_H
#define HIGH_PRECISION_H

#include <string>
#include <iostream>
#include <vector>
#include <cstdint>
#include <utility>

namespace hacker_wang {

    class int_hp;

    // 通用输入输出
    std::istream& operator>>(std::istream& is, int_hp& hp);
    std::ostream& operator<<(std::ostream& os, const int_hp& hp);

    // 比较运算
    bool operator==(const int_hp& a, const int_hp& b);
    bool operator!=(const int_hp& a, const int_hp& b);
    bool operator>(const int_hp& a, const int_hp& b);
    bool operator<(const int_hp& a, const int_hp& b);
    bool operator>=(const int_hp& a, const int_hp& b);
    bool operator<=(const int_hp& a, const int_hp& b);

    // 算术运算
    int_hp operator+(int_hp a, int_hp b);
    int_hp operator-(int_hp a);
    int_hp operator-(int_hp a, int_hp b);
    int_hp operator*(const int_hp& a, const int_hp& b);
    int_hp operator/(int_hp a, int_hp b);
    int_hp operator%(const int_hp& a, const int_hp& b);
    int_hp operator^(int_hp a, int_hp b);   // 幂运算

    // 复合赋值
    int_hp& operator+=(int_hp& a, const int_hp& b);
    int_hp& operator-=(int_hp& a, const int_hp& b);
    int_hp& operator*=(int_hp& a, const int_hp& b);
    int_hp& operator/=(int_hp& a, const int_hp& b);
    int_hp& operator%=(int_hp& a, const int_hp& b);
    int_hp& operator^=(int_hp& a, const int_hp& b);

    int_hp abs(int_hp a);

    class int_hp {
    private:
        // 内部采用 2^32 基数，纯位运算
        static constexpr size_t LIMB_BITS = 32;
        static constexpr uint64_t BASE = 1ULL << LIMB_BITS;

        // IO 使用的十进制分组（9 位一组）
        static constexpr uint32_t DEC_BASE = 1000000000;
        static constexpr size_t DEC_DIGITS = 9;

        std::vector<uint32_t> digits;   // 小端存储，每元素 32 位
        bool negative = false;

        void trim();
        void normalize();

        static int compare_abs(const int_hp& a, const int_hp& b);

        // 绝对值加减（位运算）
        static int_hp add_abs(const int_hp& a, const int_hp& b);
        static int_hp sub_abs(const int_hp& a, const int_hp& b);

        // 乘法（朴素 / Karatsuba / NTT）
        static int_hp naive_mul(const uint32_t* a, size_t an,
                                const uint32_t* b, size_t bn);
        static int_hp karatsuba_mul(const uint32_t* a, size_t an,
                                    const uint32_t* b, size_t bn);
        static int_hp ntt_mul(const uint32_t* a, size_t an,
                              const uint32_t* b, size_t bn);
        static int_hp multiply_abs(const int_hp& a, const int_hp& b);

        // 单肢快速乘除（用于 IO 和除法快速路径）
        static int_hp multiply_small(const int_hp& a, uint32_t b);
        static int_hp divide_small(const int_hp& a, uint32_t b, uint32_t& rem);

        // 除法（返回 {商, 余数}，绝对值）
        static std::pair<int_hp, int_hp> divmod_abs(const int_hp& a, const int_hp& b);

        // 解析与输出
        static void parse_string(const std::string& s, int_hp& hp);
        static std::string to_string(const int_hp& hp);

        // 分治十进制 IO（O(M(n)·log n)）
        static int_hp parse_digits_dc(const std::string& s, size_t lo, size_t hi);
        static void   to_string_dc(const int_hp& n, std::string& out);

    public:
        int_hp();
        int_hp(long long n);
        explicit int_hp(const std::string& s);

        int_hp(const int_hp&) = default;
        int_hp(int_hp&&) noexcept = default;

        int_hp& operator=(int_hp other) noexcept {
            swap(other);
            return *this;
        }

        void swap(int_hp& other) noexcept {
            using std::swap;
            swap(digits, other.digits);
            swap(negative, other.negative);
        }

        size_t size() const { return digits.size(); }
        uint32_t operator[](size_t idx) const { return digits[idx]; }

        int_hp& operator++();
        int_hp operator++(int);
        int_hp& operator--();
        int_hp operator--(int);

        friend std::istream& operator>>(std::istream& is, int_hp& hp);
        friend std::ostream& operator<<(std::ostream& os, const int_hp& hp);
        friend std::string to_string(const int_hp& hp);

        friend bool operator==(const int_hp& a, const int_hp& b);
        friend bool operator!=(const int_hp& a, const int_hp& b);
        friend bool operator>(const int_hp& a, const int_hp& b);
        friend bool operator<(const int_hp& a, const int_hp& b);
        friend bool operator>=(const int_hp& a, const int_hp& b);
        friend bool operator<=(const int_hp& a, const int_hp& b);

        friend int_hp operator+(int_hp a, int_hp b);
        friend int_hp operator-(int_hp a);
        friend int_hp operator-(int_hp a, int_hp b);
        friend int_hp operator*(const int_hp& a, const int_hp& b);
        friend int_hp operator/(int_hp a, int_hp b);
        friend int_hp operator%(const int_hp& a, const int_hp& b);
        friend int_hp operator^(int_hp a, int_hp b);

        friend int_hp& operator+=(int_hp& a, const int_hp& b);
        friend int_hp& operator-=(int_hp& a, const int_hp& b);
        friend int_hp& operator*=(int_hp& a, const int_hp& b);
        friend int_hp& operator/=(int_hp& a, const int_hp& b);
        friend int_hp& operator%=(int_hp& a, const int_hp& b);
        friend int_hp& operator^=(int_hp& a, const int_hp& b);

        friend int_hp abs(int_hp a);
    };

} // namespace hacker_wang

#endif
