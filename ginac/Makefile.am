## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libpynac.la
libpynac_la_SOURCES = add.cpp archive.cpp assume.cpp basic.cpp clifford.cpp \
  constant.cpp ex.cpp expair.cpp expairseq.cpp exprseq.cpp \
  fail.cpp fderivative.cpp function.cpp idx.cpp indexed.cpp infinity.cpp \
  inifcns.cpp inifcns_trig.cpp inifcns_zeta.cpp inifcns_hyperb.cpp \
  inifcns_trans.cpp inifcns_gamma.cpp inifcns_nstdsums.cpp \
  inifcns_orthopoly.cpp \
  integral.cpp lst.cpp matrix.cpp mul.cpp ncmul.cpp normal.cpp numeric.cpp \
  operators.cpp power.cpp py_funcs.cpp registrar.cpp relational.cpp remember.cpp \
  pseries.cpp print.cpp symbol.cpp symmetry.cpp tensor.cpp upoly-ginac.cpp \
  utils.cpp wildcard.cpp templates.cpp infoflagbase.cpp \
  remember.h tostring.h utils.h compiler.h order.cpp

#The -no-undefined breaks Pynac on OS X 10.4.  See #9135
if CYGWIN
libpynac_la_LDFLAGS = -version-info $(LT_VERSION_INFO) -no-undefined
else
libpynac_la_LDFLAGS = -version-info $(LT_VERSION_INFO)
endif
libpynac_la_CPPFLAGS = $(PYTHON_CPPFLAGS) -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wno-unused-parameter

libpynac_la_LIBADD = $(PYTHON_LDFLAGS) $(LIBS)
ginacincludedir = $(includedir)/pynac
ginacinclude_HEADERS = ginac.h py_funcs.h add.h archive.h assertion.h basic.h class_info.h \
  clifford.h constant.h infinity.h container.h ex.h expair.h expairseq.h \
  exprseq.h fail.h fderivative.h flags.h function.h idx.h indexed.h \
  inifcns.h integral.h lst.h matrix.h mul.h ncmul.h normal.h numeric.h operators.h \
  power.h print.h pseries.h ptr.h registrar.h relational.h extern_templates.h \
  symbol.h symmetry.h tensor.h version.h wildcard.h order.h templates.h \
  infoflagbase.h assume.h upoly.h

EXTRA_DIST = version.h.in
