dnl Process this file with autoconf to produce a configure script.

dnl GiNaC library version information.
dnl
dnl When making releases do whatever you want with the major,
dnl minor and micro project version numbers ; but for the library
dnl version, you must be more careful (the following lines are
dnl to be read as an algorithm, don't stop at the first one!) :
dnl 1. increment lt_revision
dnl 2. if any interface has been added, removed or modified since the
dnl    last release, then increment lt_current and set lt_revision to 0
dnl 3. if any interface has been added, then increment lt_age
dnl 4. if any interface has been removed, then set lt_age to zero
dnl
dnl In case you wonder why such instructions:
dnl http://www.sourceware.org/autobook/autobook/autobook_91.html#SEC91
dnl The following article is helpful too:
dnl http://www.freesoftwaremagazine.com/articles/building_shared_libraries_once_using_autotools

dnl if you change this YOU MUST also change the libtool version below
m4_define([ginac_major_version], [0])
m4_define([ginac_minor_version], [7])
m4_define([ginac_micro_version], [29])

m4_define([lt_current], [21])
m4_define([lt_revision], [7])
m4_define([lt_age], [3])


m4_define([ginac_version], [ginac_major_version.ginac_minor_version.ginac_micro_version])
m4_define([ginac_release], [ginac_major_version.ginac_minor_version])

AC_INIT([pynac],[ginac_version],[<pynac-devel@googlegroups.com>])
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR(ginac/basic.cpp)
AC_CONFIG_HEADERS(config.h)
dnl This defines PACKAGE and VERSION.
AM_INIT_AUTOMAKE([gnu 1.7 dist-bzip2])
# Allow "configure --disable-maintainer-mode" to disable timestamp checking 
AM_MAINTAINER_MODE([enable])

AM_PATH_PYTHON([2.7])

dnl Adapted from CPython 3.8 cpython/Misc/python-config.in
AC_MSG_CHECKING([for Python preprocessor flags])
PYTHON_CPPFLAGS=$($PYTHON - <<EOD
import sysconfig
print(' '.join(list(('-I' + sysconfig.get_path('include'),
                     '-I' + sysconfig.get_path('platinclude'),))
               + sysconfig.get_config_var('CFLAGS').split()))
EOD
)
AC_MSG_RESULT([$PYTHON_CPPFLAGS])
AC_SUBST([PYTHON_CPPFLAGS])

AC_MSG_CHECKING([for Python library flags])
PYTHON_LDFLAGS=$(CC=dummy-cc $PYTHON - <<EOD
from distutils.ccompiler import new_compiler
from distutils.sysconfig import customize_compiler
compiler = new_compiler()
customize_compiler(compiler)
import sys
import sysconfig
if compiler.linker_so.pop(0) != 'dummy-cc':
    sys.stderr.write('Warning: linker_so has a changed linker command; avoid setting the LDSHARED environment variable\n')
print(' '.join(compiler.linker_so))
EOD
)
AC_MSG_RESULT([$PYTHON_LDFLAGS])
AC_SUBST([PYTHON_LDFLAGS])

AC_CANONICAL_HOST
AC_MSG_CHECKING([for Cygwin])
case $host_os in
     *cygwin*)
        AM_CONDITIONAL(CYGWIN, true)
        dnl On Cygwin, we need to link through with the Python library,
        dnl otherwise libtool complains.
        PYTHON_LDFLAGS="$PYTHON_LDFLAGS $($PYTHON - <<EOD
import sysconfig
print(' '.join(list(('-L' + sysconfig.get_config_var('LIBDIR'),
                     '-lpython' + sysconfig.get_config_var('LDVERSION'),
                     sysconfig.get_config_var('SHLIBS'),
                     sysconfig.get_config_var('SYSLIBS')))))
EOD
)"
        ;;
     *)
        AM_CONDITIONAL(CYGWIN, false)
        ;;
esac
AC_MSG_RESULT([$CYGWIN])

dnl use "make V=1" if you want to see the long awful lines
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl keep the correct libtool macros in-tree
AC_CONFIG_MACRO_DIR([m4])

dnl Process GiNaC version information
GINACLIB_MAJOR_VERSION=ginac_major_version
GINACLIB_MINOR_VERSION=ginac_minor_version
GINACLIB_MICRO_VERSION=ginac_micro_version
GINACLIB_VERSION=ginac_version
AC_SUBST(GINACLIB_MAJOR_VERSION)
AC_SUBST(GINACLIB_MINOR_VERSION)
AC_SUBST(GINACLIB_MICRO_VERSION)
AC_SUBST(GINACLIB_VERSION)

dnl GiNaC archive file version information.
dnl
dnl If properties have been added, ARCHIVE_VERSION += 1, ARCHIVE_AGE += 1.
dnl If backwards compatibility has been broken, set ARCHIVE_AGE to 0.
dnl
dnl The version number in newly created archives will be ARCHIVE_VERSION.
dnl Archives version (ARCHIVE_VERSION-ARCHIVE_AGE) thru ARCHIVE_VERSION can
dnl be read by this version of the GiNaC library.

ARCHIVE_VERSION=3
ARCHIVE_AGE=0

AC_SUBST(ARCHIVE_VERSION)
AC_SUBST(ARCHIVE_AGE)
AC_DEFINE_UNQUOTED(ARCHIVE_VERSION, $ARCHIVE_VERSION, [Current GiNaC archive file version number])
AC_DEFINE_UNQUOTED(ARCHIVE_AGE, $ARCHIVE_AGE, [GiNaC archive file version age])

