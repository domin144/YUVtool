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
#include <drawer_gl.h>
#include <yuv/Picture_buffer.h>
#include <yuv/Errors.h>
#include <yuv/Yuv_file.h>
#include <yuv/Cache.h>

#include <epoxy/gl.h>
#include <cmath>

namespace YUV_tool {
/*----------------------------------------------------------------------------*/
namespace {
/*----------------------------------------------------------------------------*/
const Index tile_size = 64;
const Index buffer_count = 8;
const Index tile_size_in_bytes =
        tile_size * tile_size * Rgba_component_count * 2;
const Index bytes_in_GLfloat = 4;
const Index vertex_count = 4;
using Tile_in_scaled_pic_coord =
        Coordinates<Unit::tile, Reference_point::scaled_picture>;
using Pixel_in_picture =
        Coordinates<Unit::pixel, Reference_point::picture>;
/*----------------------------------------------------------------------------*/
struct Resource_id
{
    Index m_picture_index;
    Index m_leaf_id;

    Resource_id(
            const Index picture_index,
            const Coordinates<Unit::tile, Reference_point::scaled_picture>
                coordinates,
            const Index zoom_level) :
        m_picture_index(picture_index)
    {
        const Index stride = cached_power<2>(zoom_level);
        m_leaf_id =
                get_first_child_in_n_tree_at_level<4>(0, zoom_level)
                + coordinates.y() * stride
                + coordinates.x();
    }

    Resource_id() :
        m_picture_index(0),
        m_leaf_id(0)
    { }

    bool operator<(const Resource_id rhs) const
    {
        return
                std::make_pair(m_picture_index, m_leaf_id)
                < std::make_pair(rhs.m_picture_index, rhs.m_leaf_id);
    }
};
/*----------------------------------------------------------------------------*/
/* Source is expected to be in rgb_32bpp format. */
Picture_buffer extract_tile(
        const Picture_buffer &source,
        const Vector<Unit::pixel> offset)
{
    const Index bytes_per_pixel = Rgba_component_count;
    const Index offset_in_row = offset.x() * bytes_per_pixel;
    const Index source_stride = bytes_per_pixel * source.get_resolution().x();
    const Index target_stride = tile_size * bytes_per_pixel;
    const Index available_pixels_in_row =
            std::min(
                source_stride - offset_in_row,
                target_stride);
    const Index available_rows =
            std::min(
                source.get_resolution().y() - offset.y(),
                tile_size);
    Picture_buffer result(
                Vector<Unit::pixel>(tile_size, tile_size),
                rgb_32bpp);
    for(Index y = 0; y < available_rows; ++y)
    {
        const auto row_source_begin =
                source.get_data().begin()
                + (offset.y() + y) * source_stride
                + offset_in_row;
        std::copy(
                    row_source_begin,
                    row_source_begin + available_pixels_in_row,
                    result.get_data().begin() + y * target_stride);
        std::fill(
                    (
                        result.get_data().begin()
                        + y * target_stride
                        + available_pixels_in_row),
                    result.get_data().begin() + (y + 1) * target_stride,
                    0);
    }
    for(Index y = available_rows; y < tile_size; ++y)
    {
        std::fill(
                    result.get_data().begin() + y * target_stride,
                    result.get_data().begin() + (y + 1) * target_stride,
                    0);
    }

    return result;
}
/*----------------------------------------------------------------------------*/
} /* anonymous namespace */
/*----------------------------------------------------------------------------*/
class Drawer_gl::Implementation
{
public:
    Implementation();
    ~Implementation();
    void initialize(Index cache_size);
    void deinitialize();
    bool is_initialized() const;
    void attach_yuv_file(Yuv_file *yuv_file);
    void draw(
            Index picture_index,
            Rectangle<Unit::pixel, Reference_point::scaled_picture>
                visible_area,
            float scale);

private:
    void reallocate_buffers(Index buffers_count);
    std::vector<Tile_in_scaled_pic_coord> get_missing_buffers(
            Index picture_index,
            Rectangle<Unit::tile, Reference_point::scaled_picture> tile_range,
            Index zoom_level);
    void fill_missing_buffers(
            Index picture_index,
            const std::vector<Tile_in_scaled_pic_coord> &buffers,
            Index zoom_level);
    void fill_tile(
            Index buffer_index,
            const Picture_buffer &source_buffer,
            Vector<Unit::pixel> offset_in_source,
            Rectangle<Unit::pixel, Reference_point::picture> rectangle);
    Index assign_buffer_index(Resource_id key);
    Index get_buffer_index(Resource_id key);
    static Index get_picture_zoom_levels_count(Vector<Unit::pixel> resolution);
    static Index get_unzoom_steps_count(float scale);
    void check_gl_errors();

