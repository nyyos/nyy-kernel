#!/bin/sh

LIMINE=vendor/limine
ISOROOT=build/isoroot
ISO=build/nyy.iso

mkdir -p ${ISOROOT} &&
	cp build/platform/amd64/nyy ${LIMINE}/limine-bios.sys ${LIMINE}/limine-bios-cd.bin ${LIMINE}/limine-uefi-cd.bin ${ISOROOT} &&
	cp tools/limine-amd64.cfg ${ISOROOT}/limine.cfg &&
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		${ISOROOT} -o ${ISO} &&
	./build/limine bios-install ${ISO}