dnl libtool versioning
LT_VERSION_INFO="lt_current:lt_revision:lt_age"
AC_SUBST(LT_VERSION_INFO)


dnl Check for the compiler and all the utilities needed for the build.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL
LT_INIT

dnl Switch to C++ language mode for the following libraries and headers.
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])

AC_CHECK_HEADERS([gmp.h], , AC_MSG_ERROR([This package needs gmp headers]))
AC_SEARCH_LIBS([__gmpz_get_str], [gmp], [], [AC_MSG_ERROR([This package needs libgmp])])

AC_CHECK_HEADERS([flint/fmpq_poly.h], , AC_MSG_ERROR([This package needs flint headers]))
AC_SEARCH_LIBS([fmpq_get_mpz_frac], [flint], [], [AC_MSG_ERROR([This package needs libflint])])

AC_ARG_WITH([giac],
  [AS_HELP_STRING([--with-giac@<:@=no|check|yes@:>@],
    [use giac for polynomial manipulations @<:@default=no@:>@ (experimental)])],
  [],
  [with_giac=no])

LIBGIAC=
AS_IF([test "x$with_giac" != xno],
  [AC_CHECK_LIB([giac], [ConvertUTF16toUTF8],
      [AC_SUBST([LIBGIAC], ["-lgiac"])
       AC_DEFINE([HAVE_LIBGIAC], [1],
                 [Define if you have libgiac])
      ],
      [if test "x$with_giac" != xcheck; then
         AC_MSG_FAILURE(
           [--with-giac was given, but test for giac failed])
       fi
      ], -lgmp)])

dnl Check for data types which are needed by the hash function 
dnl (golden_ratio_hash).
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(long double)
AC_CHECK_SIZEOF(void *)

dnl Make sure all the necessary standard headers are installed on the system.
AC_CHECK_HEADER(iosfwd, , GINAC_ERROR([The standard <iosfwd> header file could not be found.]))
AC_CHECK_HEADER(iostream, , GINAC_ERROR([The standard <iostream> header file could not be found.]))
AC_CHECK_HEADER(vector, , GINAC_ERROR([The standard <vector> header file could not be found.]))
AC_CHECK_HEADER(list, , GINAC_ERROR([The standard <list> header file could not be found.]))
AC_CHECK_HEADER(map, , GINAC_ERROR([The standard <map> header file could not be found.]))
AC_CHECK_HEADER(string, , GINAC_ERROR([The standard <string> header file could not be found.]))
AC_CHECK_HEADER(sstream, , GINAC_ERROR([The standard <sstream> header file could not be found.]))
AC_CHECK_HEADER(typeinfo, , GINAC_ERROR([The standard <typeinfo> header file could not be found.]))
AC_CHECK_HEADER(stdexcept, , GINAC_ERROR([The standard <stdexcept> header file could not be found.]))
AC_CHECK_HEADER(algorithm, , GINAC_ERROR([The standard <algorithm> header file could not be found.]))
AC_CHECK_HEADER(limits, , GINAC_ERROR([The standard <limits> header file could not be found.]))
if test "x$CONFIG_RUSAGE" = "xno"; then
    AC_CHECK_HEADER(ctime, , GINAC_ERROR([The standard <ctime> header file could not be found.]))
fi

PKG_PROG_PKG_CONFIG

dnl Find Singular's factory header and library with pkg-config
PKG_CHECK_MODULES([FACTORY], [factory], [
    dnl Found it
  ], [
    dnl Debian renames factory to singular-factory, look for that
    PKG_CHECK_MODULES([SINGULAR_FACTORY], [singular-factory], [
      FACTORY_CFLAGS="$SINGULAR_FACTORY_CFLAGS"
      FACTORY_LIBS="$SINGULAR_FACTORY_LIBS"
    ], [
      AC_MSG_ERROR([This package needs libfactory])
    ])
])
AC_SUBST(FACTORY_CFLAGS)
AC_SUBST(FACTORY_LIBS)

dnl Check for utilities needed by the different kinds of documentation.
dnl Documentation needs only be built when extending it, so never mind if it
dnl cannot find those helpers:
#AC_PATH_PROG(DOXYGEN, doxygen, "")
#AM_CONDITIONAL(CONFIG_DOXYGEN, [test ! -z "$DOXYGEN"])
#AC_PATH_PROG(LATEX, latex, "")
#AC_PATH_PROG(PDFLATEX, pdflatex, "")
#AC_PATH_PROG(MAKEINDEX, makeindex, "")
#AC_PATH_PROG(DVIPS, dvips, "")
#AM_CONDITIONAL(CONFIG_TEX, [test ! \( -z "$LATEX" -o -z $"PDFLATEX" -o -z "$MAKEINDEX" -o -z "$DVIPS" \)])
#AC_PATH_PROG(FIG2DEV, fig2dev, "")
#AM_CONDITIONAL(CONFIG_FIG2DEV, [test ! -z "$FIG2DEV"])

AC_PATH_PROG(RM, rm, $FALSE)
RM="$RM -f"


dnl Output makefiles etc.
AC_CONFIG_FILES([
Makefile
pynac.spec
pynac.pc
ginac/Makefile
ginac/version.h
])
AX_PREFIX_CONFIG_H(ginac/pynac-config.h)
AC_OUTPUT
dnl Display a final warning if there has been a GINAC_ERROR or a GINAC_WARNING
GINAC_CHECK_ERRORS
