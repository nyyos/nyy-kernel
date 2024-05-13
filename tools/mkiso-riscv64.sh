#!/bin/sh

PORT=riscv64

LIMINE=vendor/limine
ISOROOT=build-${PORT}/isoroot
ISO=build-${PORT}/nyy.iso

mkdir -p "${ISOROOT}" &&
	cp build-${PORT}/platform/${PORT}-virt/nyy "${ISOROOT}" &&
	cp "${LIMINE}/limine-uefi-cd.bin" "${LIMINE}/BOOTRISCV64.EFI" "${ISOROOT}" &&
	cp tools/limine-${PORT}.cfg "${ISOROOT}/limine.cfg" &&
	xorriso -as mkisofs \
		--efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		"${ISOROOT}" -o "${ISO}"
