#include "high_precision.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace hacker_wang {

    constexpr uint32_t int_hp::BASE;
    constexpr size_t int_hp::BASE_DIGITS;

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

    int_hp::int_hp() : digits(1, 0), negative(false) {}

    int_hp::int_hp(long long n) {
        if (n < 0) {
            negative = true;
            n = -n;
        } else {
            negative = false;
        }
        if (n == 0) {
            digits.push_back(0);
        } else {
            while (n > 0) {
                digits.push_back(n % BASE);
                n /= BASE;
            }
        }
    }

    int_hp& int_hp::operator++() {
        *this += 1;
        return *this;
    }
    int_hp int_hp::operator++(int) {
        int_hp temp = *this;
        ++(*this);
        return temp;
    }
    int_hp& int_hp::operator--() {
        *this -= 1;
        return *this;
    }
    int_hp int_hp::operator--(int) {
        int_hp temp = *this;
        --(*this);
        return temp;
    }

    int_hp abs(int_hp a) {
        a.negative = false;
        return a;
    }

    int_hp int_hp::add_abs(const int_hp& a, const int_hp& b) {
        int_hp result;
        result.digits.clear();
        size_t n = std::max(a.digits.size(), b.digits.size());
        result.digits.reserve(n + 1);
        uint64_t carry = 0;
        for (size_t i = 0; i < n || carry; ++i) {
            uint64_t sum = carry;
            if (i < a.digits.size()) sum += a.digits[i];
            if (i < b.digits.size()) sum += b.digits[i];
            result.digits.push_back(static_cast<uint32_t>(sum % BASE));
            carry = sum / BASE;
        }
        result.negative = false;
        return result;
    }

    int_hp int_hp::sub_abs(const int_hp& a, const int_hp& b) {
        int_hp result;
        result.digits.clear();
        result.digits.reserve(a.digits.size());
        int64_t borrow = 0;
        for (size_t i = 0; i < a.digits.size(); ++i) {
            int64_t diff = static_cast<int64_t>(a.digits[i]) - borrow;
            if (i < b.digits.size()) diff -= b.digits[i];
            if (diff < 0) {
                diff += BASE;
                borrow = 1;
            } else {
                borrow = 0;
            }
            result.digits.push_back(static_cast<uint32_t>(diff));
        }
        result.negative = false;
        result.trim();
        return result;
    }

    int_hp int_hp::naive_mul(const int_hp& a, const int_hp& b) {
        int_hp result;
        result.digits.assign(a.digits.size() + b.digits.size() + 1, 0);
        for (size_t i = 0; i < a.digits.size(); ++i) {
            uint64_t carry = 0;
            for (size_t j = 0; j < b.digits.size() || carry; ++j) {
                uint64_t cur = result.digits[i + j] +
                               static_cast<uint64_t>(a.digits[i]) * (j < b.digits.size() ? b.digits[j] : 0) +
                               carry;
                result.digits[i + j] = static_cast<uint32_t>(cur % BASE);
                carry = cur / BASE;
            }
        }
        result.trim();
        return result;
    }

    static const size_t KARATSUBA_THRESHOLD = 32;

    int_hp int_hp::karatsuba_mul(const int_hp& a, const int_hp& b) {
        size_t n = std::max(a.digits.size(), b.digits.size());
        if (n <= KARATSUBA_THRESHOLD)
            return naive_mul(a, b);

        size_t m = n / 2;
        int_hp a0, a1, b0, b1;
        a0.digits.assign(a.digits.begin(), a.digits.begin() + std::min(m, a.digits.size()));
        if (a.digits.size() > m)
            a1.digits.assign(a.digits.begin() + m, a.digits.end());
        b0.digits.assign(b.digits.begin(), b.digits.begin() + std::min(m, b.digits.size()));
        if (b.digits.size() > m)
            b1.digits.assign(b.digits.begin() + m, b.digits.end());
        a0.trim(); a1.trim(); b0.trim(); b1.trim();

        int_hp z0 = karatsuba_mul(a0, b0);
        int_hp z2 = karatsuba_mul(a1, b1);

        int_hp sum_a = add_abs(a0, a1);
        int_hp sum_b = add_abs(b0, b1);
        int_hp z1 = karatsuba_mul(sum_a, sum_b);
        z1 = sub_abs(z1, z0);
        z1 = sub_abs(z1, z2);

        int_hp result = z0;
        result = add_abs(result, z1.shift_left(m));
        result = add_abs(result, z2.shift_left(2 * m));
        result.trim();
        return result;
    }

    int_hp int_hp::shift_left(size_t shift) const {
        if (digits.size() == 1 && digits[0] == 0)
            return int_hp();
        int_hp result = *this;
        result.digits.insert(result.digits.begin(), shift, 0);
        return result;
    }

    int_hp operator*(const int_hp& a, const int_hp& b) {
        if (a.digits.size() == 1 && a.digits[0] == 0) return int_hp();
        if (b.digits.size() == 1 && b.digits[0] == 0) return int_hp();
        int_hp result = int_hp::karatsuba_mul(a, b);
        result.negative = (a.negative != b.negative);
        result.normalize();
        return result;
    }

    int_hp int_hp::div_abs(const int_hp& a, const int_hp& b) {
        if (a.digits.size() < b.digits.size()) return int_hp();

        size_t m = b.digits.size();
        size_t n = a.digits.size();
        size_t q_size = n - m + 1;
        int_hp remainder;
        int_hp quotient;
        quotient.digits.resize(q_size, 0);

        size_t processed = 0;
        for (size_t i = a.digits.size(); i > 0; --i) {
            size_t idx = i - 1;
            ++processed;

            remainder.digits.insert(remainder.digits.begin(), 0);

            uint64_t sum = static_cast<uint64_t>(remainder.digits[0]) + a.digits[idx];
            remainder.digits[0] = static_cast<uint32_t>(sum % BASE);
            uint64_t carry = sum / BASE;
            if (carry) {
                size_t pos = 1;
                while (carry && pos < remainder.digits.size()) {
                    uint64_t val = static_cast<uint64_t>(remainder.digits[pos]) + carry;
                    remainder.digits[pos] = static_cast<uint32_t>(val % BASE);
                    carry = val / BASE;
                    ++pos;
                }
                if (carry) remainder.digits.push_back(static_cast<uint32_t>(carry));
            }
            remainder.trim();

            uint32_t q = 0;
            uint32_t low = 0, high = BASE - 1;
            while (low <= high) {
                uint32_t mid = low + (high - low) / 2;
                int_hp prod = b * int_hp(mid);
                if (int_hp::compare_abs(prod, remainder) <= 0) {
                    q = mid;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }

            if (processed >= m) {
                size_t q_pos = q_size - 1 - (processed - m);
                quotient.digits[q_pos] = q;
            }

            int_hp prod = b * int_hp(q);
            remainder = int_hp::sub_abs(remainder, prod);
        }

        quotient.trim();
        return quotient;
    }

    int_hp operator/(int_hp a, int_hp b) {
        if (b.digits.size() == 1 && b.digits[0] == 0)
            throw std::domain_error("Division by zero");
        if (a.digits.size() == 1 && a.digits[0] == 0) return int_hp();
        bool neg = (a.negative != b.negative);
        a.negative = false;
        b.negative = false;
        int_hp result = int_hp::div_abs(a, b);
        result.negative = neg;
        result.normalize();
        return result;
    }

    int_hp operator%(const int_hp& a, const int_hp& b) {
        return a - (a / b) * b;
    }

    // 修正后的 operator^
    int_hp operator^(int_hp a, int_hp b) {
        if (b.negative)
            throw std::runtime_error("Negative exponent not supported");
        if (b.digits.size() == 1 && b.digits[0] == 0)
            return int_hp(1);

        bool base_neg = a.negative;
        a.negative = false;

        // 保存指数奇偶性（仅当指数不为0时有效）
        bool exp_odd = (b.digits[0] & 1);

        int_hp result(1);
        while (!(b.digits.size() == 1 && b.digits[0] == 0)) {
            if (b.digits[0] & 1)
                result = result * a;
            a = a * a;
            uint64_t carry = 0;
            for (size_t i = b.digits.size(); i > 0; --i) {
                size_t idx = i - 1;
                uint64_t cur = carry * int_hp::BASE + b.digits[idx];
                b.digits[idx] = cur / 2;
                carry = cur % 2;
            }
            b.trim();
        }

        if (base_neg && exp_odd)
            result.negative = true;
        return result;
    }

    int_hp operator+(int_hp a, int_hp b) {
        if (a.negative == b.negative) {
            int_hp result = int_hp::add_abs(a, b);
            result.negative = a.negative;
            return result;
        } else {
            if (a.negative) {
                int_hp abs_a = a;
                abs_a.negative = false;
                int_hp abs_b = b;
                if (int_hp::compare_abs(abs_b, abs_a) >= 0)
                    return int_hp::sub_abs(abs_b, abs_a);
                else {
                    int_hp result = int_hp::sub_abs(abs_a, abs_b);
                    result.negative = true;
                    return result;
                }
            } else {
                int_hp abs_a = a;
                int_hp abs_b = b;
                abs_b.negative = false;
                if (int_hp::compare_abs(abs_a, abs_b) >= 0)
                    return int_hp::sub_abs(abs_a, abs_b);
                else {
                    int_hp result = int_hp::sub_abs(abs_b, abs_a);
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

    bool operator==(const int_hp& a, const int_hp& b) {
        return a.negative == b.negative && a.digits == b.digits;
    }
    bool operator!=(const int_hp& a, const int_hp& b) {
        return !(a == b);
    }
    bool operator>(const int_hp& a, const int_hp& b) {
        if (a.negative != b.negative) return b.negative;
        int cmp = int_hp::compare_abs(a, b);
        if (a.negative) return cmp < 0;
        else return cmp > 0;
    }
    bool operator<(const int_hp& a, const int_hp& b) {
        return b > a;
    }
    bool operator>=(const int_hp& a, const int_hp& b) {
        return !(a < b);
    }
    bool operator<=(const int_hp& a, const int_hp& b) {
        return !(a > b);
    }

    int_hp& operator+=(int_hp& a, const int_hp& b) {
        a = a + b;
        return a;
    }
    int_hp& operator-=(int_hp& a, const int_hp& b) {
        a = a - b;
        return a;
    }
    int_hp& operator*=(int_hp& a, const int_hp& b) {
        a = a * b;
        return a;
    }
    int_hp& operator/=(int_hp& a, const int_hp& b) {
        a = a / b;
        return a;
    }
    int_hp& operator%=(int_hp& a, const int_hp& b) {
        a = a % b;
        return a;
    }
    int_hp& operator^=(int_hp& a, const int_hp& b) {
        a = a ^ b;
        return a;
    }

    void int_hp::parse_string(const std::string& s, int_hp& hp) {
        size_t pos = 0;
        hp.negative = false;
        if (!s.empty() && (s[0] == '-' || s[0] == '+')) {
            if (s[0] == '-') hp.negative = true;
            pos = 1;
        }
        hp.digits.clear();
        size_t len = s.size();
        for (size_t i = len; i > pos; ) {
            size_t start = (i > pos + int_hp::BASE_DIGITS) ? i - int_hp::BASE_DIGITS : pos;
            std::string part = s.substr(start, i - start);
            uint32_t val = static_cast<uint32_t>(std::stoul(part));
            hp.digits.push_back(val);
            i = start;
        }
        if (hp.digits.empty()) hp.digits.push_back(0);
        hp.normalize();
    }

    std::istream& operator>>(std::istream& is, int_hp& hp) {
        std::string s;
        is >> s;
        int_hp::parse_string(s, hp);
        return is;
    }
    std::ifstream& operator>>(std::ifstream& is, int_hp& hp) {
        std::string s;
        is >> s;
        int_hp::parse_string(s, hp);
        return is;
    }

    std::ostream& operator<<(std::ostream& os, const int_hp& hp) {
        if (hp.negative) os << '-';
        if (hp.digits.empty()) { os << '0'; return os; }
        os << hp.digits.back();
        for (size_t i = hp.digits.size() - 1; i > 0; --i) {
            size_t idx = i - 1;
            std::string group = std::to_string(hp.digits[idx]);
            os << std::string(int_hp::BASE_DIGITS - group.length(), '0') << group;
        }
        return os;
    }
    std::ofstream& operator<<(std::ofstream& os, const int_hp& hp) {
        if (hp.negative) os << '-';
        if (hp.digits.empty()) { os << '0'; return os; }
        os << hp.digits.back();
        for (size_t i = hp.digits.size() - 1; i > 0; --i) {
            size_t idx = i - 1;
            std::string group = std::to_string(hp.digits[idx]);
            os << std::string(int_hp::BASE_DIGITS - group.length(), '0') << group;
        }
        return os;
    }

}
