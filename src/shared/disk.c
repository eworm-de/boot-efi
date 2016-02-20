/***
  This file is part of bus1. See COPYING for details.

  bus1 is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  bus1 is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with bus1; If not, see <http://www.gnu.org/licenses/>.
***/

#include <efi.h>
#include <efilib.h>

#include "shared/util.h"

typedef struct {
        UINT8   signature[8];
        UINT32  revision;
        UINT32  header_size;
        UINT32  header_crc32;
        UINT32  reserved;
        UINT64  header_lba;
        UINT64  alternate_header_lba;
        UINT64  first_usable_lba;
        UINT64  last_usable_lba;
        UINT8   disk_guid[16];
        UINT64  entry_lba;
        UINT32  entry_count;
        UINT32  entry_size;
        UINT32  entry_crc32;
        UINT8   reserved2[420];
} GPTHeader;

EFI_DEVICE_PATH *path_parent(EFI_DEVICE_PATH *path, EFI_DEVICE_PATH *node) {
        EFI_DEVICE_PATH *parent;
        UINTN len;

        len = (UINT8 *)NextDevicePathNode(node) - (UINT8 *)path;
        parent = (EFI_DEVICE_PATH *)AllocatePool(len + sizeof(EFI_DEVICE_PATH));
        CopyMem(parent, path, len);
        CopyMem((UINT8 *)parent + len, EndDevicePath, sizeof(EFI_DEVICE_PATH));
        return parent;
}

EFI_STATUS disk_get_disk_uuid(EFI_HANDLE *part_handle, CHAR16 uuid[37]) {
        EFI_DEVICE_PATH *part_path;
        EFI_DEVICE_PATH *node;
        EFI_STATUS r;

        part_path = DevicePathFromHandle(part_handle);
        if (!part_path)
                return EFI_NOT_FOUND;

        for (node = part_path; !IsDevicePathEnd(node); node = NextDevicePathNode(node)) {
                _c_cleanup_(CFreePoolP) EFI_DEVICE_PATH *disk_path = NULL;
                EFI_DEVICE_PATH *p;
                EFI_HANDLE disk_handle;
                EFI_BLOCK_IO *block_io;
                GPTHeader gpt_header = {};

                if (DevicePathType(node) != MESSAGING_DEVICE_PATH)
                        continue;

                disk_path = path_parent(part_path, node);
                p = disk_path;
                r = uefi_call_wrapper(BS->LocateDevicePath, 3, &BlockIoProtocol, &p, &disk_handle);
                if (EFI_ERROR(r))
                        continue;

                r = uefi_call_wrapper(BS->HandleProtocol, 3, disk_handle, &BlockIoProtocol, (VOID **)&block_io);
                if (EFI_ERROR(r))
                        continue;

                if (block_io->Media->LogicalPartition || !block_io->Media->MediaPresent)
                        continue;

                r = uefi_call_wrapper(block_io->ReadBlocks, 5, block_io, block_io->Media->MediaId, 1, sizeof(GPTHeader), &gpt_header);
                if (EFI_ERROR(r))
                        continue;

                if (CompareMem(gpt_header.signature, "EFI PART", sizeof(gpt_header.signature)) != 0)
                        continue;

                if (gpt_header.revision != 0x00010000)
                        continue;

                if (gpt_header.header_size < 92 || gpt_header.header_size > 512)
                        continue;

                GuidToString(uuid, (EFI_GUID *)&gpt_header.disk_guid);
                return 0;
        }

        return EFI_NOT_FOUND;
}
