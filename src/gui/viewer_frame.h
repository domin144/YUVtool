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
#ifndef VIEWER_FRAME_H
#define VIEWER_FRAME_H

#ifdef GTK4_PORT_DONE
#include <scroll_adapter.h>
#include <drawer_gl.h>
#endif /* GTK4_PORT_DONE */
#include <resolution_and_format_dialog.h>
#include <yuv/Yuv_file.h>

#include <giomm/simpleactiongroup.h>
#include <glibmm/refptr.h>
#include <gtkmm/box.h>
#include <gtkmm/builder.h>
#include <gtkmm/button.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/window.h>

namespace YUV_tool {

class Viewer_frame : public Gtk::Window
{
public:
    Viewer_frame();
    virtual ~Viewer_frame();

private:
    void on_action_file_quit();
    void on_action_show_size();
    void on_action_file_open();
    void on_file_dialog_finish(int response_id);
    void on_format_dialog_finish(int response_id);
    void on_action_file_close();
    void on_action_help_info();
    void on_action_help_about();
    void on_message_dialog_response(int);
    bool on_action_draw_event(const Glib::RefPtr<Gdk::GLContext> &context);
    void on_action_gl_context_init();
    void on_action_gl_context_deinit();
    void draw_triangle();
    void draw_frame();

    Glib::RefPtr<Gio::SimpleActionGroup> m_file_action_group;
    Glib::RefPtr<Gio::SimpleActionGroup> m_help_action_group;
    Glib::RefPtr<Gtk::Builder> m_builder;
    Gtk::Box m_box;
    Yuv_file m_yuv_file;
#ifdef GTK4_PORT_DONE
    Drawer_gl m_drawer_gl;
    Scroll_adapter m_scroll_adapter;
#endif /* GTK4_PORT_DONE */
    Glib::RefPtr<Gtk::MessageDialog> m_message_dialog;
    Glib::RefPtr<Gtk::FileChooserDialog> m_file_dialog;
    Glib::RefPtr<Resolution_and_format_dialog> m_format_dialog;
};

} /* namespace YUV_tool */

#endif // VIEWER_FRAME_H
