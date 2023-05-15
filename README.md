# rtos-pico-rp2040


Proyecto de CC7

Integrantes:

- 18002864 Daniel Sierra


Usando la Raspberry Pi RP2040 y la libreria FREERTOS, https://www.freertos.org/  Es un sistema operativo en tiempo real (RTOS) líder en el mercado para microcontroladores y pequeños microprocesadores. FreeRTOS, que se distribuye libremente bajo la licencia de código abierto del MIT, incluye un kernel y un conjunto creciente de bibliotecas de IoT adecuadas para su uso en todos los sectores de la industria. FreeRTOS está diseñado con énfasis en la confiabilidad y la facilidad de uso.


El proyecto se compone de varios pequenos proyectos que ejemplifican cada uno de los procesos que se llevaron a cabo
en la fase 1 de PINTOS, esta fase se compuso de manejar las interrupciones, manejar tareas, manejar prioridades, manejo
de colas, "paralelismo", implementaciond de algoritmos como round-robin

Instrucciones instalacion y configuracion para WINDOWS OS

- Instalar Git (includes Bash terminal): https://git-scm.com/downloads
- Instalar CMake: https://cmake.org/download/
- Instalar Arm GNU Toolchain: https://developer.arm.com/Tools%20and%20Software/GNU%20Toolchain
- Instalar Make usando Chocolatey
- Chocolatey deberia de estar ya instalado porque el paquete se baja cuando se onstala git, de lo contrario: https://chocolatey.org/
- Desde Git Bash: $ choco install make

En el directorio principal (freertos-pico) ejecutar antes de compilar cualquier codigo

- export PICO_SDK_PATH=$PWD/pico-sdk
- export FREERTOS_KERNEL_PATH=$PWD/FreeRTOS-Kernel

Cada proyecto tiene su carpeta build, eliminar todo si hay archivos luego ejecutar
- cmake -G "MinGW Makefiles" ..
- make

Instrucciones instalacion y configuracion para MAC OS

 - brew install cmake
 - brew tap ArmMbed/homebrew-formulae
 - brew install arm-none-eabi-gcc

En el directorio principal (freertos-pico) ejecutar antes de compilar cualquier codigo

- export PICO_SDK_PATH=$PWD/pico-sdk
- export FREERTOS_KERNEL_PATH=$PWD/FreeRTOS-Kernel

Cada proyecto tiene su carpeta build, eliminar todo si hay archivos luego ejecutar

- cmake ..
- make
