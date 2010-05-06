AC_DEFUN([AM_CHECK_PYTHON_LIB],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_REQUIRE([AM_CHECK_PYTHON_HEADERS])

AC_MSG_CHECKING(for libpython${PYTHON_VERSION})

py_config_dir=${py_exec_prefix}/lib/python${PYTHON_VERSION}/config
py_makefile="${py_config_dir}/Makefile"
if test -f "$py_makefile"; then
dnl extra required libs
  py_localmodlibs=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $py_makefile`
  py_basemodlibs=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $py_makefile`
  py_other_libs=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $py_makefile`

dnl now the actual libpython
  if test -e "${py_config_dir}/libpython${PYTHON_VERSION}.a"; then
    PYTHON_LIBS="-L${py_config_dir} -lpython${PYTHON_VERSION} $py_localmodlibs $py_basemodlibs $py_other_libs"
    AC_MSG_RESULT(found)
  elif test -e "${py_config_dir}/libpython${PYTHON_VERSION}.dll.a"; then
    PYTHON_LIBS="-L${py_config_dir} -lpython${PYTHON_VERSION} $py_localmodlibs $py_basemodlibs $py_other_libs"
    AC_MSG_RESULT(found)
  else
    if test -e "/usr/lib/libpython${PYTHON_VERSION}.dylib"; then
      PYTHON_LIBS="-lpython${PYTHON_VERSION} $py_localmodlibs $py_basemodlibs $py_other_libs"
      AC_MSG_RESULT(found)
    else
     AC_MSG_RESULT(not found)
    fi
  fi
fi
AC_SUBST(PYTHON_LIBS)])

