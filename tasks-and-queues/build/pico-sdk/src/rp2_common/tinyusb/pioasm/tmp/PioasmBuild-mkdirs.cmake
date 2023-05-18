# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/daniel.sierra/Desktop/freertos-pico/pico-sdk/tools/pioasm"
  "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pioasm"
  "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pico-sdk/src/rp2_common/tinyusb/pioasm"
  "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pico-sdk/src/rp2_common/tinyusb/pioasm/tmp"
  "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp"
  "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src"
  "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/daniel.sierra/Desktop/freertos-pico/tasks-and-queues/build/pico-sdk/src/rp2_common/tinyusb/pioasm/src/PioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()