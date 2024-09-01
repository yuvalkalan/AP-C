cmake_minimum_required(VERSION 3.13)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Set name of project (as PROJECT_NAME)
project(main VERSION 1.0.0)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(${PROJECT_NAME}
                src/main.c
                src/dhcpserver/dhcpserver.c
                src/dnsserver/dnsserver.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)


target_link_libraries(${PROJECT_NAME}
                        pico_stdlib          # Standard Pico SDK libraries
                        hardware_gpio        # GPIO hardware support
                        pico_cyw43_arch_lwip_threadsafe_background  # CYW43 Wi-Fi driver with lwIP
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})
