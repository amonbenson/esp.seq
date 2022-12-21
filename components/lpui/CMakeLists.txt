idf_component_register(
    SRCS src/lpui_types.c src/lpui.c
        src/lpui_components/button.c
        src/lpui_components/pattern_editor.c
        src/lpui_components/piano_editor.c
    INCLUDE_DIRS include
    REQUIRES midi callback sequencer)
