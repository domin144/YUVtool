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
#ifndef RESOLUTION_CHOOSER_WIDGET_H
#define RESOLUTION_CHOOSER_WIDGET_H

#include <yuv/Coordinates.h>

#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/spinbutton.h>

namespace YUV_tool {

class Resolution_chooser_widget : public Gtk::Frame
{
private:
    Gtk::Box m_box;

    Gtk::Box m_x_box;
    Gtk::Label m_x_label;
    Gtk::SpinButton m_x_entry;

    Gtk::Box m_y_box;
    Gtk::Label m_y_label;
    Gtk::SpinButton m_y_entry;

    sigc::signal<void()> m_signal_resolution_changed;

    bool m_update_in_progress {false};

public:
    Resolution_chooser_widget();

    Vector<Unit::pixel> get_resolution() const;
    void set_resolution(const Vector<Unit::pixel> &resolution);
    sigc::signal<void()> &signal_resolution_changed();

private:
    void on_update();
};

} /* namespace YUV_tool */

#endif /* RESOLUTION_CHOOSER_WIDGET_H */
