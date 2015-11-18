AC_INIT([boot-efi],
        [1],
        [http://www.github.com/bus1/boot-efi],
        [boot-efi],
        [http://www.github.com/bus1/boot-efi])

AC_CONFIG_SRCDIR([src/boot.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules tar-pax no-dist-gzip dist-xz subdir-objects])
AM_SILENT_RULES([yes])

AC_CANONICAL_HOST
AC_SYS_LARGEFILE

AC_PROG_CC

# do not try to use things like -std=c99 for efi compilation
EFI_CC=$CC
AC_SUBST([EFI_CC])

AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_GCC_TRADITIONAL

AC_PROG_MKDIR_P
AC_CHECK_TOOL(OBJCOPY, objcopy)

# ------------------------------------------------------------------------------
SET_ARCH(X86_64, x86_64*)
SET_ARCH(IA32, i*86*)
SET_ARCH(MIPS, mips*)
SET_ARCH(AARCH64, aarch64*)

AC_CHECK_TOOL(EFI_CC, gcc)

EFI_ARCH=`echo $host | sed "s/\(-\).*$//"`

AM_COND_IF(ARCH_IA32, [
        EFI_ARCH=ia32
        EFI_MACHINE_TYPE_NAME=ia32])

AM_COND_IF(ARCH_X86_64, [
        EFI_MACHINE_TYPE_NAME=x64])

AM_COND_IF(ARCH_AARCH64, [
        EFI_MACHINE_TYPE_NAME=aa64])

AC_SUBST([EFI_ARCH])
AC_SUBST([EFI_MACHINE_TYPE_NAME])

AC_CHECK_HEADERS(efi/${EFI_ARCH}/efibind.h,
        [AC_DEFINE(HAVE_GNUEFI, 1, [Define if gnuefi is available])
         have_gnuefi=yes],
        [AS_IF([test "x$enable_gnuefi" = xyes],
               [AC_MSG_ERROR([*** gnuefi support requested but headers not found])])
])

efiroot=$(echo $(cd /usr/lib/$(${EFI_CC} -print-multi-os-directory); pwd))

EFI_LIB_DIR="$efiroot"
AC_ARG_WITH(efi-libdir,
        AS_HELP_STRING([--with-efi-libdir=PATH], [Path to EFI lib directory]),
        [EFI_LIB_DIR="$withval"], [EFI_LIB_DIR="$efiroot"]
)
AC_SUBST([EFI_LIB_DIR])

have_efi_lds=no
AC_ARG_WITH(efi-ldsdir,
        AS_HELP_STRING([--with-efi-ldsdir=PATH], [Path to EFI lds directory]),
        [EFI_LDS_DIR="$withval" && AS_IF([test -f "${EFI_LDS_DIR}/elf_${EFI_ARCH}_efi.lds"],
                [have_efi_lds=yes])],
        [AS_FOR([DIR], [EFI_LDS_DIR], ["${EFI_LIB_DIR}/gnuefi" "${EFI_LIB_DIR}"],
                [AS_IF([test -f "${EFI_LDS_DIR}/elf_${EFI_ARCH}_efi.lds"],
                               [have_efi_lds=yes && break])])])
AS_IF([test "x$have_efi_lds" = xyes],
      [AC_SUBST([EFI_LDS_DIR])],
      [AS_IF([test "x$enable_gnuefi" = xyes],
             [AC_MSG_ERROR([*** gnuefi support requested but files not found])],
             [have_gnuefi=no])])

AC_ARG_WITH(efi-includedir,
        AS_HELP_STRING([--with-efi-includedir=PATH], [Path to EFI include directory]),
        [EFI_INC_DIR="$withval"], [EFI_INC_DIR="/usr/include"]
)
AC_SUBST([EFI_INC_DIR])

# ------------------------------------------------------------------------------
# QEMU and OVMF UEFI firmware
AS_IF([test x"$cross_compiling" = "xyes"], [], [
        AC_PATH_PROG([QEMU], [qemu-system-x86_64])
        AC_CHECK_FILE([/usr/share/qemu/bios-ovmf.bin], [QEMU_BIOS=/usr/share/qemu/bios-ovmf.bin])
        AC_SUBST([QEMU_BIOS])
])

# ------------------------------------------------------------------------------
AC_CONFIG_FILES([
        Makefile
])

AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE_NAME $VERSION

        prefix:                  ${prefix}
        arch:                    $EFI_ARCH
        EFI machine type:        $EFI_MACHINE_TYPE_NAME

        EFI libdir:              ${EFI_LIB_DIR}
        EFI ldsdir:              ${EFI_LDS_DIR}
        EFI includedir:          ${EFI_INC_DIR}

        QEMU:                    ${QEMU}
        QEMU OVMF:               ${QEMU_BIOS}
])
