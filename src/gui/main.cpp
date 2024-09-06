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

#include <gtkmm/application.h>
#include <gtkmm/window.h>

// #include "viewer_frame.h"

int main(int argc, char* argv[])
{
    // using namespace YUV_tool;

    Glib::RefPtr<Gtk::Application> app =
        Gtk::Application::create("org.yuvtool");

    return app->make_window_and_run<Gtk::Window>(argc, argv);
}
