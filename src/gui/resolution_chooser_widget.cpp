/*
 * Copyright 2019 Dominik Wójt
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
#include <resolution_chooser_widget.h>

#include <gtkmm/adjustment.h>

namespace YUV_tool {
/*----------------------------------------------------------------------------*/
Resolution_chooser_widget::Resolution_chooser_widget() :
    m_box(Gtk::Orientation::VERTICAL),
    m_x_box(Gtk::Orientation::HORIZONTAL),
    m_x_label("resolution x:"),
    m_x_entry(Gtk::Adjustment::create(0, 0, 1 << 16)),
    m_y_box(Gtk::Orientation::HORIZONTAL),
    m_y_label("resolution y:"),
    m_y_entry(Gtk::Adjustment::create(0, 0, 1 << 16))
{
    set_child(m_box);

    m_box.append(m_x_box);
    m_box.append(m_y_box);

    m_x_box.append(m_x_label);
    m_x_box.append(m_x_entry);

    m_y_box.append(m_y_label);
    m_y_box.append(m_y_entry);

    auto on_update_slot =
        sigc::mem_fun(*this, &Resolution_chooser_widget::on_update);
    m_x_entry.signal_value_changed().connect(on_update_slot);
    m_y_entry.signal_value_changed().connect(on_update_slot);
}
/*----------------------------------------------------------------------------*/
Vector<Unit::pixel> Resolution_chooser_widget::get_resolution() const
{
    return {m_x_entry.get_value_as_int(), m_y_entry.get_value_as_int()};
}
/*----------------------------------------------------------------------------*/
void Resolution_chooser_widget::set_resolution(
    const Vector<Unit::pixel>& resolution)
{
    m_update_in_progress = true;
    m_x_entry.set_value(resolution.x());
    m_y_entry.set_value(resolution.y());
    m_update_in_progress = false;
    signal_resolution_changed().emit();
}
/*----------------------------------------------------------------------------*/
sigc::signal<void ()> &Resolution_chooser_widget::signal_resolution_changed()
{
    return m_signal_resolution_changed;
}
/*----------------------------------------------------------------------------*/
void Resolution_chooser_widget::on_update()
{
    if (m_update_in_progress)
        return;

    signal_resolution_changed().emit();
}
/*----------------------------------------------------------------------------*/
} /* namespace YUV_tool */
