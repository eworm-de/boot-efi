Name:           org.bus1.boot-efi
Version:        1
Release:        1
Summary:        Bus1 UEFI Boot Manager
License:        LGPL2+
URL:            https://github.com/bus1/boot-efi
Source0:        %{name}.tar.xz
BuildRequires:  autoconf automake
BuildRequires:  gnu-efi-devel
ExclusiveArch:  x86_64
%define debug_package %{nil}

%description
Bus1 UEFI Boot Manager and Stub Kernel

%prep
%setup -q

%build
./autogen.sh
%configure
make %{?_smp_mflags}

%install
%make_install

%files
%doc COPYING
%{_datadir}/org.bus1.boot-efi/bootx64.efi
%{_datadir}/org.bus1.boot-efi/stubx64.efi

%changelog
* Mon Apr 25 2016 <kay@redhat.com> 1-1
- intial release
