SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-i386"
TEXT_START_ADDR=0x08048000
TEXT_START_SYMBOLS='_btext = .;'
MAXPAGESIZE=0x1000
NONPAGED_TEXT_START_ADDR=0x08048000
#SHLIB_TEXT_START_ADDR=0xb0300000
ARCH=i386
MACHINE=
NOP=0x9090
TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
