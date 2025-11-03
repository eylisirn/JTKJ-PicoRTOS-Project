# Remember to uncomment in the root CMakeLists.txt the corresponding add_subdirectory if you want to include this application in your project


set(DEFAULT_TARGET imu_sensori)
add_executable($imu_sensori
   src/main.c
)


target_link_libraries(imu_sensori PRIVATE
  pico_stdlib
  FreeRTOS-Kernel
  FreeRTOS-Kernel-Heap4
  TKJHAT_SDK
  usb_serial_debug
)

pico_enable_stdio_usb(imu_sensori 1)
pico_enable_stdio_uart(imu_sensori 0)

pico_add_extra_outputs(imu_sensori)