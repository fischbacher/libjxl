# Copyright (c) the JPEG XL Project Authors. All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

set(JPEGXL_EXTRAS_SOURCES
  extras/codec.cc
  extras/codec.h
  extras/dec/color_description.cc
  extras/dec/color_description.h
  extras/dec/color_hints.cc
  extras/dec/color_hints.h
  extras/dec/decode.cc
  extras/dec/decode.h
  extras/dec/pgx.cc
  extras/dec/pgx.h
  extras/dec/pnm.cc
  extras/dec/pnm.h
  extras/enc/pgx.cc
  extras/enc/pgx.h
  extras/enc/pnm.cc
  extras/enc/pnm.h
  extras/hlg.cc
  extras/hlg.h
  extras/packed_image.h
  extras/packed_image_convert.cc
  extras/packed_image_convert.h
  extras/time.cc
  extras/time.h
  extras/tone_mapping.cc
  extras/tone_mapping.h
)

set(JPEGXL_EXTRAS_DEC_SOURCES
  extras/dec/color_description.cc
  extras/dec/color_description.h
  extras/dec/color_hints.cc
  extras/dec/color_hints.h
  extras/dec/decode.cc
  extras/dec/decode.h
  extras/dec/pgx.cc
  extras/dec/pgx.h
  extras/dec/pnm.cc
  extras/dec/pnm.h
  extras/packed_image.h
)

