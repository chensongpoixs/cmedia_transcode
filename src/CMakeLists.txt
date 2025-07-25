# MIT License
#
# Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

file(GLOB MediaKit_SRC_LIST
  ${CMAKE_CURRENT_SOURCE_DIR}/*/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*/*.h)

if(NOT ENABLE_SRT)
    file(GLOB SRT_SRC_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/Srt/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/Srt/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Srt/*.h)
    list(REMOVE_ITEM MediaKit_SRC_LIST ${SRT_SRC_LIST})
endif()

if(USE_SOLUTION_FOLDERS AND (NOT GROUP_BY_EXPLORER))
  # 在 IDE 中对文件进行分组, 源文件和头文件分开
  set_file_group("${CMAKE_CURRENT_SOURCE_DIR}" ${MediaKit_SRC_LIST})
endif()


# 添加库
add_library(zlmediakit STATIC ${MediaKit_SRC_LIST})
add_library(ZLMediaKit::MediaKit ALIAS zlmediakit)

set(LINK_LIBRARIES ${MK_LINK_LIBRARIES})
list(REMOVE_ITEM LINK_LIBRARIES ZLMediaKit::MediaKit)
set(COMPILE_DEFINITIONS ${MK_COMPILE_DEFINITIONS})

target_compile_definitions(zlmediakit
  PUBLIC ${COMPILE_DEFINITIONS})
target_compile_options(zlmediakit
  PRIVATE ${COMPILE_OPTIONS_DEFAULT})
target_link_libraries(zlmediakit
  PUBLIC ${LINK_LIBRARIES}
    "D:/deps/libcuda/lib/x64/nvcuvid.lib"
"D:/deps/libcuda/lib/x64/cuda.lib"
"D:/deps/opencv/x64/vc16/lib/opencv_world4120d.lib"
"D:/deps/opencv/x64/vc16/lib/opencv_img_hash4120d.lib"
"D:/deps/libcuda/lib/x64/cudart.lib"
"D:/deps/libcuda/lib/x64/nvencodeapi.lib"
  )
target_include_directories(zlmediakit
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>"
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

update_cached_list(MK_LINK_LIBRARIES ZLMediaKit::MediaKit)

# 未在使用
if(ENABLE_CXX_API)
  # 保留目录结构
  if(ENABLE_SRT)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
      DESTINATION ${INSTALL_PATH_INCLUDE}/ZLMediaKit
      FILES_MATCHING
        PATTERN "*.h")
  else()
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
      DESTINATION ${INSTALL_PATH_INCLUDE}/ZLMediaKit
      FILES_MATCHING 
        REGEX ".*/Srt.*" EXCLUDE
        PATTERN "*.h")
  endif()
  install(TARGETS zlmediakit
    DESTINATION ${INSTALL_PATH_LIB})
endif ()
