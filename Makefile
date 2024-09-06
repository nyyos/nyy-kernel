export ARCH ?= amd64
export LC_ALL=C
export LANG=C

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

run-hyper:
	./tools/mkiso-$(ARCH)-hyper.sh
	./tools/run-$(ARCH).sh -sku

gdb:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -sgp 

gdb-nogui:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -gGp

.PHONY: menuconfig
menuconfig:
	ROOT=$(shell pwd) python3 vendor/kconfiglib/menuconfig.py
