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
#include <yuv/Pixel_format.h>
#include <yuv/Errors.h>

#include <stdexcept>
#include <algorithm>
#include <numeric>

namespace YUV_tool {
/*----------------------------------------------------------------------------*/
Pixel_format get_expanded_pixel_format(
        const Color_space &color_space,
        const std::vector<Entry> &entries)
{
    const Index components_count = color_space.components.size();
    my_assert(
        size(entries) == components_count,
        "number of entries does not match the namber of planes for expanded "
        "pixel format");
    Pixel_format result;
    result.color_space = color_space;
    result.macropixel_coding.size = {1, 1};
    result.macropixel_coding.pixels.resize(1);
    Coded_pixel& target_pixel = result.macropixel_coding.pixels.front();
    target_pixel.components.resize(components_count);
    result.planes.resize(components_count);
    for (const Index component_index : make_value_range(components_count))
    {
        target_pixel.components[component_index] = {component_index, 0, 0};
        result.planes[component_index] = {{{{entries[component_index]}}}};
    }
    return result;
}
/*----------------------------------------------------------------------------*/
Pixel_format get_expanded_pixel_format(const Pixel_format &input)
{
    const Index components_count = input.color_space.components.size();

    std::vector<Entry> entries(components_count);
    const std::vector<Coded_pixel> &coded_pixels =
            input.macropixel_coding.pixels;
    for(Index plane_index = 0; plane_index < components_count; plane_index++)
    {
        const Bit_position max_depth =
                std::accumulate(
                    coded_pixels.begin(),
                    coded_pixels.end(),
                    Bit_position(0),
                    [plane_index, input](
                            const Bit_position max,
                            const Coded_pixel &coded_pixel)
                    {
                        const Component_coding &input_component_coding =
                                coded_pixel.components[plane_index];
                        const Entry &input_entry =
                                input.planes[
                                input_component_coding.plane_index].rows[
                                input_component_coding.row_index].entries[
                                input_component_coding.entry_index];
                        return std::max(max, input_entry.width);
                    });
        entries[plane_index].width.set_position(round_up(
                max_depth.get_position(), bits_in_byte));
    }

    return get_expanded_pixel_format(input.color_space, entries);
}
/*----------------------------------------------------------------------------*/
std::optional<Precalculated_pixel_format> Precalculated_pixel_format::create(
    const Pixel_format& pixel_format)
{
    Precalculated_pixel_format result;

    result.m_pixel_format = pixel_format;

    const Index planes_count = pixel_format.planes.size();
    if (!is_in_range(planes_count, valid_planes_count_range))
    {
        return {};
    }

    const Index components_count = pixel_format.color_space.components.size();
    if (!is_in_range(components_count, valid_components_count_range))
    {
        return {};
    }

    const auto macropixel_size = result.get_macropixel_size();
    if (!is_in_range(macropixel_size.x(), valid_macropixel_size_range)
        || !is_in_range(macropixel_size.y(), valid_macropixel_size_range))
    {
        return {};
    }

    const Index pixels_in_macropixel_count =
        pixel_format.macropixel_coding.pixels.size();
    if (macropixel_size.x() * macropixel_size.y() != pixels_in_macropixel_count)
    {
        return {};
    }

    result.m_planes.resize(planes_count);
    for(const Index plane_index : make_value_range(planes_count))
    {
        Plane_parameters &plane_parameters = result.m_planes[plane_index];
        const Plane &plane = pixel_format.planes[plane_index];

        const Index rows_count = pixel_format.planes[plane_index].rows.size();
        if (!is_in_range(rows_count, valid_rows_in_plane_count_range))
        {
            return {};
        }
        plane_parameters.m_rows.resize(rows_count);
        for(const Index row_index : make_value_range(rows_count))
        {
            Entry_row_paramters &row_parameters =
                    plane_parameters.m_rows[row_index];
            const Entry_row &row = plane.rows[row_index];

            const Index entry_count = row.entries.size();
            row_parameters.m_entries.resize(entry_count);
            Bit_position offset = 0;
            for (const Index entry_index : make_value_range(entry_count))
            {
                Entry_parameters &entry = row_parameters.m_entries[entry_index];
                entry.m_offset = offset;
                entry.m_sampling_point = {-1, -1};
                entry.m_component_index = -1;
                const Bit_position bit_width = row.entries[entry_index].width;
                offset += row.entries[entry_index].width;
                if (!is_in_range(bit_width, valid_bit_width_range))
                {
                    return {};
                }
            }
            row_parameters.m_bits_per_macropixel = offset;
        }
        plane_parameters.m_bits_per_macropixel =
                std::accumulate(
                    plane_parameters.m_rows.begin(),
                    plane_parameters.m_rows.end(),
                    Bit_position(0),
                    [](const Bit_position sum, const Entry_row_paramters &row)
                    {
                        return sum + row.m_bits_per_macropixel;
                    });
    }
    result.m_bits_per_macropixel =
            std::accumulate(
                result.m_planes.begin(),
                result.m_planes.end(),
                Bit_position(0),
                [](const Bit_position sum, const Plane_parameters &plane)
                {
                    return sum + plane.m_bits_per_macropixel;
                });

    const auto macropixel_rectangle =
        make_rectangle<Unit::pixel, Reference_point::macropixel>(
            {0, 0}, macropixel_size);
    for (const auto i : macropixel_rectangle)
    {
        const Index pixel_index =
            i.y() * result.get_macropixel_size().y() + i.x();
        const Coded_pixel& coded_pixel =
            pixel_format.macropixel_coding.pixels[pixel_index];
        if (size(coded_pixel.components) != components_count)
        {
            return {};
        }

        for (const Index component_index : make_value_range(components_count))
        {
            const Component_coding& component_coding =
                coded_pixel.components[component_index];

            if (!is_in_range(
                    component_coding.plane_index,
                    make_value_range(planes_count)))
            {
                return {};
            }
            const Plane& plane =
                pixel_format.planes[component_coding.plane_index];
            Plane_parameters& plane_parameters =
                result.m_planes[component_coding.plane_index];

            if (!is_in_range(
                    component_coding.row_index,
                    make_value_range(size(plane.rows))))
            {
                return {};
            }
            const Entry_row& row = plane.rows[component_coding.row_index];
            Entry_row_paramters& row_parameters =
                plane_parameters.m_rows[component_coding.row_index];

            if (!is_in_range(
                    component_coding.entry_index,
                    make_value_range(size(row.entries))))
            {
                return {};
            }
            Entry_parameters& entry_parameters =
                    row_parameters.m_entries[component_coding.entry_index];

            if (entry_parameters.m_sampling_point
                == Coordinates<Unit::pixel, Reference_point::macropixel>(
                    -1, -1))
            {
                entry_parameters.m_sampling_point = i;
                entry_parameters.m_component_index = component_index;
            }
        }
    }

    // m_is_expanded
    if (macropixel_size == Vector<Unit::pixel>(1, 1)
        && planes_count == result.get_components_count())
    {
        for (const Index plane_index : make_value_range(components_count))
        {
            if (size(pixel_format.planes[plane_index].rows) != 1
                || size(pixel_format.planes[plane_index].rows.front().entries)
                    != 1)
            {
                goto is_not_expanded;
            }
            const Coded_pixel& pixel =
                pixel_format.macropixel_coding.pixels.front();
            const Component_coding& component = pixel.components[plane_index];
            if (component.plane_index != plane_index || component.row_index != 0
                || component.entry_index != 0)
            {
                goto is_not_expanded;
            }
        }
        result.m_is_expanded = true;
    }
    else
    {
    is_not_expanded:
        result.m_is_expanded = false;
    }
    return result;
}
/*----------------------------------------------------------------------------*/
std::optional<Precalculated_buffer_parameters> Precalculated_buffer_parameters::
    create(const Pixel_format& format, const Vector<Unit::pixel>& resolution)
{
    auto maybePixelFormat = Precalculated_pixel_format::create(format);
    if (!maybePixelFormat)
    {
        return {};
    }

    Precalculated_buffer_parameters result;
    static_cast<Precalculated_pixel_format&>(result) = *maybePixelFormat;
    result.m_resolution = resolution;
    result.m_size_in_macropixels.set(
            resolution.x() / result.get_macropixel_size().x(),
            resolution.y() / result.get_macropixel_size().y());
    result.m_planes.resize(result.get_planes_count());
    Bit_position plane_offset = 0;
    for (const Index plane_index : make_value_range(result.get_planes_count()))
    {
        Plane_parameters &plane = result.m_planes[plane_index];
        plane.m_offset = plane_offset;
        const Index rows_count =
            result.get_entry_rows_count_in_plane(plane_index);
        plane.m_rows.resize(rows_count);
        Bit_position bits_per_macropixel_row_in_plane = 0;
        for (const Index row_index : make_value_range(rows_count))
        {
            plane.m_rows[row_index].m_size =
                    result.m_size_in_macropixels.x()
                    * result.get_bits_per_macropixel_in_row_in_plane(
                        plane_index,
                        row_index);
            plane.m_rows[row_index].m_offset =
                    bits_per_macropixel_row_in_plane;
            bits_per_macropixel_row_in_plane += plane.m_rows[row_index].m_size;
        }
        plane.m_size_per_row_of_macropixels = bits_per_macropixel_row_in_plane;
        plane.m_size =
            bits_per_macropixel_row_in_plane * result.m_size_in_macropixels.y();
        plane_offset += plane.m_size;
    }
    result.m_buffer_size = plane_offset;
    return result;
}
/*----------------------------------------------------------------------------*/
} /* namespace YUV_tool */
