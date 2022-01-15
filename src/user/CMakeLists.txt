file(GLOB user_sources CONFIGURE_DEPENDS "*.S")

add_library(user STATIC ${user_sources})

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_COMPILER ${aarch64_gcc})
set(CMAKE_ASM_COMPILER ${aarch64_gcc})

include_directories(../../libc/obj/include)
include_directories(../../libc/arch/aarch64)
include_directories(../../libc/arch/generic)

set(libc_specs ${CMAKE_CURRENT_BINARY_DIR}/../../musl-gcc.specs)
set(CMAKE_C_FLAGS "-specs ${libc_specs} \
	-std=gnu99 \
	-MMD -MP \
	-static \
	-fno-plt -fno-pic -fpie \
	-z max-page-size=4096 \
	-s")
set(CMAKE_EXE_LINKER_FLAGS "")

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/mkfs
	COMMAND cc ${CMAKE_CURRENT_SOURCE_DIR}/mkfs/main.c
				-o ${CMAKE_CURRENT_BINARY_DIR}/mkfs
				-I ${CMAKE_CURRENT_SOURCE_DIR}/..
	DEPENDS "mkfs/main.c")
add_custom_target(mkfs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/mkfs)

# add targets here if needed.
set(user_binary_list
	cat
	echo
	init
	ls
	sh
	mkdir)

foreach(binary ${user_binary_list})
	add_executable(${binary} ${CMAKE_CURRENT_SOURCE_DIR}/${binary}/main.c)
	add_dependencies(${binary} libc)
endforeach(binary)

add_custom_target(user_binaries
	DEPENDS ${user_binary_list})

add_custom_command(
    OUTPUT fs.img
    COMMAND ./mkfs fs.img ${user_binary_list}
    DEPENDS mkfs user_binaries)

add_custom_target(fs-image ALL DEPENDS fs.img)

set_property(GLOBAL PROPERTY fs_image_path ${CMAKE_CURRENT_BINARY_DIR}/fs.img)
