Name:       eet
Summary:    Library for speedy data storage, retrieval, and compression
Version:    1.4.999.svn60246
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO BE FILLED IN
URL:        http://www.enlightenment.org/
Source0:    http://download.enlightenment.org/releases/eet-%{version}.tar.gz
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

%package bin
Summary:    Library for speedy data storage, retrieval, and compression. (bin)
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description bin
Enlightenment DR17 file chunk reading/writing library  (bin)


%prep
%setup -q -n %{name}


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
%defattr(-,root,root,-)
%{_libdir}/*.so.*


%files devel
%defattr(-,root,root,-)
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/pkgconfig/eet.pc

%files bin
%defattr(-,root,root,-)
/usr/bin/*

