idf_component_register(
    SRCS src/main.c
    INCLUDE_DIRS include
    REQUIRES midi store output sequencer controller)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
