Name:       eet
Summary:    Library for speedy data storage, retrieval, and compression
Version:    1.6.0+svn.70375slp2+build01
Release:    1
Group:      System/Libraries
License:    BSD
URL:        http://www.enlightenment.org/
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(gnutls)
BuildRequires:  pkgconfig(openssl)
BuildRequires:  pkgconfig(zlib)
BuildRequires:  libjpeg-devel


%description
Enlightenment DR17 file chunk reading/writing library development files Eet is a tiny library designed to write an arbitary set of chunks of data to a
 file and optionally compress each chunk (very much like a zip file) and allow
 fast random-access reading of the file later on. It does not do zip as zip
 itself has more complexity than we need, and it was much simpler to implement
 this once here.
 .
 This package contains headers and static libraries for development with libeet.


%package devel
Summary:    Library for speedy data storage, retrieval, and compression (devel)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}


%description devel
Enlightenment DR17 file chunk reading/writing library  (devel)


%package tools
Summary:    Library for speedy data storage, retrieval, and compression. (tools)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Provides:   %{name}-bin
Obsoletes:  %{name}-bin


%description tools
Enlightenment DR17 file chunk reading/writing library  (tools)


%prep
%setup -q


%build
export CFLAGS+=" -fvisibility=hidden -fPIC"
export LDFLAGS+=" -fvisibility=hidden -Wl,--hash-style=both -Wl,--as-needed"

%autogen --disable-static
%configure --disable-static \
    --disable-openssl --disable-cypher --disable-signature --disable-gnutls

make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%make_install


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/libeet.so.*


%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/eet.pc


%files tools
%defattr(-,root,root,-)
%{_bindir}/*
%{_datadir}/eet/examples/eet-basic.c
%{_datadir}/eet/examples/eet-data-cipher_decipher.c
%{_datadir}/eet/examples/eet-data-file_descriptor_01.c
%{_datadir}/eet/examples/eet-data-file_descriptor_02.c
%{_datadir}/eet/examples/eet-data-nested.c
%{_datadir}/eet/examples/eet-data-simple.c
%{_datadir}/eet/examples/eet-file.c

