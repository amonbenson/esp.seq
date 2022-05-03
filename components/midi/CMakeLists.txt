file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
set(INCLUDES "include")

idf_component_register(SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDES}
    REQUIRES usb)
