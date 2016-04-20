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
#include "shared/pefile.h"

struct DosFileHeader {
        UINT8   Magic[2];
        UINT16  LastSize;
        UINT16  nBlocks;
        UINT16  nReloc;
        UINT16  HdrSize;
        UINT16  MinAlloc;
        UINT16  MaxAlloc;
        UINT16  ss;
        UINT16  sp;
        UINT16  Checksum;
        UINT16  ip;
        UINT16  cs;
        UINT16  RelocPos;
        UINT16  nOverlay;
        UINT16  reserved[4];
        UINT16  OEMId;
        UINT16  OEMInfo;
        UINT16  reserved2[10];
        UINT32  ExeHeader;
} __attribute__((packed));

#define PE_HEADER_MACHINE_I386          0x014c
#define PE_HEADER_MACHINE_X64           0x8664
struct PeFileHeader {
        UINT16  Machine;
        UINT16  NumberOfSections;
        UINT32  TimeDateStamp;
        UINT32  PointerToSymbolTable;
        UINT32  NumberOfSymbols;
        UINT16  SizeOfOptionalHeader;
        UINT16  Characteristics;
} __attribute__((packed));

struct PeSectionHeader {
        UINT8   Name[8];
        UINT32  VirtualSize;
        UINT32  VirtualAddress;
        UINT32  SizeOfRawData;
        UINT32  PointerToRawData;
        UINT32  PointerToRelocations;
        UINT32  PointerToLinenumbers;
        UINT16  NumberOfRelocations;
        UINT16  NumberOfLinenumbers;
        UINT32  Characteristics;
} __attribute__((packed));

EFI_STATUS pefile_locate_sections(EFI_FILE_HANDLE handle,
                                  CHAR8 **sections, UINTN n_sections,
                                  UINTN *addrs, UINTN *offsets, UINTN *sizes) {
        struct DosFileHeader dos;
        uint8_t magic[4];
        struct PeFileHeader pe;
        UINTN len;
        EFI_STATUS r;

        /* MS-DOS stub */
        len = sizeof(dos);
        r = uefi_call_wrapper(handle->Read, 3, handle, &len, &dos);
        if (EFI_ERROR(r))
                return r;

        if (len != sizeof(dos))
                return EFI_INVALID_PARAMETER;

        if (CompareMem(dos.Magic, "MZ", 2) != 0)
                return EFI_INVALID_PARAMETER;

        r = uefi_call_wrapper(handle->SetPosition, 2, handle, dos.ExeHeader);
        if (EFI_ERROR(r))
                return r;

        /* PE header */
        len = sizeof(magic);
        r = uefi_call_wrapper(handle->Read, 3, handle, &len, &magic);
        if (EFI_ERROR(r))
                return r;

        if (len != sizeof(magic))
                return EFI_INVALID_PARAMETER;

        if (CompareMem(magic, "PE\0\0", 2) != 0)
                return EFI_INVALID_PARAMETER;

        len = sizeof(pe);
        r = uefi_call_wrapper(handle->Read, 3, handle, &len, &pe);
        if (EFI_ERROR(r))
                return r;

        if (len != sizeof(pe))
                return EFI_INVALID_PARAMETER;

        /* PE32+ Subsystem type */
        if (pe.Machine != PE_HEADER_MACHINE_X64 &&
            pe.Machine != PE_HEADER_MACHINE_I386)
                return EFI_INVALID_PARAMETER;

        if (pe.NumberOfSections > 96)
                return EFI_INVALID_PARAMETER;

        /* the sections start directly after the headers */
        r = uefi_call_wrapper(handle->SetPosition, 2, handle, dos.ExeHeader + sizeof(magic) + sizeof(pe) + pe.SizeOfOptionalHeader);
        if (EFI_ERROR(r))
                return r;

        for (UINTN i = 0; i < pe.NumberOfSections; i++) {
                struct PeSectionHeader sect;

                len = sizeof(sect);
                r = uefi_call_wrapper(handle->Read, 3, handle, &len, &sect);
                if (EFI_ERROR(r))
                        return r;

                if (len != sizeof(sect))
                        return EFI_INVALID_PARAMETER;

                for (UINTN n = 0; n < n_sections; n++) {
                        if (CompareMem(sect.Name, sections[n], strlena(sections[n])) != 0)
                                continue;

                        if (addrs)
                                addrs[n] = (UINTN)sect.VirtualAddress;
                        if (offsets)
                                offsets[n] = (UINTN)sect.PointerToRawData;
                        if (sizes)
                                sizes[n] = (UINTN)sect.VirtualSize;
                }
        }

        return 0;
}
