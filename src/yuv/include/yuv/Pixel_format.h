/* 
 * Copyright 2015 Dominik Wójt
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
#ifndef PIXEL_FORMAT_H
#define PIXEL_FORMAT_H

#include <yuv/utils.h>
#include <yuv/Bit_position.h>
#include <yuv/Coordinates.h>
#include <yuv/saturable_fixed.h>

#include <vector>
#include <algorithm>

namespace YUV_tool {

struct Component
{
    /*
     * E'_X = K_XR * E'_R + K_XG * E'_G + K_XB * E'_B + K_XA * E'_A
     */
    std::array<saturable_fixed, Rgba_component_count> coeff;
    std::array<saturable_fixed, 2> valid_range;
    std::array<saturable_fixed, 2> coded_range;
};

struct Color_space
{
    std::vector<Component> components;
};

struct Entry
{
    Bit_position width;
};

struct Entry_row
{
    std::vector<Entry> entries;
};

struct Plane
{
    // one set of entries in plane corresponds to one macropixel
    std::vector<Entry_row> rows;
};

struct Component_coding
{
    Index plane_index;
    Index row_index;
    Index entry_index;
};

struct Coded_pixel
{
    std::vector<Component_coding> components;
};

struct Macropixel_coding
{
    std::vector<Coded_pixel> pixels;
    Vector<Unit::pixel> size;
};

// TODO: add support for big-endian storage
// TODO: add support for rounding up and down in case
//       picture size is not integer multiplicity of macropixel size
struct Pixel_format
{
    std::vector<Plane> planes;
    Color_space color_space;
    Macropixel_coding macropixel_coding;
};

inline bool operator==(const Component &a, const Component &b)
{
    return
            a.coeff == b.coeff
            && a.valid_range == b.valid_range
            && a.coded_range == b.coded_range;
}

inline bool operator==(const Color_space &a, const Color_space &b)
{
    return a.components == b.components;
}

inline bool operator==(const Entry &a, const Entry &b)
{
    return a.width == b.width;
}

inline bool operator==(const Entry_row &a, const Entry_row &b)
{
    return a.entries == b.entries;
}

inline bool operator==(const Plane &a, const Plane &b)
{
    return a.rows == b.rows;
}

inline bool operator==(const Component_coding &a, const Component_coding &b)
{
    return
            a.plane_index == b.plane_index
            && a.row_index == b.row_index
            && a.entry_index == b.entry_index;
}

inline bool operator==(const Coded_pixel &a, const Coded_pixel &b)
{
    return a.components == b.components;
}

inline bool operator==(const Macropixel_coding &a, const Macropixel_coding &b)
{
    return a.pixels == b.pixels && a.size == b.size;
}

inline bool operator==(const Pixel_format &a, const Pixel_format &b)
{
    return
            a.planes == b.planes
            && a.color_space == b.color_space
            && a.macropixel_coding == b.macropixel_coding;
}

// full range RGB

const Color_space RGB_color_space
{
    {
        { // R
            { 1, 0, 0, 0 },
            { 0, 1 },
            { 0, 1 }
        },
        { // G
            { 0, 1, 0, 0 },
            { 0, 1 },
            { 0, 1 }
        },
        { // B
            { 0, 0, 1, 0 },
            { 0, 1 },
            { 0, 1 }
        }
    }
};

// studio RGB

const Color_space sRGB_color_space
{
    {
        { // R
            { 1, 0, 0, 0 },
            { 0, 1 },
            { saturable_fixed(16) / 255, saturable_fixed(235) / 255 }
        },
        { // G
            { 0, 1, 0, 0 },
            { 0, 1 },
            { saturable_fixed(16) / 255, saturable_fixed(235) / 255 }
        },
        { // B
            { 0, 0, 1, 0 },
            { 0, 1 },
            { saturable_fixed(16) / 255, saturable_fixed(235) / 255 }
        }
    }
};

// ITU BT.601 standard
const saturable_fixed ITU601_K_R = saturable_fixed(299) / 1000;
const saturable_fixed ITU601_K_B = saturable_fixed(114) / 1000;

const Color_space ITU601_YCbCr_color_space
{
    {
        { // Y
            {
                ITU601_K_R,
                1 - ITU601_K_B - ITU601_K_R,
                ITU601_K_B,
                0
            },
            { 0, 1 },
            { saturable_fixed(16) / 255, saturable_fixed(235) / 255 }
        },
        { // Cb
            {
                -ITU601_K_R/(2*(1-ITU601_K_B)),
                -(1-ITU601_K_B-ITU601_K_R)/(2*(1-ITU601_K_B)),
                saturable_fixed(1) / 2,
                0
            },
            { -saturable_fixed(1) / 2, saturable_fixed(1) / 2 },
            { saturable_fixed(16) / 255, saturable_fixed(240) / 255 }
        },
        { // Cr
            {
                saturable_fixed(1) / 2,
                -(1-ITU601_K_B-ITU601_K_R)/(2*(1-ITU601_K_R)),
                -ITU601_K_B/(2*(1-ITU601_K_R)),
                0
            },
            { -saturable_fixed(1) / 2, saturable_fixed(1) / 2 },
            { saturable_fixed(16) / 255, saturable_fixed(240) / 255 }
        }
    }
};

