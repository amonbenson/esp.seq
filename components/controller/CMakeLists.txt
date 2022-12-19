idf_component_register(
    SRCS src/controller.c
        src/controllers/launchpad/lpui.c
        src/controllers/launchpad/launchpad.c
        src/controllers/generic.c
    INCLUDE_DIRS include
    REQUIRES callback midi sequencer output)
