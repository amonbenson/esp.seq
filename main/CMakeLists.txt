idf_component_register(
    SRCS src/main.c
    INCLUDE_DIRS include
    REQUIRES store output sequencer)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
