qemu-system-riscv64 -M virt -cpu rv64 -device ramfb -device qemu-xhci -device usb-kbd -m 2G -drive if=pflash,unit=0,format=raw,file=ovmf-riscv64/OVMF.fd -device virtio-scsi-pci,id=scsi -device scsi-cd,drive=cd0 -drive id=cd0,format=raw,file=./build-riscv64/nyy.iso
^Cqemu: terminating on signal 2
