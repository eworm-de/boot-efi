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
#include "console.h"

#define EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID \
        { 0xdd9e7534, 0x7762, 0x4698, { 0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa } }

struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET_EX)(
        struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
        BOOLEAN ExtendedVerification
);

typedef UINT8 EFI_KEY_TOGGLE_STATE;

typedef struct {
        UINT32 KeyShiftState;
        EFI_KEY_TOGGLE_STATE KeyToggleState;
} EFI_KEY_STATE;

typedef struct {
        EFI_INPUT_KEY Key;
        EFI_KEY_STATE KeyState;
} EFI_KEY_DATA;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY_EX)(
        struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
        EFI_KEY_DATA *KeyData
);

typedef EFI_STATUS (EFIAPI *EFI_SET_STATE)(
        struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
        EFI_KEY_TOGGLE_STATE *KeyToggleState
);

typedef EFI_STATUS (EFIAPI *EFI_KEY_NOTIFY_FUNCTION)(
        EFI_KEY_DATA *KeyData
);

typedef EFI_STATUS (EFIAPI *EFI_REGISTER_KEYSTROKE_NOTIFY)(
        struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
        EFI_KEY_DATA KeyData,
        EFI_KEY_NOTIFY_FUNCTION KeyNotificationFunction,
        VOID **NotifyHandle
);

typedef EFI_STATUS (EFIAPI *EFI_UNREGISTER_KEYSTROKE_NOTIFY)(
        struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
        VOID *NotificationHandle
);

typedef struct _EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL {
        EFI_INPUT_RESET_EX Reset;
        EFI_INPUT_READ_KEY_EX ReadKeyStrokeEx;
        EFI_EVENT WaitForKeyEx;
        EFI_SET_STATE SetState;
        EFI_REGISTER_KEYSTROKE_NOTIFY RegisterKeyNotify;
        EFI_UNREGISTER_KEYSTROKE_NOTIFY UnregisterKeyNotify;
} EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL;

EFI_STATUS console_key_read(UINT64 *key, BOOLEAN wait) {
        EFI_GUID EfiSimpleTextInputExProtocolGuid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
        static EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *TextInputEx;
        static BOOLEAN checked;
        UINTN index;
        EFI_INPUT_KEY k;
        EFI_STATUS r;

        if (!checked) {
                r = LibLocateProtocol(&EfiSimpleTextInputExProtocolGuid, (VOID **)&TextInputEx);
                if (EFI_ERROR(r))
                        TextInputEx = NULL;

                checked = TRUE;
        }

        /* wait until key is pressed */
        if (wait) {
                if (TextInputEx)
                        uefi_call_wrapper(BS->WaitForEvent, 3, 1, &TextInputEx->WaitForKeyEx, &index);
                else
                        uefi_call_wrapper(BS->WaitForEvent, 3, 1, &ST->ConIn->WaitForKey, &index);
        }

        if (TextInputEx) {
                EFI_KEY_DATA keydata;
                UINT64 keypress;

                r = uefi_call_wrapper(TextInputEx->ReadKeyStrokeEx, 2, TextInputEx, &keydata);
                if (!EFI_ERROR(r)) {
                        UINT32 shift = 0;

                        /* do not distinguish between left and right keys */
                        if (keydata.KeyState.KeyShiftState & EFI_SHIFT_STATE_VALID) {
                                if (keydata.KeyState.KeyShiftState & (EFI_RIGHT_CONTROL_PRESSED|EFI_LEFT_CONTROL_PRESSED))
                                        shift |= EFI_CONTROL_PRESSED;
                                if (keydata.KeyState.KeyShiftState & (EFI_RIGHT_ALT_PRESSED|EFI_LEFT_ALT_PRESSED))
                                        shift |= EFI_ALT_PRESSED;
                        };

                        /* 32 bit modifier keys + 16 bit scan code + 16 bit unicode */
                        keypress = KEYPRESS(shift, keydata.Key.ScanCode, keydata.Key.UnicodeChar);
                        if (keypress > 0) {
                                *key = keypress;
                                return 0;
                        }
                }
        }

        /* fallback for firmware which does not support SimpleTextInputExProtocol
         *
         * This is also called in case ReadKeyStrokeEx did not return a key, because
         * some broken firmwares offer SimpleTextInputExProtocol, but never acually
         * handle any key. */
        r  = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &k);
        if (EFI_ERROR(r))
                return r;

        *key = KEYPRESS(0, k.ScanCode, k.UnicodeChar);
        return 0;
}
