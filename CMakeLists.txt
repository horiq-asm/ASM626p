cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico_w)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(asm626)
file(GLOB MY_SOURCE *.h *.c)
# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(asm626
        ${MY_SOURCE} 
)
# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(asm626 pico_stdlib pico_stdlib hardware_rtc)
# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(asm626)
