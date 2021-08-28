set(CMAKE_C_COMPILER ${aarch64_gcc})
set(CMAKE_ASM_COMPILER ${aarch64_gcc})

include_directories(.)

set(WARNINGS "-Wall -Wextra")

set(CMAKE_C_FLAGS " \
    ${CMAKE_C_FLAGS} ${WARNINGS} \
    -fno-pie -fno-pic -fno-stack-protector \
    -fno-zero-initialized-in-bss \
    -static -fno-builtin -nostdlib -nostdinc -ffreestanding -nostartfiles \
    -mgeneral-regs-only \
    -MMD -MP \
    -mlittle-endian -mcmodel=small -mno-outline-atomics \
    -mcpu=cortex-a53 -mtune=cortex-a53")

set(linker_script "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")
set(LINK_DEPENDS "${LINK_DEPENDS} ${linker_script}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${linker_script}")

add_subdirectory(aarch64)

set(kernel_name kernel8)
add_executable(${kernel_name}.elf main.c start.S)
target_link_libraries(${kernel_name}.elf aarch64)

set(kernel_prefix "${CMAKE_CURRENT_BINARY_DIR}/${kernel_name}")
set(kernel_elf "${kernel_prefix}.elf")
set(kernel_asm "${kernel_prefix}.asm")
set(kernel_hdr "${kernel_prefix}.hdr")
set(kernel_img "${kernel_prefix}.img")

set_property(GLOBAL PROPERTY kernel_image_path ${kernel_img})

add_custom_command(
    OUTPUT ${kernel_asm}
    COMMAND ${aarch64_objdump} -S -d ${kernel_elf} > ${kernel_asm}
    DEPENDS ${kernel_elf})

add_custom_command(
    OUTPUT ${kernel_hdr}
    COMMAND ${aarch64_objdump} -x ${kernel_elf} > ${kernel_hdr}
    DEPENDS ${kernel_elf})

add_custom_command(
    OUTPUT ${kernel_img}
    COMMAND ${aarch64_objcopy} -O binary ${kernel_elf} ${kernel_img}
    DEPENDS ${kernel_elf})

add_custom_target(kernel ALL DEPENDS ${kernel_asm} ${kernel_hdr} ${kernel_img})
