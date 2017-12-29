PROGRAM = led

HOMEKIT_SPI_FLASH_BASE_ADDR=0x70000

EXTRA_COMPONENTS = \
	extras/http-parser \
	$(abspath ../components/esp-wolfssl) \
	$(abspath ../components/esp-cjson) \
	$(abspath ../components/esp-homekit)

FLASH_SIZE = 8

EXTRA_CFLAGS += -I.. -DHOMEKIT_SHORT_APPLE_UUIDS

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)

reset:
	$(ESPTOOL) -p $(ESPPORT) --baud $(ESPBAUD) erase_region $(HOMEKIT_SPI_FLASH_BASE_ADDR) 0x1000
