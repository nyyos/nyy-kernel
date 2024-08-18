#!/bin/sh

export LANG=c

PORT=amd64
ISOROOT=build-${PORT}/isoroot-hyper
ISO=build-${PORT}/nyy-${PORT}-hyper.iso
HYPER_RELEASE=v0.8.0
HYPER_RELEASE_URL="https://github.com/UltraOS/Hyper/releases/download/${HYPER_RELEASE}"
HYPER=build-${PORT}/hyper-${HYPER_RELEASE}

function dl_hyper() {
	if [[ ! -f ${HYPER}/$1 ]]; then
		echo "downloading $1 ..."
		curl -sL "${HYPER_RELEASE_URL}/$1" -o "${HYPER}/$1"
	fi
}

mkdir -p "${HYPER}"

dl_hyper hyper_iso_boot
dl_hyper BOOTX64.EFI
dl_hyper hyper_install-linux-x86_64
chmod +x "${HYPER}/hyper_install-linux-x86_64"

if [[ ! -f ${HYPER}/esp.img ]]; then
	dd if=/dev/zero of="${HYPER}/esp.img" iflag=fullblock bs=512K count=1
	mkfs.fat "${HYPER}/esp.img"
	device="$(udisksctl loop-setup -f "${HYPER}/esp.img" | awk '{ print $5 }'|tr -d '.')"
	mp="$(udisksctl mount -b "$device" | awk '{print $4}')"

	mkdir -p "${mp}/EFI/BOOT/"
	cp "${HYPER}/BOOTX64.EFI" "${mp}/EFI/BOOT/"

	udisksctl unmount -b "$device"
	udisksctl loop-delete -b "${device}"
fi

mkdir -p "${ISOROOT}" &&
	cp build-${PORT}/kernel/platform/amd64/nyy ${HYPER}/esp.img ${HYPER}/hyper_iso_boot "${ISOROOT}" &&
	cp tools/hyper-amd64.cfg "${ISOROOT}/hyper.cfg" &&
	xorriso -as mkisofs -b hyper_iso_boot \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot esp.img -efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		"${ISOROOT}" -o "${ISO}" &&
	"${HYPER}/hyper_install-linux-x86_64" "${ISO}"
