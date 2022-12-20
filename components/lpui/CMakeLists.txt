idf_component_register(
    SRCS src/lpui_types.c src/lpui.c
        src/components/pattern_editor.c
    INCLUDE_DIRS include
    REQUIRES callback sequencer)
