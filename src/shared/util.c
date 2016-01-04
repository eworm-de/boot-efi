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

#include "util.h"

static const EFI_GUID global_guid = EFI_GLOBAL_VARIABLE;

EFI_STATUS efivar_get(const EFI_GUID *vendor, CHAR16 *name, CHAR8 **buffer, UINTN *size) {
        CHAR8 *buf;
        UINTN l;
        EFI_STATUS err;

        if (!vendor)
                vendor = &global_guid;

        l = sizeof(CHAR16 *) * EFI_MAXIMUM_VARIABLE_SIZE;
        buf = AllocatePool(l);
        if (!buf)
                return EFI_OUT_OF_RESOURCES;

        err = uefi_call_wrapper(RT->GetVariable, 5, name, (EFI_GUID *)vendor, NULL, &l, buf);
        if (!EFI_ERROR(err)) {
                *buffer = buf;
                if (size)
                        *size = l;
        } else
                FreePool(buf);
        return err;

}

EFI_STATUS efivar_set(const EFI_GUID *vendor, CHAR16 *name, CHAR8 *buf, UINTN size, BOOLEAN persistent) {
        UINT32 flags;

        if (!vendor)
                vendor = &global_guid;

        flags = EFI_VARIABLE_BOOTSERVICE_ACCESS|EFI_VARIABLE_RUNTIME_ACCESS;
        if (persistent)
                flags |= EFI_VARIABLE_NON_VOLATILE;

        return uefi_call_wrapper(RT->SetVariable, 5, name, (EFI_GUID *)vendor, flags, size, buf);
}

INTN StrniCmp(const CHAR16 *s1, const CHAR16 *s2, UINTN n) {
        while (*s1 && n != 0) {
                if (*s1 >= 'A' && *s1 <= 'Z') {
                        if ((*s1 | 0x20) != (*s2 | 0x20))
                                break;
                } else {
                        if (*s1 != *s2)
                                break;
                }

                s1  += 1;
                s2 += 1;
                n -= 1;
        }

        return n > 0 ? *s1 - *s2 : 0;
}

EFI_STATUS filename_validate_release(EFI_FILE_HANDLE f, const CHAR16 *release, UINTN n) {
        EFI_FILE_INFO *info;
        UINTN name_base_len;

        info = LibFileInfo(f);
        if (!info)
                return EFI_LOAD_ERROR;

        name_base_len = StrLen(info->FileName);
        if (name_base_len < 5)
                return EFI_INVALID_PARAMETER;

        name_base_len -= 4;

        /* require .efi extension */
        if (StriCmp(info->FileName + name_base_len, L".efi") != 0)
                return EFI_INVALID_PARAMETER;

        /* require the file name to match the release name */
        if (StrniCmp(info->FileName, release, n) != 0)
                return EFI_INVALID_PARAMETER;

        return EFI_SUCCESS;
}

INTN file_read_str(EFI_FILE_HANDLE dir, CHAR16 *name, UINTN off, UINTN size, CHAR16 **str) {
        EFI_FILE_HANDLE handle;
        CHAR16 *buf;
        UINTN buflen;
        EFI_STATUS err;
        UINTN len;

        err = uefi_call_wrapper(dir->Open, 5, dir, &handle, name, EFI_FILE_MODE_READ, 0ULL);
        if (EFI_ERROR(err))
                return err;

        if (size == 0) {
                EFI_FILE_INFO *info;

                info = LibFileInfo(handle);
                buflen = info->FileSize;
                FreePool(info);
        } else
                buflen = size ;

        if (off > 0) {
                err = uefi_call_wrapper(handle->SetPosition, 2, handle, off);
                if (EFI_ERROR(err))
                        return err;
        }

        buf = AllocatePool(buflen + sizeof(CHAR16));
        err = uefi_call_wrapper(handle->Read, 3, handle, &buflen, buf);
        if (!EFI_ERROR(err)) {
                buf[buflen / sizeof(CHAR16)] = '\0';
                *str = buf;
                len = buflen / sizeof(CHAR16);
        } else {
                len = err;
                FreePool(buf);
        }

        uefi_call_wrapper(handle->Close, 1, handle);
        return len;
}