// ITU BT.709 standard
const saturable_fixed ITU709_K_R = saturable_fixed(2126) / 10000;
const saturable_fixed ITU709_K_B = saturable_fixed(722) / 10000;

const Color_space ITU709_YCbCr_color_space
{
    {
        { // Y
            {
                ITU709_K_R,
                1 - ITU709_K_B - ITU709_K_R,
                ITU709_K_B,
                0
            },
            { 0, 1 },
            { saturable_fixed(16) / 255, saturable_fixed(235) / 255 }
        },
        { // Cb
            {
                -ITU709_K_R/(2*(1-ITU709_K_B)),
                -(1-ITU709_K_B-ITU709_K_R)/(2*(1-ITU709_K_B)),
                saturable_fixed(1) / 2,
                0
            },
            { -saturable_fixed(1) / 2, saturable_fixed(1) / 2 },
            { saturable_fixed(16) / 255, saturable_fixed(240) / 255 }
        },
        { // Cr
            {
                saturable_fixed(1) / 2,
                -(1-ITU709_K_B-ITU709_K_R)/(2*(1-ITU709_K_R)),
                -ITU709_K_B/(2*(1-ITU709_K_R)),
                0
            },
            { -saturable_fixed(1) / 2, saturable_fixed(1) / 2 },
            { saturable_fixed(16) / 255, saturable_fixed(240) / 255 }
        }
    }
};

// ITU BT.2020 standard
const saturable_fixed ITU2020_K_R = saturable_fixed(2627) / 10000;
const saturable_fixed ITU2020_K_B = saturable_fixed(593) / 10000;

const Color_space ITU2020_YCbCr_color_space
{
    {
        { // Y
            {
                ITU2020_K_R,
                1 - ITU2020_K_B - ITU2020_K_R,
                ITU2020_K_B,
                0
            },
            { 0, 1 },
            { saturable_fixed(16) / 255, saturable_fixed(235) / 255 }
        },
        { // Cb
            {
                -ITU2020_K_R/(2*(1-ITU2020_K_B)),
                -(1-ITU2020_K_B-ITU2020_K_R)/(2*(1-ITU2020_K_B)),
                saturable_fixed(1) / 2,
                0
            },
            { -saturable_fixed(1) / 2, saturable_fixed(1) / 2 },
            { saturable_fixed(16) / 255, saturable_fixed(240) / 255 }
        },
        { // Cr
            {
                saturable_fixed(1) / 2,
                -(1-ITU2020_K_B-ITU2020_K_R)/(2*(1-ITU2020_K_R)),
                -ITU2020_K_B/(2*(1-ITU2020_K_R)),
                0
            },
            { -saturable_fixed(1) / 2, saturable_fixed(1) / 2 },
            { saturable_fixed(16) / 255, saturable_fixed(240) / 255 }
        }
    }
};

const Pixel_format yuv_420p_8bit
{
    { // planes
        { // plane Y
            { // rows
                { { { 8 }, { 8 } } },
                { { { 8 }, { 8 } } }
            }
        },
        { // plane U
            { // rows
                { { { 8 } } }
            }
        },
        { // plane V
            { // rows
                { { { 8 } } }
            }
        },
    },
    ITU601_YCbCr_color_space,
    { // macropixel coding
        { // coded pixels
            { // top left
                { { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 } }
            },
            { // top right
                { { 0, 0, 1 }, { 1, 0, 0 }, { 2, 0, 0 } }
            },
            { // bottom left
                { { 0, 1, 0 }, { 1, 0, 0 }, { 2, 0, 0 } }
            },
            { // bottom right
                { { 0, 1, 1 }, { 1, 0, 0 }, { 2, 0, 0 } }
            }
        },
        { 2, 2 }
    }
};

const Pixel_format yuv_444p_8bit
{
    { // planes
        { // plane Y
            { // rows
                { { { 8 } } }
            }
        },
        { // plane U
            { // rows
                { { { 8 } } }
            }
        },
        { // plane V
            { // rows
                { { { 8 } } }
            }
        },
    },
    ITU601_YCbCr_color_space,
    { // macropixel coding
        { // coded pixels
            { // coded pixel
                { { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 } }
            }
        },
        { 1, 1 }
    }
};