    std::vector<GLuint> m_vertex_arrays;
    std::vector<GLuint> m_coordinate_buffers;
    std::vector<GLuint> m_textures;
    std::vector<GLuint> m_pixel_buffers;

    GLuint m_vertex_shader;
    GLuint m_fragment_shader;
    GLuint m_shader_program;
    GLuint m_sampling_buffer;

    GLint m_viewport_start_location;
    GLint m_viewport_size_location;

    /* contains indices of elements in m_vertex_arrays array */
    Cache<Resource_id, Index> m_cache;

    Yuv_file *m_yuv_file;

    bool m_initialized;
};
/*----------------------------------------------------------------------------*/
Drawer_gl::Implementation::Implementation() :
    m_vertex_shader(0),
    m_fragment_shader(0),
    m_shader_program(0),
    m_sampling_buffer(0),
    m_viewport_start_location(0),
    m_viewport_size_location(0),
    m_cache(0),
    m_yuv_file(nullptr),
    m_initialized(false)
{ }
/*----------------------------------------------------------------------------*/
Drawer_gl::Implementation::~Implementation()
{
    if (m_initialized)
        deinitialize();
}
/*----------------------------------------------------------------------------*/
/* This function is required to be called after OpenGL context is ready
 * and before any drawing is done using the drawer.
 * cache_size is expressed in number of 64x64 pixel tiles. Each tile occupies
 * 32 KB of video memory. */
void Drawer_gl::Implementation::initialize(const Index cache_size)
{
    my_assert(!m_initialized, "trying to initialize initialized drawer");
    GLint success;
    const int max_log_length = 512;
    GLchar info_log[max_log_length];

    /* vetex shader */
    m_vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    MY_ASSERT(m_vertex_shader);
    const GLchar *vertex_shader_source =
R"(
#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texture_sampling;
out vec2 texture_sampling2;
uniform vec2 viewport_size;
uniform vec2 viewport_start;
void main()
{
    vec2 shifted_position =
            position - viewport_start;
    vec2 scaled_position =
            vec2(
                shifted_position.x / viewport_size.x * 2.0f - 1.0f,
                shifted_position.y / viewport_size.y * -2.0f + 1.0f);
    gl_Position = vec4(scaled_position, 0.0f, 1.0f);
    texture_sampling2 = texture_sampling;
}
)";
    glShaderSource(m_vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(m_vertex_shader);
    glGetShaderiv(m_vertex_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(m_vertex_shader, max_log_length, nullptr, info_log);
        my_assert(
                    false,
                    std::string("could not compile vertex shader: ")
                    + info_log);
    }

    /* fragment shader */
    m_fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    MY_ASSERT(m_fragment_shader);
    const GLchar *fragment_shader_source =
R"(
#version 330 core
in vec2 texture_sampling2;
out vec4 color;
uniform sampler2D my_texture;
void main()
{
    color = texture(my_texture, texture_sampling2);
}
)";
    glShaderSource(m_fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(m_fragment_shader);
    glGetShaderiv(m_fragment_shader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(
            m_fragment_shader, max_log_length, nullptr, info_log);
        my_assert(
                    false,
                    std::string("could not compile fragment shader: ")
                    + info_log);
    }

    /* shader program */
    m_shader_program = glCreateProgram();
    MY_ASSERT(m_shader_program);
    glAttachShader(m_shader_program, m_vertex_shader);
    glAttachShader(m_shader_program, m_fragment_shader);
    glLinkProgram(m_shader_program);
    glGetProgramiv(m_shader_program, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(m_shader_program, max_log_length, nullptr, info_log);
        my_assert(
                    false,
                    std::string("could not link shader program: ")
                    + info_log);
    }

    m_viewport_size_location =
            glGetUniformLocation(m_shader_program, "viewport_size");
    m_viewport_start_location =
            glGetUniformLocation(m_shader_program, "viewport_start");
    my_assert(
                m_viewport_size_location != -1
                && m_viewport_start_location != -1,
                "GL could not find location of uniform");

    /* buffers and cache */
    reallocate_buffers(cache_size);
    m_cache = Cache<Resource_id, Index>(cache_size);

    /* texture sampling buffer */
    glGenBuffers(1, &m_sampling_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_sampling_buffer);
    const Index texture_sampling_array_size = vertex_count * 2;
    const GLfloat texture_sampling_array[texture_sampling_array_size] =
    {
        0.0, 0.0,
        0.0, 1.0,
        1.0, 0.0,
        1.0, 1.0
    };
    glBufferData(
                GL_ARRAY_BUFFER,
                texture_sampling_array_size * bytes_in_GLfloat,
                texture_sampling_array,
                GL_STATIC_DRAW);

    /* finalize */
    m_initialized = true;
    check_gl_errors();
}
/*----------------------------------------------------------------------------*/
/* This function should be called when no more drawing is expected to be done */
void Drawer_gl::Implementation::deinitialize()
{
    my_assert(m_initialized, "trying to deinitialize uninitialized Drawer_gl");
    reallocate_buffers(0);
    glDeleteBuffers(1, &m_sampling_buffer);
    glDeleteProgram(m_shader_program);
    glDeleteShader(m_vertex_shader);
    glDeleteShader(m_fragment_shader);
    m_initialized = false;

    check_gl_errors();
}
/*----------------------------------------------------------------------------*/
bool Drawer_gl::Implementation::is_initialized() const
{
    return m_initialized;
}
/*----------------------------------------------------------------------------*/
/* yuv_file might be nullptr in no yuv_file is to be attached */
void Drawer_gl::Implementation::attach_yuv_file(Yuv_file *yuv_file)
{
    m_yuv_file = yuv_file;
}
/*----------------------------------------------------------------------------*/
/* visible_area might be greater than the scaled picture */
void Drawer_gl::Implementation::draw(
        const Index picture_index,
        const Rectangle<Unit::pixel, Reference_point::scaled_picture>
            visible_area,
        const float scale)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!(is_initialized() && m_yuv_file))
    {
        return;
    }

    const Index zoom_levels_count =
            get_picture_zoom_levels_count(m_yuv_file->get_resolution());
    const Index unzoom_steps_count = get_unzoom_steps_count(scale);
    const Index zoom_level = zoom_levels_count - 1 - unzoom_steps_count;
    const Vector<Unit::pixel> resolution = m_yuv_file->get_resolution();
    const Vector<Unit::pixel> scaled_picture_size(
            static_cast<Length>(std::ceil(resolution.x() * scale)),
            static_cast<Length>(std::ceil(resolution.y() * scale)));
    const Coordinates<Unit::pixel, Reference_point::scaled_picture>
            area_to_be_drawn(
                std::min(visible_area.get_end().x(), scaled_picture_size.x()),
                std::min(visible_area.get_end().y(), scaled_picture_size.y()));

    const Coordinates<Unit::tile, Reference_point::scaled_picture> begin_tile(
                visible_area.get_start().x() / tile_size,
                visible_area.get_start().y() / tile_size);
    const Coordinates<Unit::tile, Reference_point::scaled_picture> end_tile(
                round_up(area_to_be_drawn.x(), tile_size) / tile_size,
                round_up(area_to_be_drawn.y(), tile_size) / tile_size);
    const auto tiles_range = make_rectangle(begin_tile, end_tile - begin_tile);

    auto missing_buffers =
            get_missing_buffers(picture_index, tiles_range, zoom_level);
    fill_missing_buffers(picture_index, missing_buffers, zoom_level);

    for(auto tile : tiles_range)
    {
        const Resource_id key(picture_index, tile, zoom_level);
        const Index buffer_index = get_buffer_index(key);

        /* bind texture */
        glBindTexture(GL_TEXTURE_2D, m_textures[buffer_index]);

        /* bind vertex array object */
        glBindVertexArray(m_vertex_arrays[buffer_index]);

        glUseProgram(m_shader_program);
        glUniform2f(
                    m_viewport_size_location,
                    1.0f * visible_area.get_size().x(),
                    1.0f * visible_area.get_size().y());
        glUniform2f(
                    m_viewport_start_location,
                    1.0f * visible_area.get_start().x(),
                    1.0f * visible_area.get_start().y());

        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertex_count);

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        check_gl_errors();
    }
}
/*----------------------------------------------------------------------------*/
void Drawer_gl::Implementation::reallocate_buffers(const Index buffers_count)
{
    MY_ASSERT(m_vertex_arrays.size() == m_coordinate_buffers.size());
    MY_ASSERT(m_vertex_arrays.size() == m_textures.size());
    MY_ASSERT(m_vertex_arrays.size() == m_pixel_buffers.size());
    const Index old_size = m_vertex_arrays.size();
    if(old_size != buffers_count)
    {
        if(old_size < buffers_count)
        {
            m_vertex_arrays.resize(buffers_count);
            m_coordinate_buffers.resize(buffers_count);
            m_pixel_buffers.resize(buffers_count);
            m_textures.resize(buffers_count);
            glGenVertexArrays(
                        buffers_count - old_size,
                        &m_vertex_arrays[old_size]);
            glGenBuffers(
                        buffers_count - old_size,
                        &m_coordinate_buffers[old_size]);
            glGenTextures(buffers_count - old_size, &m_textures[old_size]);
            glGenBuffers(buffers_count - old_size, &m_pixel_buffers[old_size]);
        }
        else
        {
            glDeleteVertexArrays(
                    old_size - buffers_count,
                    &m_vertex_arrays[buffers_count]);
            glDeleteBuffers(
                    old_size - buffers_count,
                    &m_coordinate_buffers[buffers_count]);
            glDeleteTextures(
                    old_size - buffers_count,
                    &m_textures[buffers_count]);
            glDeleteBuffers(
                    old_size - buffers_count,
                    &m_pixel_buffers[buffers_count]);
            m_vertex_arrays.resize(buffers_count);
            m_coordinate_buffers.resize(buffers_count);
            m_pixel_buffers.resize(buffers_count);
            m_textures.resize(buffers_count);
        }
    }
    check_gl_errors();
}
/*----------------------------------------------------------------------------*/
std::vector<Tile_in_scaled_pic_coord>
    Drawer_gl::Implementation::get_missing_buffers(
        const Index picture_index,
        const Rectangle<Unit::tile, Reference_point::scaled_picture> tile_range,
        const Index zoom_level)
{
    std::vector<Tile_in_scaled_pic_coord> result;
    for(auto tile_coordinates : tile_range)
    {
        const Resource_id key(picture_index, tile_coordinates, zoom_level);

        /* no need to update priority, will update on draw */
        if(!m_cache.get(key).has_value())
        {
            result.push_back(tile_coordinates);
        }
    }
    return result;
}
/*----------------------------------------------------------------------------*/
void Drawer_gl::Implementation::fill_missing_buffers(
        const Index picture_index,
        const std::vector<Tile_in_scaled_pic_coord> &buffers,
        const Index zoom_level)
{
    const auto macropixel_size =
            m_yuv_file->get_pixel_format().macropixel_coding.size;
    const Index zoom_levels_count =
            get_picture_zoom_levels_count(m_yuv_file->get_resolution());
    my_assert(zoom_level == zoom_levels_count - 1, "scaling not supported yet");

    auto tile_row_begin = buffers.begin();
    auto tile_row_end = buffers.begin();
    while(tile_row_begin != buffers.end())
    {
        while(
                tile_row_end != buffers.end()
                && tile_row_end->y() == tile_row_begin->y())
            ++tile_row_end;

        Coordinates<Unit::pixel, Reference_point::picture> strip_begin(
                    round_down(
                        tile_row_begin->x() * tile_size,
                        macropixel_size.x()),
                    round_down(
                        tile_row_begin->y() * tile_size,
                        macropixel_size.y()));
        Coordinates<Unit::pixel, Reference_point::picture> strip_end(
                    std::min(
                        round_up(
                            ((tile_row_end - 1)->x() + 1) * tile_size,
                            macropixel_size.x()),
                        m_yuv_file->get_resolution().x()),
                    std::min(
                        round_up(
                            (tile_row_begin->y() + 1) * tile_size,
                            macropixel_size.y()),
                        m_yuv_file->get_resolution().y()));
        auto strip =
                m_yuv_file->extract_buffer(
                    picture_index,
                    strip_begin,
                    strip_end);
        auto strip_rgb = convert(strip, rgb_32bpp);

        for(auto tile : make_range(tile_row_begin, tile_row_end))
        {
            const Coordinates<Unit::pixel, Reference_point::picture> tile_begin(
                        tile.x() * tile_size,
                        tile.y() * tile_size);
            const Index scale_inverted =
                    1 << (zoom_levels_count - 1 - zoom_level);
            const auto tile_in_picture =
                    make_rectangle(
                        Pixel_in_picture(
                            tile_begin.x() * scale_inverted,
                            tile_begin.y() * scale_inverted),
                        Vector<Unit::pixel>(
                            64 * scale_inverted,
                            64 * scale_inverted));
            const auto offset_in_strip =
                    tile_begin - strip_begin;
            const Resource_id resource_id(
                        picture_index,
                        tile,
                        zoom_level);
            /* buffer indices in the middle of the row may be already filled,
             * but we have read the data anyway, so not much sense skipping
             * them. Might implement skipping later. */
            const auto buffer_index = assign_buffer_index(resource_id);
            fill_tile(
                        buffer_index,
                        strip_rgb,
                        offset_in_strip,
                        tile_in_picture);
        }

        tile_row_begin = tile_row_end;
    }
}
/*----------------------------------------------------------------------------*/
/* Buffer is expected to be in rgb_32bpp format. 64x64 rectangle will be
 * extracted from the source buffer. */
