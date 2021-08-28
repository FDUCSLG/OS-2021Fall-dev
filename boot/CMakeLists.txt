get_property(kernel_image GLOBAL PROPERTY kernel_image_path)

add_custom_command(
    OUTPUT sd.img
    BYPRODUCTS boot.img
    COMMAND ./generate-image.py ${kernel_image} ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${kernal_image} generate-image.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_custom_target(image ALL DEPENDS sd.img)
