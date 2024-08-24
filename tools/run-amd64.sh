#!/bin/bash

PORT=amd64

qemu_args=
iso="build-${PORT}/nyy-amd64-limine.iso"

while getopts "kngspHuq:10" optchar; do
	case $optchar in
	s) serial_stdio=1 ;;
	k) qemu_args="$qemu_args -enable-kvm -cpu host" ;;
	n) qemu_args="$qemu_args -drive file=test.img,if=none,id=nvm -device nvme,serial=deadbeef,drive=nvm" ;;
	p) pause=1 ;;
	q) QEMU_EXE=$OPTARG;;
	g) qemu_args="$qemu_args -M smm=off -d int -D qemulog.txt" ;;
	u) iso="build-${PORT}/nyy-${PORT}-hyper.iso" ;;
	H) qemu_args="$qemu_args -machine hpet=off" ;;
	*) exit 1 ;;
	esac
done

QEMU_EXE="${QEMU_EXE:=qemu-system-x86_64}"
qemu_args="$qemu_args -s"
if [ "$serial_stdio" = "1" ]; then
	qemu_args="${qemu_args} -serial stdio"
fi

if [ "$pause" = "1" ]; then
	qemu_args="${qemu_args} -S"
fi

${QEMU_EXE} -m 256M -smp 4 -M q35 \
	-cdrom "${iso}" \
	-no-shutdown -no-reboot \
	${qemu_args}
