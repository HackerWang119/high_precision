#pragma once
#ifndef HIGH_PRECISION_H
#define HIGH_PRECISION_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <utility>

namespace hacker_wang {

    class int_hp;

    std::istream& operator>>(std::istream& is, int_hp& hp);
    std::ostream& operator<<(std::ostream& os, const int_hp& hp);
    std::ifstream& operator>>(std::ifstream& is, int_hp& hp);
    std::ofstream& operator<<(std::ofstream& os, const int_hp& hp);

    bool operator==(const int_hp& a, const int_hp& b);
    bool operator!=(const int_hp& a, const int_hp& b);
    bool operator>(const int_hp& a, const int_hp& b);
    bool operator<(const int_hp& a, const int_hp& b);
    bool operator>=(const int_hp& a, const int_hp& b);
    bool operator<=(const int_hp& a, const int_hp& b);

    int_hp operator+(int_hp a, int_hp b);
    int_hp operator-(int_hp a);
    int_hp operator-(int_hp a, int_hp b);
    int_hp operator*(const int_hp& a, const int_hp& b);
    int_hp operator/(int_hp a, int_hp b);
    int_hp operator%(const int_hp& a, const int_hp& b);
    int_hp operator^(int_hp a, int_hp b);

    int_hp& operator+=(int_hp& a, const int_hp& b);
    int_hp& operator-=(int_hp& a, const int_hp& b);
    int_hp& operator*=(int_hp& a, const int_hp& b);
    int_hp& operator/=(int_hp& a, const int_hp& b);
    int_hp& operator%=(int_hp& a, const int_hp& b);
    int_hp& operator^=(int_hp& a, const int_hp& b);

    int_hp abs(int_hp a);

    class int_hp {
    private:
        static constexpr uint32_t BASE = 1000000000;
        static constexpr size_t BASE_DIGITS = 9;

        std::vector<uint32_t> digits;
        bool negative = false;

        void trim();
        void normalize();

        static int compare_abs(const int_hp& a, const int_hp& b);

        static int_hp add_abs(const int_hp& a, const int_hp& b);
        static int_hp sub_abs(const int_hp& a, const int_hp& b);

        static int_hp naive_mul(const int_hp& a, const int_hp& b);
        static int_hp karatsuba_mul(const int_hp& a, const int_hp& b);
        int_hp shift_left(size_t shift) const;

        static int_hp div_abs(const int_hp& a, const int_hp& b);

        static void parse_string(const std::string& s, int_hp& hp);

    public:
        int_hp();
        int_hp(long long n);

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
        friend std::ifstream& operator>>(std::ifstream& is, int_hp& hp);
        friend std::ofstream& operator<<(std::ofstream& os, const int_hp& hp);

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

}

#endif
