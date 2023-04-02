sphinx = find_program('sphinx-build')

# configured documentation tools and intermediate build results
docs_build_dir = meson.current_build_dir() / '_build'

# Sphinx cache with pickled ReST documents
docs_cache_dir = meson.current_build_dir() / '_doctrees'

# HTML output directory
docs_html_dir = meson.current_build_dir() / 'html'

docs_conf_data = configuration_data()
docs_conf_data.set('SPHINX_THEME', 'default')
docs_conf_data.set('SPHINX_THEME_DIR', '')
docs_conf = configure_file(
    input : 'conf.py.in',
    output : 'conf.py',
    configuration : docs_conf_data
)

docs_sources = [
    'bibliography.rst',
    'gui.rst',
    'images/entries_in_plane.svg',
    'images/format_chooser_dialog.svg',
    'images/YUV_view_window.svg',
    'implementation.rst',
    'index.rst',
    'introduction.rst',
    'pixel_format_representation.rst',
    'xml_companion_format.rst',
]

custom_target(
    'developers_documentation',
    input : docs_sources,
    output : 'html',
    command : [
        sphinx,
        '-q',
        '-b', 'html',
        '-c', meson.current_build_dir(),
        '-d', docs_cache_dir,
        meson.current_source_dir(),
        docs_html_dir],
    build_by_default: true)