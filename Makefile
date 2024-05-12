ARCH ?= amd64

all:
	@meson compile -C build-$(ARCH)

.PHONY: build
build:
	@meson setup --cross-file crossfiles/$(ARCH).ini -Darch=$(ARCH) build-$(ARCH)

.PHONY: build-linux
build-linux:
	@meson setup -Darch=linux build-linux

run:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -sk

gdb:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -sgp

.PHONY: menuconfig
menuconfig:
	ROOT=$(shell pwd) python3 vendor/kconfiglib/menuconfig.py
