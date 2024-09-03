/* 
 * Copyright 2015, 2016 Dominik Wójt
 * Copyright 2015 Remigiusz Wilmont
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

#include <resolution_and_format_dialog.h>
#include <viewer_frame.h>
#include <yuv/Errors.h>

#include <giomm/menu.h>
#include <giomm/simpleactiongroup.h>
#include <gtkmm/builder.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/menubar.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/popovermenu.h>
#include <gtkmm/socket.h>
#include <gtkmm/stock.h>
#include <iostream>

namespace YUV_tool {

Viewer_frame::Viewer_frame() :
    m_box(Gtk::ORIENTATION_VERTICAL)
{
    m_drawer_gl.attach_yuv_file(&m_yuv_file);

    /* File menu action group */
    m_file_action_group = Gio::SimpleActionGroup::create();
    m_file_action_group->add_action(
        "open", sigc::mem_fun(*this, &Viewer_frame::on_action_file_open));
    m_file_action_group->add_action(
        "close", sigc::mem_fun(*this, &Viewer_frame::on_action_file_close));
    m_file_action_group->add_action(
        "show_size", sigc::mem_fun(*this, &Viewer_frame::on_action_show_size));
    m_file_action_group->add_action(
        "quit", sigc::mem_fun(*this, &Viewer_frame::on_action_file_quit));
    insert_action_group("file", m_file_action_group);

    /* Help menu action group */
    m_help_action_group = Gio::SimpleActionGroup::create();
    m_help_action_group->add_action(
        "info", sigc::mem_fun(*this, &Viewer_frame::on_action_help_info));
    m_help_action_group->add_action(
        "about", sigc::mem_fun(*this, &Viewer_frame::on_action_help_about));
    insert_action_group("help", m_help_action_group);

    m_builder = Gtk::Builder::create();
    Glib::ustring ui_info =
        R"(
        <interface>
            <menu id='menubar'>
                <submenu>
                    <attribute name='label' translatable='yes'>_File</attribute>
                    <section>
                        <item>
                            <attribute name='label' translatable='yes'>_Open</attribute>
                            <attribute name='action'>file.open</attribute>
                        </item>
                    </section>
                    <section>
                        <item>
                            <attribute name='label' translatable='yes'>_Close</attribute>
                            <attribute name='action'>file.close</attribute>
                        </item>
                    </section>
                    <section>
                        <item>
                            <attribute name='label' translatable='yes'>_Show size</attribute>
                            <attribute name='action'>file.show_size</attribute>
                        </item>
                    </section>
                    <section>
                        <item>
                            <attribute name='label' translatable='yes'>_Quit</attribute>
                            <attribute name='action'>file.quit</attribute>
                        </item>
                    </section>
                </submenu>
                <submenu>
                    <attribute name='label' translatable='yes'>_Help</attribute>
                    <section>
                        <item>
                            <attribute name='label' translatable='yes'>_Help</attribute>
                            <attribute name='action'>help.info</attribute>
                        </item>
                    </section>
                    <section>
                        <item>
                            <attribute name='label' translatable='yes'>_About</attribute>
                            <attribute name='action'>help.about</attribute>
                        </item>
                    </section>
                </submenu>
            </menu>
        </interface>
        )";

    // <toolbar name='tool_bar'>
    //     <toolitem action='open_file'/>
    //     <toolitem action='close_file'/>
    //     <toolitem action='show_size'/>
    //     <toolitem action='quit'/>
    // </toolbar>
    m_builder->add_from_string(ui_info);

    set_default_size(400, 250);

    Glib::RefPtr<Gio::Menu> menu =
        Glib::RefPtr<Gio::Menu>::cast_dynamic(m_builder->get_object("menubar"));
    m_box.pack_start(*Gtk::make_managed<Gtk::MenuBar>(menu), Gtk::PACK_SHRINK);

    // Gtk::Widget *tool_bar = m_ui_manager->get_widget("/tool_bar");
    // m_box.pack_start( *tool_bar, Gtk::PACK_SHRINK );

    m_scroll_adapter.get_drawing_area().signal_render().connect(
                sigc::mem_fun(*this, &Viewer_frame::on_action_draw_event));
    m_scroll_adapter.get_drawing_area().signal_realize().connect(
                sigc::mem_fun(*this, &Viewer_frame::on_action_gl_context_init));

    m_scroll_adapter.set_internal_size(Vector<Unit::pixel>(256, 128));
    m_box.pack_start(m_scroll_adapter, Gtk::PACK_EXPAND_WIDGET);

    add(m_box);

    show_all();
}
//------------------------------------------------------------------------------
Viewer_frame::~Viewer_frame()
{
    on_action_gl_context_deinit();
}
//------------------------------------------------------------------------------
void Viewer_frame::on_action_file_quit()
{
    hide();
}
//------------------------------------------------------------------------------
namespace {
void print_gdk_window(
        std::ostream &os,
        const std::string &name,
        const Glib::RefPtr<Gdk::Window> window)
{
    int x, y;
    window->get_position(x, y);
    os
            << name << " : "
            << x << 'x' << y << " + "
            << window->get_width() << 'x' << window->get_height() << '\n';
}
void check_parent_child(
        std::ostream &os,
        const Glib::RefPtr<Gdk::Window> parent,
        const Glib::RefPtr<Gdk::Window> child)
{
    bool result =
            parent == child->get_parent();
    os << (result ? "is parent" : "is not parent") << '\n';
}
}
//------------------------------------------------------------------------------
void Viewer_frame::on_action_show_size()
{
    Gtk::MessageDialog dialog( *this, "Size of the drawing area." );

    std::stringstream ss;

//    const Gtk::Allocation allocation =
//            m_drawing_area.get_allocation();
//    ss
//        << "The drawing area allocation is "
//        << allocation.get_x() << 'x' << allocation.get_y() << " + "
//        << allocation.get_width() << 'x' << allocation.get_height() << '\n';

//    int internal_width, internal_height;
//    m_scroll_adapter.get_internal_size( internal_width, internal_height );
//    ss << "The internal size of the scrolled area is " <<
//        internal_width << 'x' <<
//        internal_height << '\n';

//    const Gdk::Rectangle visible_area =
//            m_scroll_adapter.get_visible_area();
//    ss
//        << "The visible area rectangle is "
//        << visible_area.get_x() << 'x' << visible_area.get_y() << " + "
//        << visible_area.get_width() << 'x' << visible_area.get_height() << '\n';

    print_gdk_window(ss, "frame", get_window());
    check_parent_child(ss, get_window(), m_scroll_adapter.get_window());
//    check_parent_child(ss, get_window(), m_scrolled_window.get_window());
//    print_gdk_window(ss, "scrolled_window", m_scrolled_window.get_window());
//    check_parent_child(ss, m_scrolled_window.get_window(), m_viewport.get_window());
//    print_gdk_window(ss, "viewport", m_viewport.get_window());
//    check_parent_child(ss, m_viewport.get_window(), m_viewport.get_view_window());
//    print_gdk_window(ss, "viewport_view_window", m_viewport.get_view_window());
//    check_parent_child(ss, m_viewport.get_view_window(), m_refGdkWindow);
//    ss << (m_viewport.get_view_window() == m_refGdkWindow ? "are equal\n" : "are different\n");
//    print_gdk_window(ss, "m_refGdkWindow", m_refGdkWindow);


//    check_parent_child(ss, m_scroll_adapter.m_viewport.get_window(), m_scroll_adapter.m_viewport.get_view_window());
//    print_gdk_window(ss, "viewport.view", m_scroll_adapter.m_viewport.get_view_window());
//    check_parent_child(ss, m_scroll_adapter.m_viewport.get_view_window(), m_scroll_adapter.m_fixed.get_window());
//    print_gdk_window(ss, "fixed", m_scroll_adapter.m_fixed.get_window());
//    check_parent_child(ss, m_scroll_adapter.m_fixed.get_window(), m_drawing_area.get_window());
//    print_gdk_window(ss, "sfml_widget", m_drawing_area.get_window());
//    check_parent_child(ss, m_drawing_area.get_window(), m_drawing_area.m_refGdkWindow);
//    ss << (m_drawing_area.get_window() == m_drawing_area.m_refGdkWindow ? "are equal\n" : "are different\n");
//    print_gdk_window(ss, "render_area", m_drawing_area.m_refGdkWindow);

//    print_gdk_window(ss, "viewport.bin", m_scroll_adapter.m_viewport.get_bin_window());

//    ss
//            << "renderwindow.getPosition() : "
//            << m_sfml_window.getPosition().x << 'x'
//            << m_sfml_window.getPosition().y << '\n';

    dialog.set_secondary_text( ss.str() );

    dialog.run();
}
//------------------------------------------------------------------------------
void Viewer_frame::on_action_file_open()
{
    std::string file_name;
    {
        Gtk::FileChooserDialog file_dialog(
                    *this,
                    "Choose YUV file.",
                    Gtk::FILE_CHOOSER_ACTION_OPEN);

        file_dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
        file_dialog.add_button("Select", Gtk::RESPONSE_OK);

        const int result = file_dialog.run();

        switch(result)
        {
        case Gtk::RESPONSE_OK:
            file_name = file_dialog.get_filename();
            break;
        case Gtk::RESPONSE_CANCEL:
            return;
        default:
            std::cerr << "unknown responce of file chooser dialog\n";
            return;
        }
    }

    try
    {
        m_yuv_file.open(file_name);
    }
    catch(std::runtime_error &e)
    {
        std::cerr << "failed to open file: " << file_name << ", " << e.what()
                  << '\n';
    }

    {
        Resolution_and_format_dialog dialog(*this);
        dialog.set_pixel_format(m_yuv_file.get_pixel_format());
        dialog.set_resolution(m_yuv_file.get_resolution());

        const int result = dialog.run();

        switch(result)
        {
        case Gtk::RESPONSE_OK:
            m_yuv_file.set_resolution(dialog.get_resolution());
            m_yuv_file.set_pixel_format(dialog.get_pixel_format());
            m_scroll_adapter.set_internal_size(m_yuv_file.get_resolution());
            break;
        case Gtk::RESPONSE_CANCEL:
            return;
        default:
            std::cerr << "unknown responce of format chooser dialog\n";
            return;
        }
    }
}
//------------------------------------------------------------------------------
void Viewer_frame::on_action_file_close()
{
    if(m_yuv_file.is_open())
    {
        m_yuv_file.close();
    }
    else
    {
        std::cerr << "tried to close file, while none is open\n";
    }
}
//------------------------------------------------------------------------------
void Viewer_frame::on_action_help_info()
{
    std::cerr << "Not implemented yet. Sorry.\n";
    Gtk::MessageDialog dialog( *this, "Help" );

    std::stringstream ss;

    ss << "Not implemented yet. Sorry";

    dialog.set_secondary_text( ss.str() );
    int result = dialog.run();

    switch(result)
    {
        case(Gtk::RESPONSE_OK):
        {
          std::cout << "DEBUG: OK clicked." << std::endl;
          break;
        }
        case(Gtk::RESPONSE_CANCEL):
        {
          std::cout << "DEBUG: Cancel clicked." << std::endl;
          break;
        }
        default:
        {
          std::cout << "DEBUG: Unexpected button clicked." << std::endl;
          break;
        }
    }
}
//------------------------------------------------------------------------------
void Viewer_frame::on_action_help_about()
{
    std::cerr << "Not implemented yet. Sorry.\n";
    Gtk::MessageDialog dialog( *this, "About..." );

    std::stringstream ss;

    ss << "Not implemented yet. Sorry";

    dialog.set_secondary_text( ss.str() );
    dialog.run();
}
//------------------------------------------------------------------------------
void Viewer_frame::draw_frame()
{
    try
    {
        const Gdk::Rectangle visible_area =
                m_scroll_adapter.get_visible_area();
        const Coordinates<Unit::pixel, Reference_point::scaled_picture>
                visible_area_start(
                    visible_area.get_x(),
                    visible_area.get_y());
        const Vector<Unit::pixel> visible_area_size(
                    visible_area.get_width(),
                    visible_area.get_height());

        {
            static int i = 0;
            std::cerr << __func__ << ' ' << i++ << std::endl;
            std::cerr << '(' << visible_area.get_x() << ", "
                      << visible_area.get_y() << ") + ("
                      << visible_area.get_width() << ", "
                      << visible_area.get_height() << ')' << std::endl;
        }

        m_drawer_gl.draw(
                    0,
                    make_rectangle(visible_area_start, visible_area_size),
                    1.0);
    }
    catch(...)
    {
        std::cerr << "failed to draw picture";
        my_assert(
                    false,
                    "TODO: The picture might have changed or be deleted. "
                    "Try to reload a file and, if fail, close it");
    }
}
//------------------------------------------------------------------------------
bool Viewer_frame::on_action_draw_event(const Glib::RefPtr<Gdk::GLContext>&)
{
//    std::cerr << "Viewer_frame::on_action_draw_event()" << std::endl;
    draw_frame();
    return true;
}
/*----------------------------------------------------------------------------*/
void Viewer_frame::on_action_gl_context_init()
{
    m_scroll_adapter.get_drawing_area().make_current();
    /* At least for now, the size of the buffer must be sufficient to hold all
     * the visible tiles. */
    m_drawer_gl.initialize(256);
}
/*----------------------------------------------------------------------------*/
void Viewer_frame::on_action_gl_context_deinit()
{
    m_scroll_adapter.get_drawing_area().make_current();
    m_drawer_gl.deinitialize();
}
/*----------------------------------------------------------------------------*/
} /* namespace YUV_tool */
