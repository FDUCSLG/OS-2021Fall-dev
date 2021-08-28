file(GLOB arm_sources CONFIGURE_DEPENDS "*.c")

add_library(arm STATIC ${arm_sources})
