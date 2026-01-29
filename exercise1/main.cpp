// Student1: Hai Chu
// Student2:
// How AI was used in the exercise:

#include <limits.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

// Precompute the scaling factors used for printing the fixed point numbers
constexpr std::array<uint64_t, 63> computeScalingFactors() {
    std::array<uint64_t, 63> factors = {};
    uint64_t factor = 10;
    for (uint8_t i = 0; i <= 62; ++i) {
        if (i == 0) {
            factors[i] = 1;  // 2^0 = 1
        } else {
            factors[i] = (factor / (1llu << i));
            factor *= 10;
        }
    }
    return factors;
}

// Precompute the scaling factors
constexpr std::array<uint64_t, 63> scalingFactors = computeScalingFactors();

template <typename T>
class FixedPoint {
   public:
    // With this constructor you can set the integer part and decimal part separately,
    // along with the number of decimal bits.
    // For example FixedPoint(3, 1, 2) = 3.25
    FixedPoint(T value, T remainder, int decimal_digits) : FixedPoint<T>(value * (1 << decimal_digits) + remainder, decimal_digits) {
    }
    // This assumes that the value is already a fractional point value.
    // FixedPoint(13, 2) = 3.25
    FixedPoint(T value, int decimal_digits)
        : m_value(value), m_decimal_digits(decimal_digits), m_print_modifier(scalingFactors[decimal_digits]) {}

    void print_value() const {
        std::cout << to_string() << std::endl;
    }

    [[nodiscard]] std::string to_string() const {
        T reverse = (1 << m_decimal_digits) - 1;
        auto absolute_value = m_value < 0 ? -m_value : m_value;
        auto integer_part = absolute_value >> m_decimal_digits;
        // multiply the fractional part by the print modifier to get the correct value of the decimal part
        auto fractional_part = ((absolute_value & reverse) * m_print_modifier);
        std::stringstream ss;
        ss << (absolute_value != m_value ? "-" : "") << integer_part << "." << std::setfill('0') << std::setw(m_decimal_digits) << fractional_part;
        auto str = ss.str();
        // Remove any trailing zeros
        str.erase(std::find_if(str.rbegin(), str.rend(), [](char ch) {
                      return ch != '0';
                  }).base(),
                  str.end());
        // Remove trailing dot if it's the last character
        if (str.back() == '.') {
            str.pop_back();
        }
        return str;
    }

    FixedPoint operator+(const FixedPoint& other) const {
        if ((-2 >> 1) < 0) {  // Arithmetic Shift Right (ASR)
            // Decide the smaller and larger values
            int first = m_decimal_digits <= other.m_decimal_digits ? m_decimal_digits : other.m_decimal_digits;
            int second = m_decimal_digits > other.m_decimal_digits ? m_decimal_digits : other.m_decimal_digits;
            int value_first = m_decimal_digits <= other.m_decimal_digits ? m_value : other.m_value;
            int value_second = m_decimal_digits > other.m_decimal_digits ? m_value : other.m_value;
            int bit_count_first = m_decimal_digits <= other.m_decimal_digits ? bit_count : other.bit_count;
            T max = 1 << (bit_count_first - 2);  // second most significant bit for type T

            // In case the two values have different decimal digits,
            // the smaller will be shifted right,
            // while the larger be shifted left
            // until the decimal digits of the two are equal
            while (first != second && !(value_first & max)) {
                // !(value_first & max) decide if the last non-sign bit of value_first
                // is 0 or 1. If 1 then it is not suitable for left shift anymore
                first++;
                value_first <<= 1;
            }
            while (first != second && !(value_second & 1)) {
                // !(value_second & 1) decide if the first bit of value_second
                // is 0 or 1. If 1 then it is not suitable for left shift anymore
                second--;
                value_second >>= 1;
            }

            if (first != second) {
                // cannot shift either value_first left or value_second right
                // anymore without changing they original values
                // TODO
                std::cout << "Cannot balance decimal digits of two numbers" << std::endl;
                return FixedPoint(0, 0);
            }

            // The actual implementation, assuming two numbers has the same decimal digits
            int new_m_value = value_first + value_second;
            return FixedPoint(new_m_value, first);
        }
        return FixedPoint(0, 0);
    }
    FixedPoint operator-(const FixedPoint& other) const {
        return FixedPoint(0, 0);
    }
    FixedPoint operator*(const FixedPoint& other) const {
        return FixedPoint(0, 0);
    }
    FixedPoint operator/(const FixedPoint& other) const {
        return FixedPoint(0, 0);
    }

