LIBUSB_CFLAGS ?= $(shell pkg-config --cflags libusb-1.0)
LIBUSB_LDFLAGS ?= $(shell pkg-config --libs libusb-1.0)

CPPFLAGS ?= -Wall -std=c++11 $(LIBUSB_CFLAGS)
LDFLAGS ?= $(LIBUSB_LDFLAGS)

hsplus_load: hsplus_load.cpp

.PHONY: clean
clean:
	$(RM) hsplus_load
