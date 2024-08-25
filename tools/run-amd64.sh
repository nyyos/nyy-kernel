#!/bin/bash

PORT=amd64

qemu_command="${QEMU_EXE:=qemu-system-x86_64}"
qemu_mem="${QEMU_MEM:=256M}"
qemu_smp="${QEMU_SMP:=2}"
qemu_args=
iso="build-${PORT}/nyy-amd64-limine.iso"

while getopts "kngspHuq:10" optchar; do
	case $optchar in
	s) qemu_args="$qemu_args -serial stdio" ;;
	k) qemu_args="$qemu_args -enable-kvm -cpu host" ;;
	n) qemu_args="$qemu_args -drive file=test.img,if=none,id=nvm -device nvme,serial=deadbeef,drive=nvm" ;;
	p) qemu_args="${qemu_args} -S" ;;
	q) QEMU_EXE=$OPTARG;;
	g) qemu_args="$qemu_args -M smm=off -d int -D qemulog.txt" ;;
	u) iso="build-${PORT}/nyy-${PORT}-hyper.iso" ;;
	H) qemu_args="$qemu_args -machine hpet=off" ;;
	*) exit 1 ;;
	esac
done

qemu_args="$qemu_args -s"
qemu_args="$qemu_args -no-shutdown -no-reboot"
qemu_args="$qemu_args -cdrom ${iso}"
qemu_args="$qemu_args -smp $qemu_smp"
qemu_args="$qemu_args -m $qemu_mem"

${qemu_command} -M q35 ${qemu_args}
