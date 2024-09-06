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
#ifndef RESOLUTION_AND_FORMAT_DIALOG_H
#define RESOLUTION_AND_FORMAT_DIALOG_H

#include <format_chooser_widget.h>
#include <resolution_chooser_widget.h>
#include <yuv/Pixel_format.h>

#include <gtkmm/dialog.h>

namespace YUV_tool {

class Resolution_and_format_dialog : public Gtk::Dialog
{
private:
    Gtk::Box m_box {Gtk::Orientation::VERTICAL};

    Format_chooser_widget m_format_widget;
    Resolution_chooser_widget m_resolution_widget;

    sigc::signal<void()> m_signal_pixel_format_changed;
    sigc::signal<void()> m_signal_resolution_changed;

public:
    Resolution_and_format_dialog(Gtk::Window& parent);

    Pixel_format get_pixel_format() const;
    void set_pixel_format(const Pixel_format &pixel_format);

    Vector<Unit::pixel> get_resolution() const;
    void set_resolution(const Vector<Unit::pixel> &resolution);

    void update_ok_button_state();

    sigc::signal<void()> &signal_pixel_format_changed();
    sigc::signal<void()> &signal_resolution_changed();
};

} /* namespace YUV_tool */

#endif /* RESOLUTION_AND_FORMAT_DIALOG_H */
