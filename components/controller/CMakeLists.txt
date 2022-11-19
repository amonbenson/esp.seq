idf_component_register(
    SRCS src/controller.c src/controllers/launchpad.c src/controllers/generic.c
    INCLUDE_DIRS include
    REQUIRES midi sequencer output)
