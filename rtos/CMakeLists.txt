cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
# include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
if (NOT PICO_SDK_PATH)
        message(FATAL_ERROR "?PICO_SDK_PATH? (not set)")
endif()

# Pull in FreeRTOS
# include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(app C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_executable(rtos main.c)

target_include_directories(rtos PRIVATE ${CMAKE_CURRENT_LIST_DIR})
 
# pull in common dependencies
target_link_libraries(rtos pico_stdlib)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(rtos)

# enable usb output, disable uart output

pico_enable_stdio_usb(rtos 1)

pico_enable_stdio_uart(rtos 0)

