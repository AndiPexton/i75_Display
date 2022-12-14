cmake_minimum_required(VERSION 3.12)

set(NAME i75_Display)

include(pimoroni_pico_import.cmake)
include(pico_sdk_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
        i75_Display.cpp # <-- Add source files here!
        )

# Include required libraries
# This assumes `pimoroni-pico` is stored alongside your project
include(common/pimoroni_i2c)
include(common/pimoroni_bus)
include(libraries/bitmap_fonts/bitmap_fonts)
include(libraries/hershey_fonts/hershey_fonts)
include(libraries/pico_explorer/pico_explorer)
include(drivers/hub75/hub75)

# Don't forget to link the libraries you need!
target_link_libraries(${NAME}
        pimoroni_i2c
        pico_explorer
        pico_stdlib
        pico_multicore
        hardware_vreg
        hub75 # <-- List libraries here!
        )

# create map/bin/hex file etc.
pico_enable_stdio_usb(${NAME} 1)
pico_add_extra_outputs(${NAME})

# Set up files for the release packages
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
        ${CMAKE_CURRENT_LIST_DIR}/README.md
        DESTINATION .
        )

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
