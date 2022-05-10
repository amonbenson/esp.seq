idf_component_register(
    SRCS src/usb.c src/usb_midi.c src/midi_message.c src/midi.c
    INCLUDE_DIRS include
    REQUIRES usb)
