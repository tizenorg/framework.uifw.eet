#sbs-git:slp/pkgs/e/eet eet_1.2.0+svn.62590slp2+build01 4fae70dfc08b5d612985ac7b598c59ae92b040b0
Name:       eet
Summary:    Library for speedy data storage, retrieval, and compression
Version:    1.5.0+svn.67705slp2+build01
Release:    2.1
Group:      TO_BE/FILLED_IN
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

%package examples
Summary:    Library for speedy data storage, retrieval, and compression. (tools)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description examples
Enlightenment DR17 file chunk reading/writing library  (tools)

%prep
%setup -q


%build

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
%{_libdir}/*.so.*


%files devel
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/eet.pc

%files tools
/usr/bin/*


%files examples
/usr/share/eet/examples/*