void Drawer_gl::Implementation::fill_tile(
        const Index buffer_index,
        const Picture_buffer &source_buffer,
        const Vector<Unit::pixel> offset_in_source,
        const Rectangle<Unit::pixel, Reference_point::picture> rectangle)
{
    const Picture_buffer rgb_buffer =
            extract_tile(source_buffer, offset_in_source);

    /* bind and fill the pixel buffer */
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pixel_buffers[buffer_index]);

        const Index pixel_buffer_size =
                Rgba_component_count * tile_size * tile_size;

        glBufferData(
                    GL_PIXEL_UNPACK_BUFFER,
                    pixel_buffer_size,
                    rgb_buffer.get_data().data(),
                    GL_STATIC_DRAW);
    }

    /* bind texture and provide it with data from the currently bound
     * pixel unpack buffer */
    {
        glBindTexture(GL_TEXTURE_2D, m_textures[buffer_index]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGB,
                    tile_size,
                    tile_size,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    nullptr);
    }

    const GLfloat x0 = rectangle.get_start().x();
    const GLfloat x1 = rectangle.get_end().x();
    const GLfloat y0 = rectangle.get_start().y();
    const GLfloat y1 = rectangle.get_end().y();

    /* bind vertex array object */
    glBindVertexArray(m_vertex_arrays[buffer_index]);

    /* Buffer for generic vertex attribute 0 - coordinates */
    {
        glBindBuffer(
                    GL_ARRAY_BUFFER,
                    m_coordinate_buffers[buffer_index]);

        /* triangle strip, see OpenGL 3.3, section ?? */
        const Index vertex_buffer_size = vertex_count * 2;
        GLfloat vertex_array[vertex_buffer_size] =
        {
            x0, y0,
            x0, y1,
            x1, y0,
            x1, y1
        };

        glBufferData(
                    GL_ARRAY_BUFFER,
                    vertex_buffer_size * bytes_in_GLfloat,
                    vertex_array,
                    GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    /* Buffer for generic vertex attribute 1 - texture sampling points */
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_sampling_buffer);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
/*----------------------------------------------------------------------------*/
Index Drawer_gl::Implementation::assign_buffer_index(const Resource_id key)
{
    MY_ASSERT(is_initialized());
    const Index buffers_count = m_vertex_arrays.size();
    MY_ASSERT(m_cache.get_cache_size() == buffers_count);

    std::optional<Index> index {m_cache.get_and_update(key)};
    if (!index.has_value())
    {
        if(m_cache.is_full())
        {
            index = m_cache.pop();
        }
        else
        {
            index = m_cache.get_used_slots_count();
        }
        m_cache.push(key, Index{index.value()});
    }
    return index.value();
}
/*----------------------------------------------------------------------------*/
Index Drawer_gl::Implementation::get_buffer_index(const Resource_id key)
{
    MY_ASSERT(is_initialized());

    auto optional_index = m_cache.get(key);
    MY_ASSERT(optional_index.has_value());
    return optional_index.value();
}
/*----------------------------------------------------------------------------*/
Index Drawer_gl::Implementation::get_picture_zoom_levels_count(
        const Vector<Unit::pixel> resolution)
{
    Index depth = 1;
    Index size = tile_size;
    while(
          resolution.x() > size
          || resolution.y() > size)
    {
        depth++;
        size *= 2;
    }
    return depth;
}
/*----------------------------------------------------------------------------*/
Index Drawer_gl::Implementation::get_unzoom_steps_count(float scale)
{
    MY_ASSERT(scale > 0.0);
    Index step = 0;
    while(scale < 0.5)
    {
        step++;
        scale *= 2;
    }
    return step;
}
/*----------------------------------------------------------------------------*/
void Drawer_gl::Implementation::check_gl_errors()
{
    GLenum error_code = glGetError();

    while(error_code != GL_NO_ERROR)
    {
        const char *error_name = nullptr;
        switch(error_code)
        {
        case GL_INVALID_ENUM:
            error_name = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            error_name = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            error_name = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            error_name = "GL_OUT_OF_MEMORY";
            break;
        default:
            error_name = "unknown error";
            break;
        }

        my_assert(false, "GL error detected: " + std::string(error_name));
    }
}
/*----------------------------------------------------------------------------*/
Drawer_gl::Drawer_gl() : m_implementation(std::make_unique<Implementation>())
{ }
/*----------------------------------------------------------------------------*/
Drawer_gl::~Drawer_gl()
{ }
/*----------------------------------------------------------------------------*/
void Drawer_gl::initialize(const Index cache_size)
{
    m_implementation->initialize(cache_size);
}
/*----------------------------------------------------------------------------*/
void Drawer_gl::deinitialize()
{
    m_implementation->deinitialize();
}
/*----------------------------------------------------------------------------*/
bool Drawer_gl::is_initialized() const
{
    return m_implementation->is_initialized();
}
/*----------------------------------------------------------------------------*/
void Drawer_gl::attach_yuv_file(Yuv_file *yuv_file)
{
    m_implementation->attach_yuv_file(yuv_file);
}
/*----------------------------------------------------------------------------*/
void Drawer_gl::draw(
        const Index frame_number,
        const Rectangle<Unit::pixel, Reference_point::scaled_picture>
            visible_area,
        const float scale)
{
    m_implementation->draw(frame_number, visible_area, scale);
}
/*----------------------------------------------------------------------------*/
} /* namespace YUV_tool */
