cmake_minimum_required(VERSION 3.16)

project(rpi-os VERSION 0.1.0 LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(aarch64_prefix "aarch64-linux-gnu")
set(aarch64_gcc "${aarch64_prefix}-gcc")
set(aarch64_ld "${aarch64_prefix}-ld")
set(aarch64_objdump "${aarch64_prefix}-objdump")
set(aarch64_objcopy "${aarch64_prefix}-objcopy")

set(aarch64_qemu "qemu-system-aarch64")

add_subdirectory(src)

# add_custom_target(qemu
#     COMMAND ${aarch64_qemu} -M raspi3 -nographic -serial null -serial mon:stdio \
#                             -drive file=${sd_img},if=sd,format=raw)
