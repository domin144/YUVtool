/*
 * Copyright 2022 Dominik Wójt
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

/* This file tests correctness of usage of Boost GIL library rather than
 * YUV_tool code. */

#include <boost/gil.hpp>
#include <boost/gil/extension/io/bmp.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <cstdint>
#include <gtest/gtest.h>

namespace gil = boost::gil;

TEST(gil_test, interleaved_view)
{
    constexpr std::uint32_t size_x = 2, size_y = 2, components_count = 3;
    const std::array<std::uint8_t, size_x* size_y* components_count> data = {
        0x10, 0x80, 0x90, 0x20, 0x80, 0x90, 0x30, 0x80, 0x90, 0x40, 0x80, 0x90};
    const gil::rgb8c_view_t view = gil::interleaved_view(
        size_x,
        size_y,
        gil::rgb8c_view_t::x_iterator(data.data()),
        size_x * components_count);
    EXPECT_EQ(gil::get_color(*view.at(0, 0), gil::red_t()), 0x10);
    EXPECT_EQ(gil::get_color(*view.at(0, 0), gil::green_t()), 0x80);
    EXPECT_EQ(gil::get_color(*view.at(0, 0), gil::blue_t()), 0x90);
    EXPECT_EQ(gil::get_color(*view.at(1, 0), gil::red_t()), 0x20);
    EXPECT_EQ(gil::get_color(*view.at(1, 0), gil::green_t()), 0x80);
    EXPECT_EQ(gil::get_color(*view.at(1, 0), gil::blue_t()), 0x90);
    EXPECT_EQ(gil::get_color(*view.at(0, 1), gil::red_t()), 0x30);
    EXPECT_EQ(gil::get_color(*view.at(0, 1), gil::green_t()), 0x80);
    EXPECT_EQ(gil::get_color(*view.at(0, 1), gil::blue_t()), 0x90);
    EXPECT_EQ(gil::get_color(*view.at(1, 1), gil::red_t()), 0x40);
    EXPECT_EQ(gil::get_color(*view.at(1, 1), gil::green_t()), 0x80);
    EXPECT_EQ(gil::get_color(*view.at(1, 1), gil::blue_t()), 0x90);

//    gil::write_view("gil_test.png", view, gil::png_tag());
//    gil::write_view("gil_test.bmp", view, gil::bmp_tag());
}