const Pixel_format rgb_32bpp
{
    { // planes
        { // plane RGB
            { // rows
                { {// entries
                    { 8 }, { 8 }, { 8 }, { 8 }
                } }
            }
        }
    },
    RGB_color_space,
    { // macropixel coding
        { // coded pixels
            { // coded pixel
                { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0, 2 } }
            }
        },
        { 1, 1 }
    }
};

const Pixel_format rgb_24bpp
{
    { // planes
        { // plane RGB
            { // rows
                { {// entries
                    { 8 }, { 8 }, { 8 }
                } }
            }
        }
    },
    RGB_color_space,
    { // macropixel coding
        { // coded pixels
            { // coded pixel
                { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 0, 2 } }
            }
        },
        { 1, 1 }
    }
};
/*----------------------------------------------------------------------------*/
constexpr auto valid_planes_count_range =
    make_inclusive_value_range<Index>(1, 16);
constexpr auto valid_rows_in_plane_count_range =
    make_inclusive_value_range<Index>(1, 4);
constexpr auto valid_entries_in_row_count_range =
    make_inclusive_value_range<Index>(1, 4);
constexpr auto valid_bit_width_range =
    make_inclusive_value_range<Bit_position>(1, 32);
constexpr auto valid_components_count_range =
    make_inclusive_value_range<Index>(1, 4);
constexpr auto valid_macropixel_size_range =
    make_inclusive_value_range<Index>(1, 4);
/*----------------------------------------------------------------------------*/
class Precalculated_pixel_format
{
private:
    Pixel_format m_pixel_format;

    struct Entry_parameters
    {
        Bit_position m_offset;
        Coordinates<Unit::pixel, Reference_point::macropixel> m_sampling_point;
        Index m_component_index;
    };

    struct Entry_row_paramters
    {
        std::vector<Entry_parameters> m_entries;
        Bit_position m_bits_per_macropixel;
    };

    struct Plane_parameters
    {
        std::vector<Entry_row_paramters> m_rows;
        Bit_position m_bits_per_macropixel;
    };

    std::vector<Plane_parameters> m_planes;

    Bit_position m_bits_per_macropixel;
    bool m_is_expanded;

public:
    static std::optional<Precalculated_pixel_format> create(
        const Pixel_format& pixel_format);

    const Pixel_format &get_pixel_format() const
    {
        return m_pixel_format;
    }
    Index get_components_count() const
    {
        return m_pixel_format.color_space.components.size();
    }
    Index get_planes_count() const
    {
        return m_planes.size();
    }
    Index get_entry_rows_count_in_plane(const Index plane_index) const
    {
        return m_planes[plane_index].m_rows.size();
    }
    Index get_entry_count_in_row_in_plane(
            const Index plane_index,
            const Index row_index) const
    {
        return m_planes[plane_index].m_rows[row_index].m_entries.size();
    }
    Bit_position get_bits_per_macropixel() const
    {
        return m_bits_per_macropixel;
    }
    Bit_position get_bits_per_macropixel_in_plane(const Index plane_index) const
    {
        return m_planes[plane_index].m_bits_per_macropixel;
    }
    Bit_position get_bits_per_macropixel_in_row_in_plane(
            const Index plane_index,
            const Index row_index) const
    {
        return m_planes[plane_index].m_rows[row_index].m_bits_per_macropixel;
    }
    Bit_position get_bits_per_entry(
            const Index plane_index,
            const Index row_index,
            const Index entry_index) const
    {
        return m_pixel_format.planes[
                plane_index].rows[
                row_index].entries[
                entry_index].width;
    }
    Bit_position get_bits_per_entry(
            const Coordinates<Unit::pixel, Reference_point::macropixel>
                &pixel_in_macropixel,
            const Index component_index) const
    {
        const Component_coding &coding =
                m_pixel_format.macropixel_coding.pixels[
                get_pixel_coding_index(pixel_in_macropixel)].components[
                component_index];
        return
                m_pixel_format.planes[
                    coding.plane_index].rows[
                    coding.row_index].entries[
                    coding.entry_index].width;
    }
    Bit_position get_entry_offset_in_macropixel_in_row_in_plane(
            const Index plane_index,
            const Index row_index,
            const Index entry_index) const
    {
        return
                m_planes[
                plane_index].m_rows[
                row_index].m_entries[
                entry_index].m_offset;
    }
    Vector<Unit::pixel> get_macropixel_size() const
    {
        return m_pixel_format.macropixel_coding.size;
    }
    bool is_expanded() const
    {
        return m_is_expanded;
    }
    Coordinates<Unit::pixel, Reference_point::macropixel> get_sampling_point(
            const Index plane_index,
            const Index row_index,
            const Index entry_index) const
    {
        return m_planes[plane_index].m_rows[row_index].m_entries[
                entry_index].m_sampling_point;
    }
    Index get_sampled_component(
            const Index plane_index,
            const Index row_index,
            const Index entry_index) const
    {
        return m_planes[plane_index].m_rows[row_index].m_entries[
                entry_index].m_component_index;
    }
private:
    Index get_pixel_coding_index(
            const Coordinates<Unit::pixel, Reference_point::macropixel>
                &pixel_in_macropixel) const
    {
        return
                pixel_in_macropixel.y() * get_macropixel_size().x()
                + pixel_in_macropixel.x();
    }
};
/*----------------------------------------------------------------------------*/
constexpr auto valid_frame_dimension_range =
    make_inclusive_value_range<Index>(1, 64*1024);
