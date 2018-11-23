Name:		linecook
Version:	999.999
Release:	99999%{?dist}
Summary:	Command line editor

Group:		Development/Libraries
License:	BSD
URL:		https://github.com/injinj/%{name}
Source0:	%{name}-%{version}-99999.tar.gz
BuildRoot:	${_tmppath}
BuildArch:      x86_64
BuildRequires:  gcc-c++
BuildRequires:  chrpath
Prefix:	        /usr

%description
A command line editing library.

%prep
%setup -q


%define _unpackaged_files_terminate_build 0
%define _missing_doc_files_terminate_build 0
%define _missing_build_ids_terminate_build 0
%define _include_gdb_index 1

%build
make build_dir=./usr %{?_smp_mflags} dist_bins
cp -a ./include ./usr/include

%install
rm -rf %{buildroot}
mkdir -p  %{buildroot}

# in builddir
cp -a * %{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
/usr/bin/*
/usr/lib64/*
/usr/include/*

%post
echo "${RPM_INSTALL_PREFIX}/lib64" > /etc/ld.so.conf.d/linecook.conf
ldconfig

%postun
rm -f /etc/ld.so.conf.d/linecook.conf
ldconfig

%changelog
* __DATE__ <gchrisanderson@gmail.com>
- Hello world
