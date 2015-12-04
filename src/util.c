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

#ifdef __x86_64__
UINT64 ticks_read(VOID) {
        UINT64 a, d;
        __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
        return (d << 32) | a;
}
#elif defined(__i386__)
UINT64 ticks_read(VOID) {
        UINT64 val;
        __asm__ volatile ("rdtsc" : "=A" (val));
        return val;
}
#else
UINT64 ticks_read(VOID) {
        UINT64 val = 1;
        return val;
}
#endif

/* count TSC ticks during a millisecond delay */
UINT64 ticks_freq(VOID) {
        UINT64 ticks_start, ticks_end;

        ticks_start = ticks_read();
        uefi_call_wrapper(BS->Stall, 1, 1000);
        ticks_end = ticks_read();

        return (ticks_end - ticks_start) * 1000;
}

UINT64 time_usec(VOID) {
        UINT64 ticks;
        static UINT64 freq;

        ticks = ticks_read();
        if (ticks == 0)
                return 0;

        if (freq == 0) {
                freq = ticks_freq();
                if (freq == 0)
                        return 0;
        }

        return 1000 * 1000 * ticks / freq;
}

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

CHAR8 *strchra(CHAR8 *s, CHAR8 c) {
        do {
                if (*s == c)
                        return s;
        } while (*s++);
        return NULL;
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
