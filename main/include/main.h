#pragma once


#define USB_MIDI_ENABLED
#define VIRTUAL_PERIPHERALS_ENABLED
#define FORCE_CONTROLLER controller_class_launchpad

// forced controller cannot be used with usb midi
#ifdef FORCE_CONTROLLER
    #undef USB_MIDI_ENABLED
#endif
