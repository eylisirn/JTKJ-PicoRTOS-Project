# Remember to uncomment in the root CMakeLists.txt the corresponding add_subdirectory if you want to include this application in your project


set(DEFAULT_TARGET hat_app)
add_executable(${DEFAULT_TARGET}
  ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

add_subdirectory(libs/TKJHAT)
add_subdirectory(libs/usb-serial-debug)
add_subdirectory(examples/hat_app)

target_link_libraries(${DEFAULT_TARGET} PRIVATE
  pico_stdlib
  FreeRTOS-Kernel
  FreeRTOS-Kernel-Heap4
  TKJHAT_SDK
)

pico_enable_stdio_usb(${DEFAULT_TARGET} 1)
pico_enable_stdio_uart(${DEFAULT_TARGET} 0)

pico_add_extra_outputs(${DEFAULT_TARGET})