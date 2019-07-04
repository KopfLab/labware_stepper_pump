SRCS:=$(shell find ./src -name *.cpp -or -name *.h -or -name *.ino)
PLATFORM?="photon"
VERSION?="1.0.1"
BIN:=$(PLATFORM)_$(VERSION)_project.bin
device?=

list:
	@echo "\nINFO: querying list of available devices..."
	@particle list

monitor:
	@echo "\nINFO: connecting to serial monitor..."
	@trap "exit" INT; while :; do particle serial monitor; done

%.bin: project.properties $(SRCS)
	@echo "INFO: compiling project in the cloud for $(PLATFORM) $(VERSION)...."
	@particle compile $(PLATFORM) --target $(VERSION) --saveTo $@

compile: 
	@$(MAKE) $(BIN)
	
flash: 
	@$(MAKE) $(BIN)
ifeq ($(device),)
	@$(MAKE) usb_flash BIN=$(BIN)
else
	@$(MAKE) cloud_flash BIN=$(BIN)
endif

cloud_flash:
ifeq ($(device),)
	@echo "ERROR: no device provided, specify the name to flash via make ... device=???."
else
	@echo "\nINFO: flashing $(BIN) to $(device) via the cloud..."
	@particle flash $(device) $(BIN)
endif

usb_flash:
	@echo "INFO: flashing $(BIN) over USB (requires device in DFU mode = yellow)..."
	@particle flash --usb  $(BIN)

clean:
	@echo "INFO: removing all .bin files..."
	@rm -f ./*.bin
