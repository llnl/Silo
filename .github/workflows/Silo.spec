Name:           Silo
Version:        9999
Release:        1%{?dist}
Summary:        Mesh and Field I/O Library and Scientific Database

License:        BSD-3-Clause 
URL:            https://silo.llnl.gov/
Source0:        https://github.com/LLNL/Silo/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake >= 3.12
BuildRequires:  hdf5-devel
BuildRequires:  qt6-qtbase-devel
BuildRequires:  python3-devel
BuildRequires:  json-c-devel

%global silo_desc \
Silo is a C/Fortran/Python API for reading and writing a wide variety of \
scientific data to binary files. Silo files can be easily shared and \
exchanged between wholly independently developed applications running \
on disparate computing platforms. Consequently, Silo facilitates the \
development of general purpose tools for processing scientific data. \
One of the more popular tools to process Silo data is the VisIt \
visualization tool. Silo supports a variety of mesh types including \
simple curves, gridless (point), structured, unstructured-zoo and \
unstructured-arbitrary, block structured AMR, and constructive solid \
geometry (CSG).

%global __brp_check_rpaths %{nil} # Standard rpaths are causing failure

%description
%{silo_desc}

%package devel
Summary:        Development package for  %{name} packages
Requires:       %{name} = %{version}-%{release}
%description devel
%{silo_desc}

This package contains the development files of %{name}.

%prep
%autosetup -p1 -n %{name}

%build
#TODO implement SILO_ENABLE_JSON
%cmake \
  -DBUILD_TESTING=ON \
  -DSILO_ENABLE_SHARED=ON \
  -DSILO_ENABLE_SILOCK=ON \
  -DSILO_ENABLE_SILEX=ON \
  -DSILO_ENABLE_BROWSER=ON \
  -DSILO_ENABLE_FORTRAN=ON \
  -DSILO_ENABLE_HDF5=ON \
  -DSILO_ENABLE_JSON=ON \
  -DSILO_ENABLE_PYTHON_MODULE=ON \
  -DSILO_ENABLE_TESTS=ON \
  -DSILO_ENABLE_INSTALL_LITE_HEADERS=ON \
  -DSILO_BUILD_FOR_BSD_LICENSE=ON \
  %{nil}
%cmake_build

%install
%cmake_install

%check
# FIX: Bug in HDF5-1.14.6 causes checksum testing to fail
# The issue was reported to The HDF Group 10/25/25
%global testargs --exclude-regex '\(checksums\)'
%ctest %{?testargs} || %ctest %{?testargs} --rerun-failed --output-on-failure

%files devel
%doc README.md
%license LICENSE
%{_bindir}/browser
%{_bindir}/silock
%{_bindir}/silodiff
%{_bindir}/silofile
%{_bindir}/silex
%{_bindir}/s2ex.py
%{_includedir}/lite_pdb.h
%{_includedir}/lite_score.h
%{_includedir}/silo.h
%{_includedir}/silo.inc
%{_includedir}/silo_FC.h
%{_includedir}/silo_exports.h
%{_includedir}/silo_f9x.inc
%{_includedir}/pmpio.h
%{_libdir}/cmake/%{name}/SiloConfig.cmake
%{_libdir}/cmake/%{name}/SiloConfigVersion.cmake
%{_libdir}/cmake/%{name}/SiloTargets-release.cmake
%{_libdir}/cmake/%{name}/SiloTargets.cmake
%{_libdir}/libsiloh5.so.4.12.1
%{_libdir}/libsiloh5.so.412
%{_libdir}/libsiloh5.so
%{_libdir}/Silo.so
