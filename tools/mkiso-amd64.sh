#!/bin/sh

PORT=amd64

LIMINE=vendor/limine
ISOROOT=build-${PORT}/isoroot
ISO=build-${PORT}/nyy.iso

mkdir -p "${ISOROOT}" &&
	cp build-${PORT}/kernel/platform/amd64/symbols.map build-${PORT}/kernel/platform/amd64/nyy ${LIMINE}/limine-bios.sys ${LIMINE}/limine-bios-cd.bin ${LIMINE}/limine-uefi-cd.bin "${ISOROOT}" &&
	cp tools/limine-amd64.cfg "${ISOROOT}/limine.cfg" &&
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		"${ISOROOT}" -o "${ISO}" &&
	./build-${PORT}/limine bios-install "${ISO}"
