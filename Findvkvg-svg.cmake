find_path(vkvg-svg_INCLUDE_DIR vkvg-svg.h)

find_library(vkvg-svg_LIBRARY NAMES vkvg-svg)

# handle the QUIETLY and REQUIRED arguments and set VKVG_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(vkvg-svg DEFAULT_MSG
  vkvg-svg_LIBRARY  vkvg-svg_INCLUDE_DIR)

if(vkvg-svg_FOUND)
  set(vkvg-svg_LIBRARIES ${vkvg-svg_LIBRARY})
endif()

mark_as_advanced(vkvg-svg_INCLUDE_DIR vkvg-svg_LIBRARY vkvg-svg_LIBRARIES)
