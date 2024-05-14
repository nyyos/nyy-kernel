#!/bin/bash

if [ ! -d "build-riscv64/ovmf-riscv64" ]; then
	echo "download ovmf"
	mkdir -p build-riscv64/ovmf-riscv64
	pushd build-riscv64/ovmf-riscv64 && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASERISCV64_VIRT_CODE.fd && dd if=/dev/zero of=OVMF.fd bs=1 count=0 seek=33554432
	popd || exit
fi

qemu-system-riscv64 -M virt -cpu rv64 -serial stdio -device ramfb -device qemu-xhci -device usb-kbd -m 128M -drive if=pflash,unit=0,format=raw,file=build-riscv64/ovmf-riscv64/OVMF.fd -device virtio-scsi-pci,id=scsi -device scsi-cd,drive=cd0 -drive id=cd0,format=raw,file=./build-riscv64/nyy.iso
