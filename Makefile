ARCH ?= amd64

all:
	@meson compile -C build

build:
	@meson setup --cross-file crossfiles/$(ARCH).ini -Darch=$(ARCH) build

run:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -sk

gdb:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -sgp
