idf_component_register(
    SRCS src/dac.c src/channel.c src/main.c
    INCLUDE_DIRS include
    REQUIRES midi launchpad)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
