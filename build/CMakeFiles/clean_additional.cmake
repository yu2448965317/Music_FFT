# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "bootloader\\bootloader.bin"
  "bootloader\\bootloader.elf"
  "bootloader\\bootloader.map"
  "config\\sdkconfig.cmake"
  "config\\sdkconfig.h"
  "esp-idf\\esptool_py\\flasher_args.json.in"
  "esp-idf\\mbedtls\\x509_crt_bundle"
  "favicon.ico.S"
  "flash_app_args"
  "flash_bootloader_args"
  "flash_project_args"
  "flasher_args.json"
  "i80_controller.bin"
  "i80_controller.map"
  "ldgen_libraries"
  "ldgen_libraries.in"
  "project_elf_src_esp32s3.c"
  "upload_script.html.S"
  "x509_crt_bundle.S"
  )
endif()
