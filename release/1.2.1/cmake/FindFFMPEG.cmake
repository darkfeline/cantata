find_package(PkgConfig)

pkg_check_modules(PC_LIBAVCODEC QUIET libavcodec)
pkg_check_modules(PC_LIBAVFORMAT QUIET libavformat)
pkg_check_modules(PC_LIBAVUTIL QUIET libavutil)

find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
          HINTS ${PC_LIBAVCODEC_INCLUDEDIR} ${PC_LIBAVCODEC_INCLUDE_DIRS})

find_library(LIBAVCODEC_LIBRARY NAMES avcodec avcodec-52
             HINTS ${PC_LIBAVCODEC_LIBDIR} ${PC_LIBAVCODEC_LIBRARY_DIRS})
find_library(LIBAVFORMAT_LIBRARY NAMES avformat avformat-52
             HINTS ${PC_LIBAVFORMAT_LIBDIR} ${PC_LIBAVFORMAT_LIBRARY_DIRS})
find_library(LIBAVUTIL_LIBRARY NAMES avutil avutil-50
             HINTS ${PC_LIBAVUTIL_LIBDIR} ${PC_LIBAVUTIL_LIBRARY_DIRS})

set(FFMPEG_LIBRARIES ${LIBAVCODEC_LIBRARY} ${LIBAVFORMAT_LIBRARY} ${LIBAVUTIL_LIBRARY})
set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFMPEG DEFAULT_MSG LIBAVCODEC_LIBRARY LIBAVFORMAT_LIBRARY LIBAVUTIL_LIBRARY FFMPEG_INCLUDE_DIR)
mark_as_advanced(LIBAVCODEC_LIBRARY LIBAVFORMAT_LIBRARY LIBAVUTIL_LIBRARY FFMPEG_INCLUDE_DIR)