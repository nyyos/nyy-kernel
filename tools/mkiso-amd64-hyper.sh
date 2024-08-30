#!/bin/sh

export LANG=c

source "$(dirname "$0")/common.sh"

PORT=amd64
ISOROOT=build-${PORT}/isoroot-hyper
ISO=build-${PORT}/nyy-${PORT}-hyper.iso
HYPER_RELEASE=v0.9.0
HYPER_URL="https://github.com/UltraOS/Hyper/releases/download/${HYPER_RELEASE}"
HYPER="build-${PORT}/hyper-${HYPER_RELEASE}"

mkdir -p "${HYPER}"

fetch "${HYPER}/hyper_iso_boot" "${HYPER_URL}/hyper_iso_boot"
fetch "${HYPER}/BOOTX64.EFI" "${HYPER_URL}/BOOTX64.EFI"
if [ "$(uname)" = "Linux" ]; then
	fetch "${HYPER}/hyper_install" "${HYPER_URL}/hyper_install-linux-x86_64"
	chmod +x "${HYPER}/hyper_install"
else
	echo "There is no native hyper_install available for your platform." 1>&2
	exit 1
fi

if [[ ! -f ${HYPER}/esp.img ]]; then
	dd if=/dev/zero of="${HYPER}/esp.img" iflag=fullblock bs=1024K count=1
	mformat -i "${HYPER}/esp.img"
	mmd -i "${HYPER}/esp.img" ::/EFI
	mmd -i "${HYPER}/esp.img" ::/EFI/BOOT
	mcopy -i "${HYPER}/esp.img" "${HYPER}/BOOTX64.EFI" ::/EFI/BOOT/
fi

mkdir -p "${ISOROOT}" &&
	cp build-${PORT}/kernel/platform/amd64/nyy \
		build-${PORT}/kernel/platform/amd64/symbols.map \
		${HYPER}/esp.img \
		${HYPER}/hyper_iso_boot \
		"${ISOROOT}" &&
	cp tools/hyper-amd64.cfg "${ISOROOT}/hyper.cfg" &&
	xorriso -as mkisofs -b hyper_iso_boot \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot esp.img -efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		"${ISOROOT}" -o "${ISO}" &&
	"${HYPER}/hyper_install" "${ISO}"
