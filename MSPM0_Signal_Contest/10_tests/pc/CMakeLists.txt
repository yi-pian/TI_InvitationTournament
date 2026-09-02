cmake_minimum_required(VERSION 3.16)
project(signal_library_pc_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

get_filename_component(SIGNAL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
file(GLOB_RECURSE SIGNAL_SOURCES CONFIGURE_DEPENDS
    "${SIGNAL_ROOT}/01_bsp/*.c"
    "${SIGNAL_ROOT}/02_acquisition/*.c"
    "${SIGNAL_ROOT}/03_measurement/*.c"
    "${SIGNAL_ROOT}/04_dsp/*.c"
    "${SIGNAL_ROOT}/05_precision/*.c"
    "${SIGNAL_ROOT}/06_generator/*.c"
    "${SIGNAL_ROOT}/07_signal_frontend/*.c")
list(FILTER SIGNAL_SOURCES EXCLUDE REGEX "/02_acquisition/adc_dma/")
list(FILTER SIGNAL_SOURCES EXCLUDE REGEX "/02_acquisition/adc_fifo_dma/")
list(FILTER SIGNAL_SOURCES EXCLUDE REGEX "/README_MINIMAL_EXAMPLE\\.c$")
list(FILTER SIGNAL_SOURCES EXCLUDE REGEX "/signal_(tft_ili9341|dual_adc|timer_capture|dac_dma)_mspm0g3507\\.c$")
list(FILTER SIGNAL_SOURCES EXCLUDE REGEX "/main(_template)?\\.c$")
list(APPEND SIGNAL_SOURCES
    "${SIGNAL_ROOT}/08_applications/common/signal_integration.c")

file(GLOB_RECURSE SIGNAL_HEADERS CONFIGURE_DEPENDS
    "${SIGNAL_ROOT}/01_bsp/*.h"
    "${SIGNAL_ROOT}/02_acquisition/*.h"
    "${SIGNAL_ROOT}/03_measurement/*.h"
    "${SIGNAL_ROOT}/04_dsp/*.h"
    "${SIGNAL_ROOT}/05_precision/*.h"
    "${SIGNAL_ROOT}/06_generator/*.h"
    "${SIGNAL_ROOT}/07_signal_frontend/*.h"
    "${SIGNAL_ROOT}/08_applications/*.h")
set(SIGNAL_INCLUDE_DIRS "")
foreach(header IN LISTS SIGNAL_HEADERS)
    get_filename_component(header_dir "${header}" DIRECTORY)
    list(APPEND SIGNAL_INCLUDE_DIRS "${header_dir}")
endforeach()
list(REMOVE_DUPLICATES SIGNAL_INCLUDE_DIRS)

add_executable(signal_library_tests test_signal_library.c ${SIGNAL_SOURCES})
target_include_directories(signal_library_tests PRIVATE ${SIGNAL_INCLUDE_DIRS})
if(MSVC)
    target_compile_options(signal_library_tests PRIVATE /W4 /WX)
else()
    target_compile_options(signal_library_tests PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(signal_library_tests PRIVATE m)
endif()

enable_testing()
add_test(NAME signal_library_tests COMMAND signal_library_tests)
