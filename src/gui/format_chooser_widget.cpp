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
#include "gtkmm/enums.h"
#include <format_chooser_widget.h>
#include <gtkmm-3.0/gtkmm/enums.h>

namespace YUV_tool {
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Format_chooser_widget() :
    m_box(Gtk::Orientation::VERTICAL)
{
    set_child(m_box);

    m_box.append(m_predefined_choice);
    m_box.append(m_notebook);
    m_notebook.append_page(m_color_space_frame, "Color space");
    m_notebook.append_page(m_plane_frame, "Planes");
    m_notebook.append_page(m_macropixel_frame, "Macropixel");

    /* predefined choice */
    {
        m_predefined_list_store =
                Gtk::ListStore::create(m_predefined_column_record);
        Gtk::ListStore::iterator iter;
        iter = m_predefined_list_store->append();
        (*iter)[m_predefined_column_record.label] = "yuv420p";
        (*iter)[m_predefined_column_record.pointer] = &yuv_420p_8bit;
        iter = m_predefined_list_store->append();
        (*iter)[m_predefined_column_record.label] = "rgb32bpp";
        (*iter)[m_predefined_column_record.pointer] = &rgb_32bpp;
        iter = m_predefined_list_store->append();
        (*iter)[m_predefined_column_record.label] = "custom";
        (*iter)[m_predefined_column_record.pointer] = nullptr;
        m_predefined_choice.set_model(m_predefined_list_store);
        m_predefined_choice.pack_start(m_predefined_column_record.label);
        m_predefined_choice.set_active(iter);
    }

    auto update_handler = sigc::mem_fun(*this, &Format_chooser_widget::update);
    m_predefined_choice.signal_changed().connect(update_handler);
    m_color_space_frame.signal_color_space_changed().connect(update_handler);
    m_plane_frame.m_planes_count_entry.signal_value_changed().connect(
        update_handler);
    m_macropixel_frame.m_rows_entry.signal_value_changed().connect(
        update_handler);
    m_macropixel_frame.m_columns_entry.signal_value_changed().connect(
        update_handler);
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::~Format_chooser_widget()
{ }
/*----------------------------------------------------------------------------*/
const Pixel_format Format_chooser_widget::get_pixel_format() const
{
    const Pixel_format* predefined_format =
        (*m_predefined_choice.get_active())[m_predefined_column_record.pointer];

    if(predefined_format != nullptr)
    {
        return *predefined_format;
    }

    Pixel_format result;

    result.color_space = m_color_space_frame.get_color_space();

    const Index planes_count =
        m_plane_frame.m_planes_count_entry.get_value_as_int();
    result.planes.resize(planes_count);
    for (Index i = 0; i < planes_count; ++i)
    {
        if (i >= m_plane_frame.m_planes.size())
        {
            continue;
        }

        auto& plane_out = result.planes[i];
        const auto& plane_in = *m_plane_frame.m_planes[i];
        const Index rows_count = plane_in.m_row_count_entry.get_value_as_int();
        plane_out.rows.resize(rows_count);
        for (Index j = 0; j < rows_count; ++j)
        {
            if (j >= plane_in.m_rows.size())
            {
                continue;
            }

            auto& row_out = plane_out.rows[j];
            const auto& row_in = *plane_in.m_rows[j];
            const Index entries_count =
                row_in.m_entry_count_entry.get_value_as_int();
            row_out.entries.resize(entries_count);
            for (Index k = 0; k < entries_count; ++k)
            {
                if (k >= row_in.m_entries.size())
                {
                    continue;
                }

                auto& entry_out = row_out.entries[k];
                const auto& entry_in = *row_in.m_entries[k];
                entry_out.width = entry_in.m_bit_count_entry.get_value_as_int();
            }
        }
    }

    const Index components_count = result.color_space.components.size();
    auto& mp_size = result.macropixel_coding.size;
    mp_size.set(
        m_macropixel_frame.m_columns_entry.get_value_as_int(),
        m_macropixel_frame.m_rows_entry.get_value_as_int());
    result.macropixel_coding.pixels.resize(mp_size.x() * mp_size.y());
    const auto mp_rectangle = make_rectangle(
        Coordinates<Unit::pixel, Reference_point::macropixel> {}, mp_size);
    for (const auto p : mp_rectangle)
    {
        const Index i = p.y() * mp_size.x() + p.x();
        auto& pixel_out = result.macropixel_coding.pixels[i];
        if (i >= m_macropixel_frame.m_pixels.size())
        {
            pixel_out.components.resize(result.color_space.components.size());
            continue;
        }

        const auto& pixel_in = *m_macropixel_frame.m_pixels[i];
        pixel_out.components.resize(components_count);
        for (Index j = 0; j < components_count; ++j)
        {
            if (j >= pixel_in.m_samples.size())
            {
                continue;
            }
            auto& component_out = pixel_out.components[j];
            const auto& component_in = *pixel_in.m_samples[j];
            component_out.plane_index =
                component_in.m_plane_entry.get_value_as_int();
            component_out.row_index =
                component_in.m_row_entry.get_value_as_int();
            component_out.entry_index =
                component_in.m_index_entry.get_value_as_int();
        }
    }
    return result;
}
/*----------------------------------------------------------------------------*/
void Format_chooser_widget::set_pixel_format(const Pixel_format& pixel_format)
{
    if (pixel_format == get_pixel_format())
        return;

    m_update_in_progress = true;
    const auto predefined_container = m_predefined_list_store->children();
    const auto predefined_iter = std::find_if(
        predefined_container.begin(),
        predefined_container.end(),
        [&pixel_format, this](const auto& x) {
            const Pixel_format* const predefined_pf =
                x[m_predefined_column_record.pointer];
            return predefined_pf && *predefined_pf == pixel_format;
        });
    if (predefined_iter == predefined_container.end())
    {
        m_predefined_choice.set_active(--predefined_container.end());
    }
    else
    {
        m_predefined_choice.set_active(predefined_iter);
    }

    update_entries(pixel_format);
    m_update_in_progress = false;
    signal_pixel_format_changed().emit();
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Entry_configurator::Entry_configurator() :
    Gtk::Box(Gtk::Orientation::HORIZONTAL),
    m_entry_label("entry:"),
    m_bit_count_entry(Gtk::Adjustment::create(0, 0, 100, 1)),
    m_bits_label("bits")
{
    append(m_entry_label);
    append(m_bit_count_entry);
    append(m_bits_label);
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Row_in_plane_configurator::Row_in_plane_configurator() :
    Gtk::Box(Gtk::Orientation::VERTICAL),
    m_entry_count_box(Gtk::Orientation::HORIZONTAL),
    m_entry_count_label("entries count:"),
    m_entry_count_entry(Gtk::Adjustment::create(0, 0, 100, 1))
{
    append(m_entry_count_box);
    m_entry_count_box.append(m_entry_count_label);
    m_entry_count_box.append(m_entry_count_entry);
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Plane_configurator::Plane_configurator() :
    m_box(Gtk::Orientation::VERTICAL),
    m_row_count_label("rows count:"),
    m_row_count_entry(Gtk::Adjustment::create(0, 0, 100, 1))
{
    set_child(m_box);
    m_box.append(m_row_count_box);
    m_row_count_box.append(m_row_count_label);
    m_row_count_box.append(m_row_count_entry);
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Plane_frame::Plane_frame() :
    m_box(Gtk::Orientation::VERTICAL),
    m_planes_count_box(Gtk::Orientation::HORIZONTAL),
    m_planes_count_label("planes count: "),
    m_planes_count_entry(Gtk::Adjustment::create(0, 0, 100, 1)),
    m_planes_box(Gtk::Orientation::HORIZONTAL)
{
    set_label("Planes");

    set_child(m_box);
    m_box.append(m_planes_count_box);
    m_planes_count_box.append(m_planes_count_label);
    m_planes_count_box.append(m_planes_count_entry);
    m_box.append(m_planes_box);
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Sample_configurator::Sample_configurator() :
    Gtk::Box(Gtk::Orientation::HORIZONTAL),
    m_sample_label("sample:"),
    m_plane_label("plane:"),
    m_plane_entry(Gtk::Adjustment::create(0, 0, 1000)),
    m_row_label("row"),
    m_row_entry(Gtk::Adjustment::create(0, 0, 1000)),
    m_index_label("index"),
    m_index_entry(Gtk::Adjustment::create(0, 0, 1000))
{
    append(m_sample_label);
    append(m_plane_label);
    append(m_plane_entry);
    append(m_row_label);
    append(m_row_entry);
    append(m_index_label);
    append(m_index_entry);
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Pixel_configurator::Pixel_configurator() :
    m_box(Gtk::Orientation::VERTICAL)
{
    set_child(m_box);
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Macropixel_frame::Macropixel_frame() :
    m_box(Gtk::Orientation::VERTICAL),
    m_parameters_box(Gtk::Orientation::HORIZONTAL),
    m_rows_label("rows:"),
    m_rows_entry(Gtk::Adjustment::create(0, 0, 100)),
    m_columns_label("columns:"),
    m_columns_entry(Gtk::Adjustment::create(0, 0, 100))
{
    set_label("Macropixel");

    set_child(m_box);
    m_box.append(m_parameters_box);
    m_parameters_box.append(m_rows_label);
    m_parameters_box.append(m_rows_entry);
    m_parameters_box.append(m_columns_label);
    m_parameters_box.append(m_columns_entry);
    m_box.append(m_pixel_grid);
}
/*----------------------------------------------------------------------------*/
void Format_chooser_widget::Macropixel_frame::reshape()
{
    const Vector<Unit::pixel> mp_size(
                m_columns_entry.get_value_as_int(),
                m_rows_entry.get_value_as_int());

    auto pixels_range =
            make_rectangle(
                Coordinates<Unit::pixel, Reference_point::macropixel>(0, 0),
                mp_size);
    for (auto xy : pixels_range)
    {
        m_pixel_grid.attach(
                    *m_pixels[xy.y() * mp_size.x() + xy.x()],
                    xy.x(),
                    xy.y(),
                    1,
                    1);
    }
}
/*----------------------------------------------------------------------------*/
Format_chooser_widget::Pixel_format_column_record::Pixel_format_column_record()
{
    add(label);
    add(pointer);
}
/*----------------------------------------------------------------------------*/
void Format_chooser_widget::update()
{
    if (m_update_in_progress)
        return;

    m_update_in_progress = true;
    update_entries(get_pixel_format());
    m_update_in_progress = false;

    signal_pixel_format_changed().emit();
}
/*----------------------------------------------------------------------------*/
void Format_chooser_widget::update_entries(const Pixel_format &pixel_format)
{
    const Pixel_format* predefined_format =
        (*m_predefined_choice.get_active())[m_predefined_column_record.pointer];
    const bool use_predefined = predefined_format != nullptr;
    m_plane_frame.set_sensitive(!use_predefined);
    m_color_space_frame.set_sensitive(!use_predefined);
    m_macropixel_frame.set_sensitive(!use_predefined);

    m_color_space_frame.set_color_space(pixel_format.color_space);

    auto fix_size =
        [](auto& container, const Index new_size, auto connect) {
            const Index old_size = container.size();
            container.resize(new_size);
            for (Index i = old_size; i < new_size; ++i)
            {
                container[i] =
                    std::make_unique<std::decay_t<decltype(*container[i])>>();
                connect(*container[i]);
            }
        };
    auto update_handler = sigc::mem_fun(*this, &Format_chooser_widget::update);

    const Index planes_count = pixel_format.planes.size();
    m_plane_frame.m_planes_count_entry.set_value(planes_count);
    fix_size(
        m_plane_frame.m_planes,
        planes_count,
        [this, &update_handler](auto& plane_out) {
            m_plane_frame.m_planes_box.append(plane_out);
            plane_out.m_row_count_entry.signal_value_changed().connect(
                update_handler);
        });
    for (Index i = 0; i < planes_count; ++i)
    {
        auto &plane_out = *m_plane_frame.m_planes[i];
        const auto &plane_in = pixel_format.planes[i];
        const Index rows_count = plane_in.rows.size();
        plane_out.m_row_count_entry.set_value(rows_count);
        fix_size(
            plane_out.m_rows,
            rows_count,
            [&plane_out, &update_handler](auto& row_out) {
                plane_out.m_box.append(row_out);
                row_out.m_entry_count_entry.signal_value_changed().connect(
                    update_handler);
            });
        for (Index j = 0; j < rows_count; ++j)
        {
            auto &row_out = *plane_out.m_rows[j];
            const auto &row_in = plane_in.rows[j];
            const Index entries_count = row_in.entries.size();
            row_out.m_entry_count_entry.set_value(entries_count);
            fix_size(
                row_out.m_entries,
                entries_count,
                [&row_out, &update_handler](auto& entry_out) {
                    row_out.append(entry_out);
                    entry_out.m_bit_count_entry.signal_value_changed().connect(
                        update_handler);
                });
            for (Index k = 0; k < entries_count; ++k)
            {
                auto &entry_out = *row_out.m_entries[k];
                const auto& entry_in = row_in.entries[k];
                entry_out.m_bit_count_entry.set_value(
                    entry_in.width.get_position());
            }
        }
    }

    const auto mp_size = pixel_format.macropixel_coding.size;
    m_macropixel_frame.m_columns_entry.set_value(mp_size.x());
    m_macropixel_frame.m_rows_entry.set_value(mp_size.y());
    for (auto &pixel : m_macropixel_frame.m_pixels)
    {
        m_macropixel_frame.m_pixel_grid.remove(*pixel);
    }
    fix_size(
        m_macropixel_frame.m_pixels, mp_size.x() * mp_size.y(), [](auto&) {});
    m_macropixel_frame.reshape();

    for (Index y = 0; y < mp_size.y(); ++y)
    {
        for (Index x = 0; x < mp_size.x(); ++x)
        {
            const Index i = y * mp_size.x() + x;
            auto& pixel_out = *m_macropixel_frame.m_pixels[i];
            const auto &pixel_in = pixel_format.macropixel_coding.pixels[i];
            const Index components_count = pixel_in.components.size();
            fix_size(
                pixel_out.m_samples,
                components_count,
                [&pixel_out, &update_handler](auto& component_out) {
                    pixel_out.m_box.append(component_out);
                    component_out.m_plane_entry.signal_value_changed().connect(
                        update_handler);
                    component_out.m_row_entry.signal_value_changed().connect(
                        update_handler);
                    component_out.m_index_entry.signal_value_changed().connect(
                        update_handler);
                });
            for (Index j = 0; j < components_count; ++j)
            {
                auto &component_out = *pixel_out.m_samples[j];
                const auto &component_in = pixel_in.components[j];
                component_out.m_plane_entry.set_value(component_in.plane_index);
                component_out.m_row_entry.set_value(component_in.row_index);
                component_out.m_index_entry.set_value(component_in.entry_index);
            }
        }
    }
}
/*----------------------------------------------------------------------------*/
sigc::signal<void()>& YUV_tool::Format_chooser_widget::
    signal_pixel_format_changed()
{
    return m_signal_pixel_format_changed;
}
/*----------------------------------------------------------------------------*/
}
