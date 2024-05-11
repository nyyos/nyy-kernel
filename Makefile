ARCH ?= amd64

all:
	@meson compile -C build-$(ARCH)

build:
	@meson setup --cross-file crossfiles/$(ARCH).ini -Darch=$(ARCH) build-$(ARCH)

run:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -sk

gdb:
	./tools/mkiso-$(ARCH).sh
	./tools/run-$(ARCH).sh -sgp
