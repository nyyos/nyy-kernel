#!/bin/bash

IMG_SIZE="${1:-4G}"

qemu-img create nvme.img ${IMG_SIZE}
