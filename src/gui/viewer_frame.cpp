#include <gui/viewer_frame.h>
#include <gui/format_chooser_dialog.h>

#include <SFML/OpenGL.hpp>
#include <gtkmm/stock.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/socket.h>
#include <gtkmm/filechooserdialog.h>
#include <iostream>
#include <cstdlib>

namespace YUV_tool {

Viewer_frame::Viewer_frame() :
    m_box(Gtk::ORIENTATION_VERTICAL)
{
    /* File menu action group */
    m_action_group = Gtk::ActionGroup::create();
    m_action_group->add(
        Gtk::Action::create( "menu_file", "_File" ) );
    m_action_group->add(
        Gtk::Action::create( "open_file", Gtk::Stock::OPEN ),
        sigc::mem_fun( *this, &Viewer_frame::on_action_file_open ) );
    m_action_group->add(
        Gtk::Action::create( "close_file", Gtk::Stock::CLOSE ),
        sigc::mem_fun( *this, &Viewer_frame::on_action_file_close ) );
    m_action_group->add(
        Gtk::Action::create( "quit", Gtk::Stock::QUIT ),
        sigc::mem_fun( *this, &Viewer_frame::on_action_file_quit ) );
    m_action_group->add(
        Gtk::Action::create( "show_size", Gtk::Stock::ABOUT,
            "Show size of the drowing area" ),
        sigc::mem_fun( *this, &Viewer_frame::on_action_show_size ) );

    /* Help menu action group */
    m_action_group->add(
        Gtk::Action::create( "menu_help", "_Help" ) );
    m_action_group->add(
        Gtk::Action::create( "help_info", Gtk::Stock::HELP ),
        sigc::mem_fun( *this, &Viewer_frame::on_action_help_info ) );
    m_action_group->add(
        Gtk::Action::create( "about_info", Gtk::Stock::ABOUT ),
        sigc::mem_fun( *this, &Viewer_frame::on_action_help_about ) );


    m_ui_manager = Gtk::UIManager::create();
    m_ui_manager->insert_action_group( m_action_group );
    add_accel_group( m_ui_manager->get_accel_group() );
    Glib::ustring ui_info =
        R"(
        <ui>
            <menubar name='menu_bar'>
                <menu action='menu_file'>
                    <menuitem action='open_file'/>
                    <menuitem action='close_file'/>
                    <separator/>
                    <menuitem action='show_size'/>
                    <separator/>
                    <menuitem action='quit'/>
                </menu>
                <menu action='menu_help'>
                    <menuitem action='help_info'/>
                    <separator/>
                    <menuitem action='about_info'/>
                </menu>
            </menubar>
            <toolbar name='tool_bar'>
                <toolitem action='open_file'/>
                <toolitem action='close_file'/>
                <toolitem action='show_size'/>
                <toolitem action='quit'/>
            </toolbar>
        </ui>
        )";
    m_ui_manager->add_ui_from_string( ui_info );

    set_default_size( 400, 250 );

    Gtk::Widget *menu_bar = m_ui_manager->get_widget("/menu_bar");
    m_box.pack_start( *menu_bar, Gtk::PACK_SHRINK );

    Gtk::Widget *tool_bar = m_ui_manager->get_widget("/tool_bar");
    m_box.pack_start( *tool_bar, Gtk::PACK_SHRINK );

    m_scroll_adapter.signal_update_viewport().connect(
            sigc::mem_fun(*this, &Viewer_frame::on_action_size_allocation));
    m_scroll_adapter.signal_update_drawing().connect(
            sigc::mem_fun(*this, &Viewer_frame::on_action_draw_event));

    m_scroll_adapter.set_internal_size(Vector<Unit::pixel>(300, 300));
    m_box.pack_start(m_scroll_adapter, Gtk::PACK_EXPAND_WIDGET);

    add(m_box);

    show_all();
}
//------------------------------------------------------------------------------
Viewer_frame::~Viewer_frame()
{ }
//------------------------------------------------------------------------------
void Viewer_frame::on_action_file_quit()
{
    hide();
}
//------------------------------------------------------------------------------
namespace {
void print_gdk_window(std::ostream &os, const std::string &name, const Glib::RefPtr<Gdk::Window> window)
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
    catch(std::runtime_error &)
    {
        std::cerr << "failed to open file: " << file_name << '\n';
    }

    {
        Format_chooser_dialog format_dialog(
                    *this,
                    m_yuv_file.get_pixel_format());

        const int result = format_dialog.run();

        switch(result)
        {
        case Gtk::RESPONSE_OK:
            m_yuv_file.set_pixel_format(format_dialog.get_pixel_format());
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
void Viewer_frame::on_action_size_allocation()
{
//    std::cerr << "Viewer_frame::on_action_size_allocation()" << std::endl;
    const Gdk::Rectangle visible_area =
            m_scroll_adapter.get_visible_area();
    const Vector<Unit::pixel> internal_size =
            m_scroll_adapter.get_internal_size();
    const int width = std::min(visible_area.get_width(), internal_size.x());
    const int height = std::min(visible_area.get_height(), internal_size.y());
    const int x0 = visible_area.get_x();
    const int y0 = visible_area.get_y();
    const int bottom_margin = visible_area.get_height() - height;

    if(!m_scroll_adapter.set_active(true))
        return;

    glViewport(0, bottom_margin, width, height);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho(x0, x0 + width, y0 + height, y0, -1, 1);

    glClearColor(0.0f, 0.5f, 0.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);

    if(!m_scroll_adapter.set_active(false))
        return;
}
//------------------------------------------------------------------------------
void Viewer_frame::draw_triangle()
{
    if(!m_scroll_adapter.set_active(true))
        return;

    glClearColor( 0.0, 0.0, 0.5, 0.0 );
    glClear( GL_COLOR_BUFFER_BIT );

    glColor3f( 1.0, 1.0, 1.0 );
    glBegin( GL_TRIANGLES );
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 300.0, 0.0);
    glVertex3f(300.0, 0.0, 0.0);
    glEnd();

    glColor3f( 0.0, 1.0, 0.0 );
    glBegin( GL_TRIANGLES );
    glVertex3f(0.0, 300.0, 0.0);
    glVertex3f(300.0, 300.0, 0.0);
    glVertex3f(300.0, 0.0, 0.0);
    glEnd();

    glFlush();
    m_scroll_adapter.display();

    if(!m_scroll_adapter.set_active(false))
        return;
}
//------------------------------------------------------------------------------
void Viewer_frame::draw_frame()
{
    if(!m_scroll_adapter.set_active(true))
        return;

    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glClear( GL_COLOR_BUFFER_BIT );
    glEnable( GL_TEXTURE_2D );
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );

    m_drawer_gl.draw( m_yuv_file, 0, m_scroll_adapter );

    glFlush();
    m_scroll_adapter.display();

    if(!m_scroll_adapter.set_active(false))
        return;
}
//------------------------------------------------------------------------------
void Viewer_frame::on_action_draw_event()
{
//    std::cerr << "Viewer_frame::on_action_draw_event()" << std::endl;
    draw_triangle();
    //draw_frame();
}

} /* namespace YUV_tool */
