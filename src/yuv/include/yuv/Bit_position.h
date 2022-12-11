/* 
 * Copyright by Dominik Wójt 2015
 * 
 * This file is part of YUVtool.
 * 
 * YUVtool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef BIT_POSITION_H
#define BIT_POSITION_H

#include <yuv/utils.h>
#include <ostream>

namespace YUV_tool {

class Bit_position
{
private:
    typedef Index Position_type;
    Position_type m_position;
public:
    constexpr Bit_position(Position_type position = 0) :
        m_position(position)
    { }
//------------------------------------------------------------------------------
    constexpr Bit_position(const int bytes, const int bits) :
        m_position(bytes * bits_in_byte + bits)
    { }
//------------------------------------------------------------------------------
    constexpr Position_type get_position() const
    {
        return m_position;
    }
//------------------------------------------------------------------------------
    constexpr void set_position(const Position_type position)
    {
        m_position = position;
    }
//------------------------------------------------------------------------------
    constexpr Position_type get_bytes() const
    {
        return m_position / bits_in_byte;
    }
//------------------------------------------------------------------------------
    constexpr int get_bits() const
    {
        return m_position % bits_in_byte;
    }
//------------------------------------------------------------------------------
    constexpr void set_position(const Position_type byte, const int bit)
    {
        set_position(byte * bits_in_byte + bit);
    }
};
//------------------------------------------------------------------------------
inline constexpr Bit_position& operator+=(
    Bit_position& a, const Bit_position& b)
{
    a.set_position(a.get_position() + b.get_position());
    return a;
}
//------------------------------------------------------------------------------
inline constexpr Bit_position& operator-=(
    Bit_position& a, const Bit_position& b)
{
    a.set_position(a.get_position() - b.get_position());
    return a;
}
//------------------------------------------------------------------------------
inline constexpr Bit_position operator+(
    const Bit_position& a, const Bit_position& b)
{
    Bit_position result = a;
    result += b;
    return result;
}
//------------------------------------------------------------------------------
inline constexpr Bit_position operator-(
    const Bit_position& a, const Bit_position& b)
{
    Bit_position result = a;
    result -= b;
    return result;
}
//------------------------------------------------------------------------------
inline constexpr Bit_position &operator*=(Bit_position &a, const int factor)
{
    a.set_position(a.get_position()*factor);
    return a;
}
//------------------------------------------------------------------------------
inline constexpr Bit_position operator*(const Bit_position &a, const int factor)
{
    Bit_position result = a;
    result *= factor;
    return result;
}
//------------------------------------------------------------------------------
inline constexpr Bit_position operator*(const int factor, const Bit_position &a)
{
    return a * factor;
}
//------------------------------------------------------------------------------
inline constexpr bool operator==(const Bit_position &a, const Bit_position &b)
{
    return a.get_position() == b.get_position();
}
//------------------------------------------------------------------------------
inline constexpr int operator/(const Bit_position &a, const Bit_position &b)
{
    return a.get_position()/b.get_position();
}
//------------------------------------------------------------------------------
inline constexpr bool operator<(const Bit_position &a, const Bit_position &b)
{
    return a.get_position() < b.get_position();
}
//------------------------------------------------------------------------------
inline constexpr bool operator>(const Bit_position &a, const Bit_position &b)
{
    return a.get_position() > b.get_position();
}
//------------------------------------------------------------------------------
inline constexpr bool operator<=(const Bit_position &a, const Bit_position &b)
{
    return a.get_position() <= b.get_position();
}
//------------------------------------------------------------------------------
inline constexpr bool operator>=(const Bit_position &a, const Bit_position &b)
{
    return a.get_position() >= b.get_position();
}
//------------------------------------------------------------------------------
inline std::ostream &operator<<(std::ostream &stream, const Bit_position &a)
{
    stream << a.get_bytes() << "bytes, " << a.get_bits() << "bits";
    return stream;
}
//------------------------------------------------------------------------------
inline constexpr Bit_position &operator++(Bit_position &a)
{
    a.set_position(a.get_position() + 1);
    return a;
}
//------------------------------------------------------------------------------
inline Bit_position operator++(Bit_position &a, const int)
{
    Bit_position tmp(a);
    ++a;
    return tmp;
}
//------------------------------------------------------------------------------
inline Bit_position &operator--(Bit_position &a)
{
    a.set_position(a.get_position() - 1);
    return a;
}
//------------------------------------------------------------------------------
inline Bit_position operator--(Bit_position &a, const int)
{
    Bit_position tmp(a);
    --a;
    return tmp;
}

} /* namespace YUV_tool */

#endif // BIT_POSITION_H
