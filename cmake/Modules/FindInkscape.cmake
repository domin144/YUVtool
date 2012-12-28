# - Find Inkscape
# This module looks for Inkscape. This module defines the
# following values:
#  INKSCAPE_EXECUTABLE: the full path to the Inkscape tool.
#  INKSCAPE_FOUND: True if Inkscape has been found.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
# Copyright 2012 Dominik Wójt, domin144@o2.pl
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)


FIND_PROGRAM(INKSCAPE_EXECUTABLE
  inkscape
)

# handle the QUIETLY and REQUIRED arguments and set WGET_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Inkscape DEFAULT_MSG INKSCAPE_EXECUTABLE)

MARK_AS_ADVANCED( INKSCAPE_EXECUTABLE )

