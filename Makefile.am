ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory
AUTOMAKE_OPTIONS = color-tests parallel-tests

GCC_COLORS ?= 'ooh, shiny!'
export GCC_COLORS

bootlibdir = $(prefix)/lib/bus1/boot-efi

# ------------------------------------------------------------------------------
efi_cppflags = \
	$(EFI_CPPFLAGS) \
	-I$(top_builddir) -include config.h \
	-I$(EFI_INC_DIR)/efi \
	-I$(EFI_INC_DIR)/efi/$(EFI_ARCH) \
	-DEFI_MACHINE_TYPE_NAME=\"$(EFI_MACHINE_TYPE_NAME)\"

efi_cflags = \
	$(EFI_CFLAGS) \
	-Wall \
	-Wextra \
	-std=gnu90 \
	-nostdinc \
	-ggdb -O0 \
	-fpic \
	-fshort-wchar \
	-nostdinc \
	-ffreestanding \
	-fno-strict-aliasing \
	-fno-stack-protector \
	-Wsign-compare \
	-Wno-missing-field-initializers

if ARCH_X86_64
efi_cflags += \
	-mno-red-zone \
	-mno-sse \
	-mno-mmx \
	-DEFI_FUNCTION_WRAPPER \
	-DGNU_EFI_USE_MS_ABI
endif

if ARCH_IA32
efi_cflags += \
	-mno-sse \
	-mno-mmx
endif

efi_ldflags = \
	$(EFI_LDFLAGS) \
	-T $(EFI_LDS_DIR)/elf_$(EFI_ARCH)_efi.lds \
	-shared \
	-Bsymbolic \
	-nostdlib \
	-znocombreloc \
	-L $(EFI_LIB_DIR) \
	$(EFI_LDS_DIR)/crt0-efi-$(EFI_ARCH).o

# Aarch64 and ARM32 don't have an EFI capable objcopy. Use 'binary' instead,
# and add required symbols manually.
if ARCH_AARCH64
efi_ldflags += --defsym=EFI_SUBSYSTEM=0xa
EFI_FORMAT = -O binary
else
EFI_FORMAT = --target=efi-app-$(EFI_ARCH)
endif

# ------------------------------------------------------------------------------
boot_headers = \
	src/util.h \
	src/console.h \
	src/graphics.h \
	src/pefile.h

boot_sources = \
	src/util.c \
	src/console.c \
	src/graphics.c \
	src/pefile.c \
	src/boot.c

EXTRA_DIST = $(boot_sources) $(boot_headers)

boot_objects = $(addprefix $(top_builddir)/,$(boot_sources:.c=.o))
boot_solib = $(top_builddir)/boot.so
boot = boot$(EFI_MACHINE_TYPE_NAME).efi

bootlib_DATA = $(boot)
CLEANFILES = $(boot_objects) $(boot_solib) $(boot)

$(top_builddir)/src/%.o: $(top_srcdir)/src/%.c $(addprefix $(top_srcdir)/,$(boot_headers))
	@$(MKDIR_P) $(top_builddir)/src/
	$(AM_V_CC)$(EFI_CC) $(efi_cppflags) $(efi_cflags) -c $< -o $@

$(boot_solib): $(boot_objects)
	$(AM_V_CCLD)$(LD) $(efi_ldflags) $(boot_objects) \
		-o $@ -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name); \
	nm -D -u $@ | grep ' U ' && exit 1 || :

$(boot): $(boot_solib)
	$(AM_V_GEN)$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	  -j .dynsym -j .rel -j .rela -j .reloc $(EFI_FORMAT) $< $@

# ------------------------------------------------------------------------------
stub_headers = \
	src/util.h \
	src/pefile.h \
	src/graphics.h \
	src/splash.h \
	src/linux.h

stub_sources = \
	src/util.c \
	src/pefile.c \
	src/graphics.c \
	src/splash.c \
	src/linux.c \
	src/stub.c

EXTRA_DIST += \
	$(stub_sources) \
	$(stub_headers) \
	test/splash.bmp

stub_objects = $(addprefix $(top_builddir)/,$(stub_sources:.c=.o))
stub_solib = $(top_builddir)/stub.so
stub = stub$(EFI_MACHINE_TYPE_NAME).efi

bootlib_DATA += $(stub)
CLEANFILES += $(stub_objects) $(stub_solib) $(stub)

$(top_builddir)/src/%.o: $(top_srcdir)/src/%.c $(addprefix $(top_srcdir)/,$(stub_headers))
	@$(MKDIR_P) $(top_builddir)/src
	$(AM_V_CC)$(EFI_CC) $(efi_cppflags) $(efi_cflags) -c $< -o $@

$(stub_solib): $(stub_objects)
	$(AM_V_CCLD)$(LD) $(efi_ldflags) $(stub_objects) \
		-o $@ -lefi -lgnuefi $(shell $(CC) -print-libgcc-file-name); \
	nm -D -u $@ | grep ' U ' && exit 1 || :

$(stub): $(stub_solib)
	$(AM_V_GEN)$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
	  -j .dynsym -j .rel -j .rela -j .reloc $(EFI_FORMAT) $< $@

# ------------------------------------------------------------------------------
CLEANFILES += test-efi-disk.img

test-efi-disk.img: $(stub) test/test-efi-create-disk.sh
	$(AM_V_GEN)test/test-efi-create-disk.sh

test-efi: test-efi-disk.img
	$(QEMU) -machine accel=kvm -m 1024 -bios $(QEMU_BIOS) -snapshot test-efi-disk.img

EXTRA_DIST += test/test-efi-create-disk.sh