add_library(jxl_extras_dec-obj OBJECT "${JPEGXL_EXTRAS_DEC_SOURCES}")
target_compile_options(jxl_extras_dec-obj PRIVATE "${JPEGXL_INTERNAL_FLAGS}")
target_compile_definitions(jxl_extras_dec-obj PRIVATE -DJXL_EXPORT=)
set_property(TARGET jxl_extras_dec-obj PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(jxl_extras_dec-obj PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/include
  $<TARGET_PROPERTY:hwy,INTERFACE_INCLUDE_DIRECTORIES>
)
set(JXL_EXTRAS_DEC_INTERNAL_LIBRARIES)
set(JXL_EXTRAS_DEC_PUBLIC_COMPILE_DEFINITIONS)

# We only define a static library for jxl_extras since it uses internal parts
# of jxl library which are not accessible from outside the library in the
# shared library case.
add_library(jxl_extras-static STATIC EXCLUDE_FROM_ALL
  "${JPEGXL_EXTRAS_SOURCES}")
target_compile_options(jxl_extras-static PRIVATE "${JPEGXL_INTERNAL_FLAGS}")
set_property(TARGET jxl_extras-static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(jxl_extras-static PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(jxl_extras-static PUBLIC
  jxl-static
  jxl_extras_dec-static
)

find_package(GIF 5.1)
if(GIF_FOUND)
  target_sources(jxl_extras_dec-obj PRIVATE
    extras/dec/gif.cc
    extras/dec/gif.h
  )
  target_include_directories(jxl_extras_dec-obj PRIVATE "${GIF_INCLUDE_DIRS}")
  list(APPEND JXL_EXTRAS_DEC_INTERNAL_LIBRARIES ${GIF_LIBRARIES})
  list(APPEND JXL_EXTRAS_DEC_PUBLIC_DEFINITIONS -DJPEGXL_ENABLE_GIF=1)
  if(JPEGXL_DEP_LICENSE_DIR)
    configure_file("${JPEGXL_DEP_LICENSE_DIR}/libgif-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.libgif COPYONLY)
  endif()  # JPEGXL_DEP_LICENSE_DIR
endif()

find_package(JPEG)
if(JPEG_FOUND)
  target_sources(jxl_extras_dec-obj PRIVATE
    extras/dec/jpg.cc
    extras/dec/jpg.h
  )
  target_include_directories(jxl_extras_dec-obj PRIVATE "${JPEG_INCLUDE_DIRS}")
  list(APPEND JXL_EXTRAS_DEC_INTERNAL_LIBRARIES ${JPEG_LIBRARIES})
  list(APPEND JXL_EXTRAS_DEC_PUBLIC_DEFINITIONS -DJPEGXL_ENABLE_JPEG=1)
  target_sources(jxl_extras-static PRIVATE
    extras/enc/jpg.cc
    extras/enc/jpg.h
  )
  target_include_directories(jxl_extras-static PRIVATE "${JPEG_INCLUDE_DIRS}")
  target_link_libraries(jxl_extras-static PRIVATE ${JPEG_LIBRARIES})
  target_compile_definitions(jxl_extras-static PUBLIC -DJPEGXL_ENABLE_JPEG=1)
  if(JPEGXL_DEP_LICENSE_DIR)
    configure_file("${JPEGXL_DEP_LICENSE_DIR}/libjpeg-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.libjpeg COPYONLY)
  endif()  # JPEGXL_DEP_LICENSE_DIR
endif()

if(NOT JPEGXL_BUNDLE_LIBPNG)
  find_package(PNG)
endif()
if(PNG_FOUND)
  target_sources(jxl_extras_dec-obj PRIVATE
    extras/dec/apng.cc
    extras/dec/apng.h
  )
  target_include_directories(jxl_extras_dec-obj PRIVATE "${PNG_INCLUDE_DIRS}")
  list(APPEND JXL_EXTRAS_DEC_INTERNAL_LIBRARIES ${PNG_LIBRARIES})
  list(APPEND JXL_EXTRAS_DEC_PUBLIC_DEFINITIONS -DJPEGXL_ENABLE_APNG=1)
  target_sources(jxl_extras-static PRIVATE
    extras/enc/apng.cc
    extras/enc/apng.h
  )
  target_include_directories(jxl_extras-static PUBLIC "${PNG_INCLUDE_DIRS}")
  target_link_libraries(jxl_extras-static PUBLIC ${PNG_LIBRARIES})
  target_compile_definitions(jxl_extras-static PUBLIC -DJPEGXL_ENABLE_APNG=1)
  configure_file(extras/LICENSE.apngdis
                 ${PROJECT_BINARY_DIR}/LICENSE.apngdis COPYONLY)
endif()

if (JPEGXL_ENABLE_SJPEG)
  target_compile_definitions(jxl_extras-static PUBLIC -DJPEGXL_ENABLE_SJPEG=1)
  target_link_libraries(jxl_extras-static PRIVATE sjpeg)
endif ()

if (JPEGXL_ENABLE_OPENEXR)
pkg_check_modules(OpenEXR IMPORTED_TARGET OpenEXR)
if (OpenEXR_FOUND)
  target_sources(jxl_extras_dec-obj PRIVATE
    extras/dec/exr.cc
    extras/dec/exr.h
  )
  list(APPEND JXL_EXTRAS_DEC_PUBLIC_DEFINITIONS -DJPEGXL_ENABLE_EXR=1)
  # For the include directory.
  target_link_libraries(jxl_extras_dec-obj PRIVATE PkgConfig::OpenEXR)
  list(APPEND JXL_EXTRAS_DEC_INTERNAL_LIBRARIES PkgConfig::OpenEXR)
  target_sources(jxl_extras-static PRIVATE
    extras/enc/exr.cc
    extras/enc/exr.h
  )
  target_compile_definitions(jxl_extras-static PUBLIC -DJPEGXL_ENABLE_EXR=1)
  target_link_libraries(jxl_extras-static PRIVATE PkgConfig::OpenEXR)
  if(JPEGXL_DEP_LICENSE_DIR)
    configure_file("${JPEGXL_DEP_LICENSE_DIR}/libopenexr-dev/copyright"
                   ${PROJECT_BINARY_DIR}/LICENSE.libopenexr COPYONLY)
  endif()  # JPEGXL_DEP_LICENSE_DIR
  # OpenEXR generates exceptions, so we need exception support to catch them.
  # Actually those flags counteract the ones set in JPEGXL_INTERNAL_FLAGS.
  if (NOT WIN32)
    set_source_files_properties(extras/dec/exr.cc extras/enc/exr.cc PROPERTIES COMPILE_FLAGS -fexceptions)
    if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
      set_source_files_properties(extras/dec/exr.cc extras/enc/exr.cc PROPERTIES COMPILE_FLAGS -fcxx-exceptions)
    endif()
  endif()
endif() # OpenEXR_FOUND
endif() # JPEGXL_ENABLE_OPENEXR

target_compile_definitions(jxl_extras_dec-obj PRIVATE ${JXL_EXTRAS_DEC_PUBLIC_DEFINITIONS})

add_library(jxl_extras_dec-static STATIC $<TARGET_OBJECTS:jxl_extras_dec-obj>)
target_compile_definitions(jxl_extras_dec-static PUBLIC ${JXL_EXTRAS_DEC_PUBLIC_DEFINITIONS})
target_link_libraries(jxl_extras_dec-static PRIVATE ${JXL_EXTRAS_DEC_INTERNAL_LIBRARIES})

add_library(jxl_extras_dec SHARED $<TARGET_OBJECTS:jxl_extras_dec-obj>)
target_compile_definitions(jxl_extras_dec PUBLIC ${JXL_EXTRAS_DEC_PUBLIC_DEFINITIONS})
target_link_libraries(jxl_extras_dec PRIVATE ${JXL_EXTRAS_DEC_INTERNAL_LIBRARIES} jxl)