    FixedPoint operator<<(int shift) const {
        int new_value = m_value << shift;
        T max = 1 << (bit_count - 2);  // second most significant bit for type T
        int new_decimal_digits = m_decimal_digits + shift <= max ? m_decimal_digits + shift : max;
        return FixedPoint(new_value, new_decimal_digits);
    }

    FixedPoint operator>>(int shift) const {
        int new_value = (m_value + (1 << shift - 1)) >> shift;
        int new_decimal_digits = m_decimal_digits >= shift ? m_decimal_digits - shift : 0;
        return FixedPoint(new_value, new_decimal_digits);
    }

    [[nodiscard]] FixedPoint change_precision(int new_decimal_digits) const {
        if (new_decimal_digits > m_decimal_digits) {
            // shift left
            return FixedPoint(m_value, m_decimal_digits) << (new_decimal_digits - m_decimal_digits);
        } else {
            // shift right
            return FixedPoint(m_value, m_decimal_digits) >> (m_decimal_digits - new_decimal_digits);
        }
        return FixedPoint(0, 0);
    }

   private:
    T m_value = 0;
    int m_decimal_digits = 0;
    int bit_count = sizeof(T) * CHAR_BIT;
    uint64_t m_print_modifier = 0;
};

template <typename T>
void compare(const FixedPoint<T>& f, const std::string& s) {
    if (f.to_string() != s) {
        std::cout << "Error: " << f.to_string() << " != " << s << std::endl;
    }
}

void test() {
    FixedPoint<int32_t> a = FixedPoint<int32_t>(0, 1, 2);  // 0.25
    FixedPoint<int32_t> b = FixedPoint<int32_t>(0, 1, 3);  // 0.125
    FixedPoint<int32_t> c = a + b;
    compare(c, "0.375");
    c = b + a;
    compare(c, "0.375");
    c = a - b;
    compare(c, "0.125");
    c = b - a;
    compare(c, "-0.125");
    c = a * b;
    compare(c, "0.03125");
    c = b * a;
    compare(c, "0.03125");
    /* Uncomment if doing the extra part
    c = a / b;
    compare(c, "2");
    c = b / a;
    compare(c, "0.5");
    */

    c = FixedPoint<int32_t>(0, 1, 1);  // 0.5
    c = c.change_precision(5);
    compare(c, "0.5");
    c = c >> 5;  // After this shift there is no fractional part
    compare(c, "1");
    c = FixedPoint<int32_t>(0, 1, 2);  // 0.25
    c = c >> 1;
    compare(c, "0.5");
    c = FixedPoint<int32_t>(0, 1, 1);  // 0.5
    c = c.change_precision(0);         // No fractional part, it should be rounded to the nearest integer
    compare(c, "1");

    b = FixedPoint<int32_t>(0, -1, 3);  // -0.125

    c = a + b;
    compare(c, "0.125");
    c = b + a;
    compare(c, "0.125");
    c = a - b;
    compare(c, "0.375");
    c = b - a;
    compare(c, "-0.375");
    c = a * b;
    compare(c, "-0.03125");
    c = b * a;
    compare(c, "-0.03125");

    c = FixedPoint(16, 5);
    c = c >> 4;
    compare(c, "0.5");

    /* Uncomment if doing the extra part
    c = a / b;
    compare(c, "-2");
    c = b / a;
    compare(c, "-0.5");
    */

    // Add your own tests here
}

int main() {
    test();
    return 0;
}
