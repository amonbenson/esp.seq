#C:\Espressif/Initialize-Idf.ps1
C:\Espressif\frameworks\esp-idf-v4.4.1\components\esptool_py\esptool\esptool.py --chip esp32s2 -p COM6 -b 460800 --no-stub write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB 0x10000 build/espmidi.bin
