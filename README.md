# rtos-pico-rp2040

Proyecto de CC7

Integrantes:

- 17001969 Pablo Agreda
- 17001623 Victor Recinos
- 18002864 Daniel Sierra
- 19003827 Santiago Maldona

Instrucciones instalacion y configuracion para MAC OS

- brew install cmake
- brew tap ArmMbed/homebrew-formulae
- brew install arm-none-eabi-gcc

En el directorio principal ejecutar antes de compilar cualquier codigo

- export PICO_SDK_PATH=$PWD/pico-sdk

El proyecto tiene su carpeta build, eliminar todo si hay archivos luego ejecutar

- cmake ..
- make
