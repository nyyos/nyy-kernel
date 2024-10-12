#!/bin/sh

PORT=amd64

LIMINE=vendor/limine
BUILDDIR=${BUILDDIR:-build-${PORT}}
ISOROOT=${BUILDDIR}/isoroot
ISO=${BUILDDIR}/nyy-${PORT}-limine.iso

mkdir -p "${ISOROOT}" &&
	cp ${BUILDDIR}/kernel/platform/amd64/symbols.map ${BUILDDIR}/kernel/platform/amd64/nyy ${LIMINE}/limine-bios.sys ${LIMINE}/limine-bios-cd.bin ${LIMINE}/limine-uefi-cd.bin "${ISOROOT}" &&
	cp tools/limine-amd64.conf "${ISOROOT}/limine.conf" &&
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin -efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		"${ISOROOT}" -o "${ISO}" &&
	./${BUILDDIR}/limine bios-install "${ISO}"
