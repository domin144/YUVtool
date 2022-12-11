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
#include <yuv/Yuv_file.h>
#include <yuv/Errors.h>

#include <iostream>

namespace YUV_tool {

Yuv_file::Yuv_file() :
    m_parameters_valid(false)
{
    m_file.exceptions(std::ios_base::badbit | std::ios_base::failbit);
}
//------------------------------------------------------------------------------
Yuv_file::Yuv_file(
        const std::filesystem::path &path,
        const std::ios_base::openmode mode) :
    m_path(path),
    m_parameters_valid(false)
{
    m_file.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    open(path, mode);
}
//------------------------------------------------------------------------------
bool Yuv_file::is_open() const
{
    return m_file.is_open();
}
//------------------------------------------------------------------------------
void Yuv_file::open(
        const std::filesystem::path &path,
        const std::ios_base::openmode mode)
{
    std::cout << "TYUV_file::TYUV_file(const std::string file_name)\n";
    m_file.open(path, mode);
    init_file_parameters();
}
//------------------------------------------------------------------------------
void Yuv_file::close()
{
    m_file.close();
}
//------------------------------------------------------------------------------
void Yuv_file::set_pixel_format(const Pixel_format &pixel_format)
{
    m_parameters_valid = false;
    m_pixel_format = pixel_format;
}
//------------------------------------------------------------------------------
const Pixel_format &Yuv_file::get_pixel_format() const
{
    return m_pixel_format;
}
//------------------------------------------------------------------------------
void Yuv_file::set_resolution(const Vector<Unit::pixel> &resolution)
{
    m_parameters_valid = false;
    m_resolution = resolution;
}
//------------------------------------------------------------------------------
Vector<Unit::pixel> Yuv_file::get_resolution() const
{
    return m_resolution;
}
//------------------------------------------------------------------------------
Bit_position Yuv_file::get_frame_size()
{
    recalculate_parameters();
    return m_buffer_parameters->get_buffer_size();
}
//------------------------------------------------------------------------------
Index Yuv_file::get_frames_count()
{
    return Bit_position(m_file_size, 0) / get_frame_size();
}
//------------------------------------------------------------------------------
void Yuv_file::set_frames_count(const Index i)
{
    my_assert(get_frames_count() <= i, "file truncation not yet supported");
    while(get_frames_count() < i)
    {
        m_file.seekp(0, std::ios::end);
        std::vector<char> input(get_frame_size().get_bytes());
        std::fill(input.begin(), input.end(), 0);
        m_file.write(input.data(), input.size());
        init_file_parameters();
    }
}
//------------------------------------------------------------------------------
Picture_buffer Yuv_file::extract_buffer(
        Index picture_number,
        const Coordinates<Unit::pixel, Reference_point::picture> &start,
        const Coordinates<Unit::pixel, Reference_point::picture> &end)
{
    recalculate_parameters();
    const Vector<Unit::pixel> macropixel_size =
            m_buffer_parameters->get_macropixel_size();
    check_range(0, picture_number, get_frames_count());
    check_range(0, start.x(), end.x());
    check_range(0, start.y(), end.y());
    check_range(0, end.x(), m_resolution.x() + 1);
    check_range(0, end.y(), m_resolution.y() + 1);
    Coordinates<Unit::pixel, Reference_point::macropixel> dummy_vector;
    my_assert(
            start
            == cast_to_pixels(
                cast_to_macropixels(start, macropixel_size, dummy_vector),
                macropixel_size),
            "non-integer macropixel boundaries are not supported");
    my_assert(
            end
            == cast_to_pixels(
                cast_to_macropixels(end, macropixel_size, dummy_vector),
                macropixel_size),
            "non-integer macropixel boundaries are not supported");


    const Pixel_format &pixel_format = get_pixel_format();
    const Vector<Unit::pixel> buffer_size = end - start;
    Picture_buffer buffer;
    buffer.allocate(buffer_size, pixel_format);
    Bit_position offset_in_buffer = 0;
    const Vector<Unit::macropixel> buffer_size_in_macropixels =
            buffer.get_parameters().get_size_in_macropixels();
    const Vector<Unit::macropixel> picture_size_in_macropixels =
            m_buffer_parameters->get_size_in_macropixels();
    const Bit_position picture_size = get_frame_size();
    const Bit_position picture_offset = picture_number * picture_size;

    const Index planes_count = pixel_format.planes.size();
    for(Index plane_idx = 0; plane_idx < planes_count; plane_idx++)
    {
        const Bit_position plane_offset =
            m_buffer_parameters->get_plane_offset(plane_idx);
        const Index pixel_rows_in_macropixel_in_plane =
            m_buffer_parameters->get_entry_rows_count_in_plane(plane_idx);
        const Bit_position macropixel_row_in_plane_size_in_bits =
            m_buffer_parameters->get_macropixel_row_in_plane_size(plane_idx);
        for(Index buffer_macropixel_row = 0;
            buffer_macropixel_row < buffer_size_in_macropixels.y();
            buffer_macropixel_row++)
        {
            const Index macropixel_row =
                    cast_to_macropixels(
                        start,
                        macropixel_size,
                        dummy_vector).y()
                    + buffer_macropixel_row;
            const Bit_position macropixel_row_offset =
                    macropixel_row_in_plane_size_in_bits * macropixel_row;
            Bit_position row_in_macropixel_offset = 0;
            for(Index row_in_macropixel = 0;
                row_in_macropixel < pixel_rows_in_macropixel_in_plane;
                row_in_macropixel++)
            {
                const Bit_position bits_per_entry_in_row_in_plane =
                    m_buffer_parameters
                        ->get_bits_per_macropixel_in_row_in_plane(
                            plane_idx, row_in_macropixel);
                const Bit_position x_position_offset =
                        cast_to_macropixels(
                            start,
                            macropixel_size,
                            dummy_vector).x()
                        * bits_per_entry_in_row_in_plane;
                const Bit_position read_length =
                        buffer_size_in_macropixels.x()
                        * bits_per_entry_in_row_in_plane;
                const Bit_position offset =
                        picture_offset
                        + plane_offset
                        + macropixel_row_offset
                        + row_in_macropixel_offset
                        + x_position_offset;

                my_assert(
                        offset.get_bits() == 0
                        && offset_in_buffer.get_bits() == 0,
                        "TODO: handle non-byte boundary reads");

                m_file.seekg(offset.get_bytes());
                m_file.read(reinterpret_cast<char *>(buffer.get_data().data() +
                        offset_in_buffer.get_bytes()),
                        read_length.get_bytes());

                row_in_macropixel_offset +=
                        bits_per_entry_in_row_in_plane *
                        picture_size_in_macropixels.x();
                offset_in_buffer += read_length;
            }
        }
    }
    return buffer;
}
//------------------------------------------------------------------------------
void Yuv_file::insert_buffer(
        const Picture_buffer &buffer,
        Index picture_number,
        const Coordinates<Unit::pixel, Reference_point::picture> &start,
        const Coordinates<Unit::pixel, Reference_point::picture> &end)
{
    const Vector<Unit::pixel> macropixel_size =
            m_buffer_parameters->get_macropixel_size();
    check_range(0, picture_number, get_frames_count());
    check_range(0, start.x(), end.x());
    check_range(0, start.y(), end.y());
    check_range(0, end.x(), m_resolution.x() + 1);
    check_range(0, end.y(), m_resolution.y() + 1);
    Coordinates<Unit::pixel, Reference_point::macropixel> dummy_vector;
    my_assert(
            start
            == cast_to_pixels(
                cast_to_macropixels(start, macropixel_size, dummy_vector),
                macropixel_size),
            "non-integer macropixel boundaries are not supported");
    my_assert(
            end
            == cast_to_pixels(
                cast_to_macropixels(end, macropixel_size, dummy_vector),
                macropixel_size),
            "non-integer macropixel boundaries are not supported");

    recalculate_parameters();

    const Pixel_format &pixel_format = get_pixel_format();
    const Vector<Unit::pixel> buffer_size = end - start;
    my_assert(
            buffer.get_resolution() == buffer_size,
            "invalid buffer size to insert");
//    my_assert(
//            buffer.get_pixel_format() == pixel_format,
//            "invalid buffer pixel format to insert");
    Bit_position offset_in_buffer = 0;
    const Vector<Unit::macropixel> buffer_size_in_macropixels =
            buffer.get_parameters().get_size_in_macropixels();
    const Vector<Unit::macropixel> picture_size_in_macropixels =
            m_buffer_parameters->get_size_in_macropixels();
    const Bit_position picture_size = get_frame_size();
    const Bit_position picture_offset = picture_number * picture_size;

    const Index planes_count = pixel_format.planes.size();
    for(Index plane_idx = 0; plane_idx < planes_count; plane_idx++)
    {
        const Bit_position plane_offset =
            m_buffer_parameters->get_plane_offset(plane_idx);
        const Index pixel_rows_in_macropixel_in_plane =
            m_buffer_parameters->get_entry_rows_count_in_plane(plane_idx);
        const Bit_position macropixel_row_in_plane_size_in_bits =
            m_buffer_parameters->get_macropixel_row_in_plane_size(plane_idx);
        for(Index buffer_macropixel_row = 0;
            buffer_macropixel_row < buffer_size_in_macropixels.y();
            buffer_macropixel_row++)
        {
            const Index macropixel_row =
                    cast_to_macropixels(
                        start,
                        macropixel_size,
                        dummy_vector).y()
                    + buffer_macropixel_row;
            const Bit_position macropixel_row_offset =
                    macropixel_row_in_plane_size_in_bits * macropixel_row;
            Bit_position row_in_macropixel_offset = 0;
            for(Index row_in_macropixel = 0;
                row_in_macropixel < pixel_rows_in_macropixel_in_plane;
                row_in_macropixel++)
            {
                const Bit_position bits_per_entry_row_in_plane =
                    m_buffer_parameters
                        ->get_bits_per_macropixel_in_row_in_plane(
                            plane_idx, row_in_macropixel);
                const Bit_position x_position_offset =
                        cast_to_macropixels(
                            start,
                            macropixel_size,
                            dummy_vector).x()
                        * bits_per_entry_row_in_plane;
                const Bit_position read_length =
                        buffer_size_in_macropixels.x()
                        * bits_per_entry_row_in_plane;
                const Bit_position offset =
                        picture_offset
                        + plane_offset
                        + macropixel_row_offset
                        + row_in_macropixel_offset
                        + x_position_offset;

                my_assert(
                        offset.get_bits() == 0
                        && offset_in_buffer.get_bits() == 0,
                        "TODO: handle non-byte boundary reads");

                m_file.seekp(offset.get_bytes());
                m_file.write(
                        reinterpret_cast<const char *>(
                            buffer.get_data().data()
                            + offset_in_buffer.get_bytes()),
                        read_length.get_bytes());

                row_in_macropixel_offset +=
                        bits_per_entry_row_in_plane *
                        picture_size_in_macropixels.x();
                offset_in_buffer += read_length;
            }
        }
    }
}
//------------------------------------------------------------------------------
void Yuv_file::init_file_parameters()
{
    // get the file size
    {
        m_file.seekg(0, std::ios_base::beg);
        auto begin = m_file.tellg();
        m_file.seekg(0, std::ios_base::end);
        auto end = m_file.tellg();
        m_file_size = end - begin;
    }
    std::cout << "m_file_size = " << m_file_size << std::endl;
    m_parameters_valid = false;
}
//------------------------------------------------------------------------------
void Yuv_file::recalculate_parameters()
{
    m_buffer_parameters =
        Precalculated_buffer_parameters::create(m_pixel_format, m_resolution);
}

} /* namespace YUV_tool */
