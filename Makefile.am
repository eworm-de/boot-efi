ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory
AUTOMAKE_OPTIONS = color-tests parallel-tests

GCC_COLORS ?= 'ooh, shiny!'
export GCC_COLORS

EFI_V_CC = $(efi__v_CC_$(V))
efi__v_CC_ = $(efi__v_CC_$(AM_DEFAULT_VERBOSITY))
efi__v_CC_0 = @echo "  EFICC   " $@;
efi__v_CC_1 = 
EFI_V_CCLD = $(efi__v_CCLD_$(V))
efi__v_CCLD_ = $(efi__v_CCLD_$(AM_DEFAULT_VERBOSITY))
efi__v_CCLD_0 = @echo "  EFICCLD " $@;
efi__v_CCLD_1 = 

bootlibdir = $(prefix)/lib/bus1/boot-efi

# ------------------------------------------------------------------------------
efi_cppflags = \
	$(EFI_CPPFLAGS) \
	-I$(top_builddir) -include config.h \
	-I$(top_srcdir)/src/shared \
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
	src/shared/disk.h \
	src/shared/graphics.h \
	src/shared/pefile.h \
	src/shared/util.h \
	src/boot/console.h

boot_sources = \
	src/shared/disk.c \
	src/shared/graphics.c \
	src/shared/pefile.c \
	src/shared/util.c \
	src/boot/console.c \
	src/boot/main.c

EXTRA_DIST = $(boot_sources) $(boot_headers)

boot_objects = $(addprefix $(top_builddir)/,$(boot_sources:.c=.o))
boot_solib = $(top_builddir)/boot.so
boot = boot$(EFI_MACHINE_TYPE_NAME).efi

bootlib_DATA = $(boot)
CLEANFILES = $(boot_objects) $(boot_solib) $(boot)

$(top_builddir)/src/%.o: $(top_srcdir)/src/%.c $(addprefix $(top_srcdir)/,$(boot_headers))
	$(AM_V_at)$(MKDIR_P) $(top_builddir)/src/
	$(EFI_V_CC)$(EFI_CC) -I$(top_srcdir)/src/boot $(efi_cppflags) $(efi_cflags) -c $< -o $@

$(boot_solib): $(boot_objects)
	$(EFI_V_CCLD)$(LD) $(efi_ldflags) $(boot_objects) \
		-o $@ -lefi -lgnuefi $(shell $(EFI_CC) -print-libgcc-file-name); \
		$(NM) -D -u $@ | grep ' U ' && exit 1 || :

$(boot): $(boot_solib)
	$(AM_V_GEN)$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym -j .rel -j .rela -j .reloc $(EFI_FORMAT) $< $@

# ------------------------------------------------------------------------------
stub_headers = \
	src/shared/disk.h \
	src/shared/graphics.h \
	src/shared/pefile.h \
	src/shared/util.h \
	src/stub/linux.h \
	src/stub/splash.h

stub_sources = \
	src/shared/disk.c \
	src/shared/graphics.c \
	src/shared/pefile.c \
	src/shared/util.c \
	src/stub/linux.c \
	src/stub/splash.c \
	src/stub/main.c

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
	$(AM_V_at)$(MKDIR_P) $(top_builddir)/src
	$(EFI_V_CC)$(EFI_CC) -I$(top_srcdir)/src/stub $(efi_cppflags) $(efi_cflags) -c $< -o $@

$(stub_solib): $(stub_objects)
	$(EFI_V_CCLD)$(LD) $(efi_ldflags) $(stub_objects) \
		-o $@ -lefi -lgnuefi $(shell $(EFI_CC) -print-libgcc-file-name); \
		$(NM) -D -u $@ | grep ' U ' && exit 1 || :

$(stub): $(stub_solib)
	$(AM_V_GEN)$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
		-j .dynsym -j .rel -j .rela -j .reloc $(EFI_FORMAT) $< $@

# ------------------------------------------------------------------------------
CLEANFILES += efi-disk.img

efi-disk.img: $(boot) $(stub) test/create-efi-disk.sh
	$(AM_V_GEN)test/create-efi-disk.sh
.DELETE_ON_ERROR: efi-disk.img

test-efi: efi-disk.img
	$(QEMU) -machine accel=kvm -m 1024 -bios $(QEMU_BIOS) -drive format=raw,file=efi-disk.img
.PHONY: test-efi

EXTRA_DIST += test/create-efi-disk.sh
