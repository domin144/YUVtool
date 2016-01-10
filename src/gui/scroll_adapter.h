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
#ifndef SCROLL_ADAPTER_H
#define SCROLL_ADAPTER_H

#include <yuv/Coordinates.h>
#include <gui/SFMLWidget.h>

#include <gtkmm/grid.h>
#include <gtkmm/scrollbar.h>
#include <gtkmm/adjustment.h>

namespace YUV_tool {

class Scroll_adapter : public Gtk::Grid
{
private:
    Glib::RefPtr<Gtk::Adjustment> m_x_adjustment;
    Glib::RefPtr<Gtk::Adjustment> m_y_adjustment;
    Gtk::Scrollbar m_x_scrollbar;
    Gtk::Scrollbar m_y_scrollbar;
    Vector<Unit::pixel> m_internal_size;
    SFML_widget m_drawing_area;
    sigc::signal<void> m_signal_update_viewport;
    sigc::signal<void> m_signal_update_drawing;

public:
    Scroll_adapter();
    void set_internal_size(const Vector<Unit::pixel> &size);
    const Vector<Unit::pixel> &get_internal_size() const;
    /* might return area larger than internal size */
    Gdk::Rectangle get_visible_area();
    sigc::signal<void> &signal_update_viewport();
    sigc::signal<void> &signal_update_drawing();
    bool set_active(bool active);
    void display();

private:
    void update_allocation();
    void on_scroll();
    bool kick_signal_update_drawing(const Cairo::RefPtr<Cairo::Context>);
};

} /* namespace YUV_tool */

#endif // SCROLL_ADAPTER_H