/*----------------------------------------------------------------------------*/
class Precalculated_buffer_parameters : public Precalculated_pixel_format
{
public:
    static std::optional<Precalculated_buffer_parameters> create(
        const Pixel_format& format, const Vector<Unit::pixel>& resolution);

    const Vector<Unit::pixel> &get_resolution() const
    {
        return m_resolution;
    }
    Bit_position get_buffer_size() const
    {
        return m_buffer_size;
    }
    Bit_position get_plane_size(Index plane_index) const
    {
        return m_planes[plane_index].m_size;
    }
    Bit_position get_plane_offset(Index plane_index) const
    {
        return m_planes[plane_index].m_offset;
    }
    Bit_position get_macropixel_row_in_plane_size(Index plane_index) const
    {
        return m_planes[plane_index].m_size_per_row_of_macropixels;
    }
    Vector<Unit::macropixel> get_size_in_macropixels() const
    {
        return m_size_in_macropixels;
    }
    Bit_position get_entry_offset(
            const Coordinates<Unit::pixel, Reference_point::picture>
                &coordinates,
            const Index component_index) const
    {
        Coordinates<Unit::pixel, Reference_point::macropixel>
                pixel_in_macropixel;
        const Coordinates<Unit::macropixel, Reference_point::picture>
                macropixel_coordintes =
                    cast_to_macropixels(
                        coordinates,
                        get_macropixel_size(),
                        pixel_in_macropixel);
        const Macropixel_coding &macropixel_coding =
                get_pixel_format().macropixel_coding;
        const Coded_pixel &coded_pixel =
                macropixel_coding.pixels[
                    macropixel_coding.size.x() * pixel_in_macropixel.y()
                    + pixel_in_macropixel.x()];
        const Component_coding &component_coding =
                coded_pixel.components[component_index];
        const Index plane_index = component_coding.plane_index;
        const Plane_parameters &plane_parameters = m_planes[plane_index];
        const Bit_position entry_offset_in_row_of_pixels_in_macropixel =
                get_entry_offset_in_macropixel_in_row_in_plane(
                    plane_index,
                    component_coding.row_index,
                    component_coding.entry_index);
        return
                plane_parameters.m_offset
                + (
                    plane_parameters.m_size_per_row_of_macropixels
                    * macropixel_coordintes.y())
                + plane_parameters.m_rows[component_coding.row_index].m_offset
                + (
                    macropixel_coordintes.x()
                    * get_bits_per_macropixel_in_row_in_plane(
                        plane_index,
                        component_coding.row_index))
                + entry_offset_in_row_of_pixels_in_macropixel;
    }
    using Precalculated_pixel_format::get_bits_per_entry;
    Bit_position get_bits_per_entry(
            const Coordinates<Unit::pixel, Reference_point::picture>
                &coordinates,
            const Index component_index) const
    {
        Coordinates<Unit::pixel, Reference_point::macropixel>
                pixel_in_macropixel;
        cast_to_macropixels(
                coordinates,
                get_macropixel_size(),
                pixel_in_macropixel);
        return get_bits_per_entry(pixel_in_macropixel, component_index);
    }
    Rectangle<Unit::pixel, Reference_point::picture> get_pixel_range() const
    {
        return Rectangle<Unit::pixel, Reference_point::picture>(
                Coordinates<Unit::pixel, Reference_point::picture>(0, 0),
                m_resolution);
    }

private:
    Vector<Unit::pixel> m_resolution;
    Vector<Unit::macropixel> m_size_in_macropixels;

    struct Entry_row_parameters
    {
        Bit_position m_size;
        Bit_position m_offset;
    };

    struct Plane_parameters
    {
        std::vector<Entry_row_parameters> m_rows;
        Bit_position m_size_per_row_of_macropixels;
        Bit_position m_size;
        Bit_position m_offset;
    };

    std::vector<Plane_parameters> m_planes;

    Bit_position m_buffer_size;
};

Pixel_format get_expanded_pixel_format(
        const Color_space &color_space,
        const std::vector<Entry> &entries);
Pixel_format get_expanded_pixel_format(const Pixel_format &input);

} /* namespace YUV_tool */

#endif /* PIXEL_FORMAT_H */
