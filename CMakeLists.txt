cmake_minimum_required(VERSION 3.16)

project(rpi-os VERSION 0.1.0 LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

# if we are already on aarch64 platform, there's no need to cross-compile.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(aarch64_prefix "")
    set(aarch64_gdb "gdb")
else()
    set(aarch64_prefix "aarch64-linux-gnu-")
    set(aarch64_gdb "gdb-multiarch")
endif()

set(aarch64_gcc "${aarch64_prefix}gcc")
set(aarch64_ld "${aarch64_prefix}ld")
set(aarch64_objdump "${aarch64_prefix}objdump")
set(aarch64_objcopy "${aarch64_prefix}objcopy")

set(aarch64_qemu "qemu-system-aarch64")

add_subdirectory(src)
add_subdirectory(boot)

get_property(kernel_elf GLOBAL PROPERTY kernel_elf_path)
get_property(kernel_image GLOBAL PROPERTY kernel_image_path)
get_property(sd_image GLOBAL PROPERTY sd_image_path)

execute_process(
    COMMAND sh -c "qemu-system-aarch64 --version | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+'"
    OUTPUT_VARIABLE qemu_version
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if (qemu_version VERSION_LESS "6.2.0")
    set(qemu_machine "raspi3")
else()
    set(qemu_machine "raspi3b")
endif()
message("QEMU version: " ${qemu_version} ". Use " ${qemu_machine} ".")

set(qemu_flags
    -machine ${qemu_machine}
    -nographic
    -serial "null"
    -serial "mon:stdio"
    -drive "file=${sd_image},if=sd,format=raw"
    -kernel "${kernel_image}")

add_custom_target(qemu
    COMMAND ${aarch64_qemu} ${qemu_flags} -gdb tcp::1234
    DEPENDS sd-image)
add_custom_target(qemu-debug
    COMMAND ${aarch64_qemu} ${qemu_flags} -gdb tcp::1234 -S
    DEPENDS sd-image)
add_custom_target(debug
    COMMAND ${aarch64_gdb} --nx --quiet
            -ex "set architecture aarch64"
            -ex "file ${kernel_elf}"
            -ex "target remote localhost:1234"
    DEPENDS kernel)

set(musl_config ${CMAKE_CURRENT_SOURCE_DIR}/libc/config.mak)
add_custom_command(
    COMMENT "Generating musl configuration..."
    OUTPUT ${musl_config}
    COMMAND git submodule update --init &&
            cd ../libc &&
            CROSS_COMPILE=${aarch64_prefix} ./configure --target=aarch64 --enable-optimize --disable-debug)
add_custom_target(init-libc ALL DEPENDS ${musl_config})

set(musl_specs ${CMAKE_CURRENT_SOURCE_DIR}/libc/lib/musl-gcc.specs)
set(libc_specs musl-gcc.specs)
add_custom_command(
    COMMENT "Compiling musl..."
    OUTPUT ${libc_specs}
    COMMAND env "MAKEFLAGS=$(subst s,,$(MAKEFLAGS))" make -C ../libc &&
            sed -e 's/\\/usr\\/local\\/musl/..\\/..\\/..\\/libc/g' ${musl_specs} > ${libc_specs}
    DEPENDS init-libc)

add_custom_target(libc ALL DEPENDS ${libc_specs})

add_custom_target(clean-libc
    COMMAND cd ../libc &&
            make clean
    DEPENDS init-libc)
