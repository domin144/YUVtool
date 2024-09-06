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
#ifndef FORMAT_CHOOSER_DIALOG_H
#define FORMAT_CHOOSER_DIALOG_H

#include <color_space_frame.h>

#include <yuv/Pixel_format.h>

#include <gtkmm/combobox.h>
#include <gtkmm/dialog.h>
#include <gtkmm/frame.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/notebook.h>
#include <gtkmm/spinbutton.h>

namespace YUV_tool {

class Format_chooser_widget : public Gtk::Frame
{
public:
    Format_chooser_widget();
    virtual ~Format_chooser_widget();
    const Pixel_format get_pixel_format() const;
    void set_pixel_format(const Pixel_format& pixel_format);
    sigc::signal<void()>& signal_pixel_format_changed();

private:
    struct Entry_configurator : Gtk::Box
    {
        Entry_configurator();

        Gtk::Label m_entry_label;
        Gtk::SpinButton m_bit_count_entry;
        Gtk::Label m_bits_label;
    };

    struct Row_in_plane_configurator : Gtk::Box
    {
        Row_in_plane_configurator();

        Gtk::Box m_entry_count_box;
        Gtk::Label m_entry_count_label;
        Gtk::SpinButton m_entry_count_entry;

        std::vector<std::unique_ptr<Entry_configurator>> m_entries;
    };

    struct Plane_configurator : Gtk::Frame
    {
        Plane_configurator();

        Gtk::Box m_box;

        Gtk::Box m_row_count_box;
        Gtk::Label m_row_count_label;
        Gtk::SpinButton m_row_count_entry;

        std::vector<std::unique_ptr<Row_in_plane_configurator>> m_rows;
    };

    struct Plane_frame : Gtk::Frame
    {
        Plane_frame();

        Gtk::Box m_box;

        Gtk::Box m_planes_count_box;
        Gtk::Label m_planes_count_label;
        Gtk::SpinButton m_planes_count_entry;

        Gtk::Box m_planes_box;
        std::vector<std::unique_ptr<Plane_configurator>> m_planes;
    };

    struct Sample_configurator : Gtk::Box
    {
        Sample_configurator();

        Gtk::Label m_sample_label;
        Gtk::Label m_plane_label;
        Gtk::SpinButton m_plane_entry;
        Gtk::Label m_row_label;
        Gtk::SpinButton m_row_entry;
        Gtk::Label m_index_label;
        Gtk::SpinButton m_index_entry;
    };

    struct Pixel_configurator : Gtk::Frame
    {
        Pixel_configurator();

        Gtk::Box m_box;
        std::vector<std::unique_ptr<Sample_configurator>> m_samples;
    };

    struct Macropixel_frame : Gtk::Frame
    {
        Macropixel_frame();
        void reshape();

        Gtk::Box m_box;

        Gtk::Box m_parameters_box;
        Gtk::Label m_rows_label;
        Gtk::SpinButton m_rows_entry;
        Gtk::Label m_columns_label;
        Gtk::SpinButton m_columns_entry;

        Gtk::Grid m_pixel_grid;
        std::vector<std::unique_ptr<Pixel_configurator> > m_pixels;
    };

    struct Pixel_format_column_record : Gtk::TreeModel::ColumnRecord
    {
        Gtk::TreeModelColumn<Glib::ustring> label;
        Gtk::TreeModelColumn<const Pixel_format *> pointer;

        Pixel_format_column_record();
    };

    void update();
    void update_entries(const Pixel_format &pixel_format);

    Gtk::Box m_box;

    Gtk::ComboBox m_predefined_choice;
    Gtk::Notebook m_notebook;
    Color_space_frame m_color_space_frame;
    Plane_frame m_plane_frame;
    Macropixel_frame m_macropixel_frame;

    Glib::RefPtr<Gtk::ListStore> m_predefined_list_store;
    Pixel_format_column_record m_predefined_column_record;

    sigc::signal<void()> m_signal_pixel_format_changed;

    bool m_update_in_progress{false};
};

}

#endif /* FORMAT_CHOOSER_DIALOG_H */
