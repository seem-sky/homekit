PROGRAM = esp-blindkit

HOMEKIT_SPI_FLASH_BASE_ADDR=0x70000

EXTRA_CFLAGS=-DLWIP_HTTPD_CGI=1 -DLWIP_HTTPD_SSI=1 -I./fsdata

EXTRA_COMPONENTS=extras/mbedtls extras/httpd

FLASH_SIZE = 8

EXTRA_CFLAGS += -I.. -DHOMEKIT_SHORT_APPLE_UUIDS

include $(SDK_PATH)/common.mk

ESPPORT ?= /dev/cu.usbserial
ESPBAUD ?= 115200


monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)

reset:
	$(ESPTOOL) -p $(ESPPORT) --baud $(ESPBAUD) erase_region $(HOMEKIT_SPI_FLASH_BASE_ADDR) 0x1000

html:
	@echo "Generating fsdata.."
	cd fsdata && ./makefsdata
