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
#include "shared/graphics.h"

EFI_STATUS graphics_mode(BOOLEAN on) {
        #define EFI_CONSOLE_CONTROL_PROTOCOL_GUID \
                { 0xf42f7782, 0x12e, 0x4c12, { 0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21 } };

        struct _EFI_CONSOLE_CONTROL_PROTOCOL;

        typedef enum {
                EfiConsoleControlScreenText,
                EfiConsoleControlScreenGraphics,
                EfiConsoleControlScreenMaxValue,
        } EFI_CONSOLE_CONTROL_SCREEN_MODE;

        typedef EFI_STATUS (EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE)(
                struct _EFI_CONSOLE_CONTROL_PROTOCOL *This,
                EFI_CONSOLE_CONTROL_SCREEN_MODE *Mode,
                BOOLEAN *UgaExists,
                BOOLEAN *StdInLocked
        );

        typedef EFI_STATUS (EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE)(
                struct _EFI_CONSOLE_CONTROL_PROTOCOL *This,
                EFI_CONSOLE_CONTROL_SCREEN_MODE Mode
        );

        typedef EFI_STATUS (EFIAPI *EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN)(
                struct _EFI_CONSOLE_CONTROL_PROTOCOL *This,
                CHAR16 *Password
        );

        typedef struct _EFI_CONSOLE_CONTROL_PROTOCOL {
                EFI_CONSOLE_CONTROL_PROTOCOL_GET_MODE GetMode;
                EFI_CONSOLE_CONTROL_PROTOCOL_SET_MODE SetMode;
                EFI_CONSOLE_CONTROL_PROTOCOL_LOCK_STD_IN LockStdIn;
        } EFI_CONSOLE_CONTROL_PROTOCOL;

        EFI_GUID ConsoleControlProtocolGuid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
        EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl = NULL;
        EFI_CONSOLE_CONTROL_SCREEN_MODE new;
        EFI_CONSOLE_CONTROL_SCREEN_MODE current;
        BOOLEAN uga_exists;
        BOOLEAN stdin_locked;
        EFI_STATUS r;

        r = LibLocateProtocol(&ConsoleControlProtocolGuid, (VOID **)&ConsoleControl);
        if (EFI_ERROR(r))
                /* console control protocol is nonstandard and might not exist. */
                return r == EFI_NOT_FOUND ? EFI_SUCCESS : r;

        /* check current mode */
        r = uefi_call_wrapper(ConsoleControl->GetMode, 4, ConsoleControl, &current, &uga_exists, &stdin_locked);
        if (EFI_ERROR(r))
                return r;

        /* do not touch the mode */
        new  = on ? EfiConsoleControlScreenGraphics : EfiConsoleControlScreenText;
        if (new == current)
                return EFI_SUCCESS;

        r = uefi_call_wrapper(ConsoleControl->SetMode, 2, ConsoleControl, new);

        /* some firmware enables the cursor when switching modes */
        uefi_call_wrapper(ST->ConOut->EnableCursor, 2, ST->ConOut, FALSE);

        return r;
}
