#!/bin/bash

set -e

ROOT=$(mktemp -d /tmp/efi-tmpXXX)

# create GPT table with EFI System Partition
rm -f efi-disk.img
dd if=/dev/null of=efi-disk.img bs=1MiB seek=1024 count=1

sfdisk efi-disk.img << EOF
label: gpt
start=1MiB, size=511MiB, type=c12a7328-f81f-11d2-ba4b-00a0c93ec93b, name="ESP"
            size=512MiB, type=e0243462-d2d0-4c3b-ad28-b365f2da3b4d, name="bus1"
EOF

# create FAT32 file system
LOOP=$(losetup --show -f -P efi-disk.img)
mkfs.vfat -F32 ${LOOP}p1
mkdir -p $ROOT
mount ${LOOP}p1 $ROOT

mkdir -p $ROOT/EFI/Boot
cp bootx64.efi $ROOT/EFI/Boot/bootx64.efi

mkdir $ROOT/EFI/bus1
echo -n "bus1-0815" | iconv -f UTF-8 -t UTF-16LE > $ROOT/release.txt
echo -n "foo=yes quiet" | iconv -f UTF-8 -t UTF-16LE > $ROOT/options.txt

linux=linux
test -e "$linux" || linux=/boot/$(cat /etc/machine-id)/$(uname -r)/linux
test -e "$linux" || linux=/vmlinuz
test -e "$linux" || exit 1

initrd=initrd
test -e "$initrd" || initrd=/boot/$(cat /etc/machine-id)/$(uname -r)/initrd
test -e "$initrd" || initrd=/initrd.img
test -e "$initrd" || exit 1

objcopy \
  --add-section .release=$ROOT/release.txt --change-section-vma .release=0x20000 \
  --add-section .options=$ROOT/options.txt --change-section-vma .options=0x30000 \
  --add-section .splash=test/bus1.bmp --change-section-vma .splash=0x40000 \
  --add-section .linux=$linux --change-section-vma .linux=0x2000000 \
  --add-section .initrd=$initrd --change-section-vma .initrd=0x3000000 \
  stubx64.efi $ROOT/EFI/bus1/bus1.efi

sync
umount $ROOT
rmdir $ROOT
losetup -d $LOOP
