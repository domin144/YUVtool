/* 
 * Copyright 2015, 2016 Dominik Wójt
 * 
 * This file is part of YUVtool.
 * 
 * YUVtool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * YUVtool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with YUVtool.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>

namespace YUV_tool {
/*----------------------------------------------------------------------------*/
using Byte = std::uint8_t;
using Index = std::int64_t;
const Index bits_in_byte = 8;
using Length = std::int64_t;
/*----------------------------------------------------------------------------*/
template<typename T>
Index size(const std::vector<T> &v)
{
    return static_cast<Index>(v.size());
}
/*----------------------------------------------------------------------------*/
enum Rgba_component
{
    Rgba_component_red,
    Rgba_component_green,
    Rgba_component_blue,
    Rgba_component_alpha,
    Rgba_component_count,
    Rgba_component_rgb_count = Rgba_component_alpha
};
/*----------------------------------------------------------------------------*/
template <typename TNumber>
TNumber round_down(const TNumber value, const TNumber divisor)
{
    return value - value % divisor;
}
/*----------------------------------------------------------------------------*/
template <typename TNumber>
TNumber round_up(const TNumber value, const TNumber divisor)
{
    return round_down(value + divisor - static_cast<TNumber>(1), divisor);
}
/*----------------------------------------------------------------------------*/
template<typename TNumber>
TNumber power(const TNumber base, TNumber exponent)
{
    TNumber result = 1;
    TNumber current_power = base;
    while(exponent)
    {
        if(exponent & 1)
            result *= current_power;
        current_power *= current_power;
        exponent >>= 1;
    }
    return result;
}
/*----------------------------------------------------------------------------*/
template<Index x, Index n>
struct Const_power
{
    static constexpr Index value =
            (
                Const_power<x, n - 1>::value
                <= std::numeric_limits<Index>::max() / x)
            ? Const_power<x, n - 1>::value * x
            : -1;
};
/*----------------------------------------------------------------------------*/
template<Index x>
struct Const_power<x, 0>
{
    static constexpr Index value = 1;
};
/*----------------------------------------------------------------------------*/
template<>
struct Const_power<0, 0>;
/*----------------------------------------------------------------------------*/
static_assert(Const_power<1, 0>::value == 1, "Const_power failure");
static_assert(Const_power<5, 0>::value == 1, "Const_power failure");
static_assert(Const_power<5, 2>::value == 25, "Const_power failure");
static_assert(Const_power<3, 3>::value == 27, "Const_power failure");
/*----------------------------------------------------------------------------*/
template<Index base, typename TNumber>
TNumber cached_power(TNumber exponent)
{
    constexpr TNumber b = base;
    constexpr TNumber cache[] =
    {
        Const_power<b, 0>::value,
        Const_power<b, 1>::value,
        Const_power<b, 2>::value,
        Const_power<b, 3>::value,
        Const_power<b, 4>::value,
        Const_power<b, 5>::value,
        Const_power<b, 6>::value,
        Const_power<b, 7>::value,
        Const_power<b, 8>::value,
        Const_power<b, 9>::value,
        Const_power<b, 10>::value,
        Const_power<b, 11>::value,
        Const_power<b, 12>::value,
        Const_power<b, 13>::value,
        Const_power<b, 14>::value,
        Const_power<b, 15>::value,
        Const_power<b, 16>::value
    };

    const TNumber cache_size = sizeof(cache) / sizeof(cache[0]);
    if(exponent < cache_size && cache[exponent] != -1)
        return cache[exponent];
    else
        return power(b, exponent);
}
/*----------------------------------------------------------------------------*/
template<typename TRatio>
struct Ratio_to_double
{
    using Ratio = TRatio;
    static constexpr double value = double(Ratio::num) / Ratio::den;
};
/*----------------------------------------------------------------------------*/
/* This class is designed as helper for making range based loops over
 * continuous ranges of e.g. enums or integers. */
template<typename TValue>
class Value_range
{
public:
    using Value = TValue;

    class Iterator
    {
    public:
        friend class Value_range;

    private:
        Value m_value;

    private:
        /* constructor is to be accesible only to Value_range class */
        constexpr Iterator(const Value value) : m_value(value) { }

    public:
        Value operator*() const
        {
            return m_value;
        }

        Iterator &operator++()
        {
            m_value = static_cast<Value>(static_cast<int>(m_value) + 1);
            return *this;
        }

        Iterator operator++(int)
        {
            const Iterator result = *this;
            ++(*this);
            return result;
        }

        bool operator!=(const Iterator &rhs) const
        {
            return m_value != rhs.m_value;
        }
    };

private:
    Iterator m_begin;
    Iterator m_end;

public:
    constexpr Value_range(const Value begin, const Value end) :
        m_begin(begin),
        m_end(end)
    { }

    Iterator begin() const
    {
        return m_begin;
    }

    Iterator end() const
    {
        return m_end;
    }
};
/*----------------------------------------------------------------------------*/
template <typename Value>
constexpr Value_range<Value> make_value_range(
    const Value begin, const Value end)
{
    return Value_range<Value>(begin, end);
}
/*----------------------------------------------------------------------------*/
template <typename Value>
constexpr Value_range<Value> make_value_range(const Value end)
{
    return Value_range<Value>(Value {}, end);
}
/*----------------------------------------------------------------------------*/
template <typename Value>
constexpr Value_range<Value> make_inclusive_value_range(
    const Value begin, const Value end)
{
    return Value_range<Value>(begin, end + 1);
}
/*----------------------------------------------------------------------------*/
template<typename Value>
bool is_in_range(const Value &x, const Value_range<Value> &range)
{
    return *range.begin() <= x && x < *range.end();
}
/*----------------------------------------------------------------------------*/
/* This is a helper class for making range based loops over a range described by
 * a pair of iterators. */
template<typename TIterator>
class Range
{
public:
    using Iterator = TIterator;

private:
    Iterator m_begin;
    Iterator m_end;

public:
    Range(const Iterator begin, const Iterator end) :
        m_begin(begin),
        m_end(end)
    { }

    Iterator begin() const
    {
        return m_begin;
    }

    Iterator end() const
    {
        return m_end;
    }
};
/*----------------------------------------------------------------------------*/
template<typename Iterator>
Range<Iterator> make_range(const Iterator begin, const Iterator end)
{
    return Range<Iterator>(begin, end);
}
/*----------------------------------------------------------------------------*/
class Timer
{
private:
    using Clock = std::chrono::high_resolution_clock;

private:
    Clock::time_point m_start_point;
    std::string m_message;

public:
    Timer(const std::string &message) :
        m_start_point(Clock::now()),
        m_message(message)
    { }

    ~Timer()
    {
        const Clock::duration time_elapsed = Clock::now() - m_start_point;
        const double time_in_ms =
                time_elapsed.count()
                * Ratio_to_double<
                    std::ratio_divide<
                        Clock::duration::period,
                        std::milli> >::value;
        std::cerr
                << "Timer \"" << m_message << "\" : "
                << time_in_ms << " ms\n";
    }
};
/*----------------------------------------------------------------------------*/
} /* YUV_tool */

#endif // UTILS_H
