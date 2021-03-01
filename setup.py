from setuptools import setup
from setuptools.extension import Extension
from setuptools.command.build_py import build_py as setuptools_build_py
from distutils.command.build_ext import build_ext as du_build_ext
import os
import sys

class build_py(setuptools_build_py):
    def run(self):
        if os.system("./configure") != 0:
            raise SystemExit("configure failed, see config.log")
        self.distribution.package_data['ginac'].append('pynac.pxd')
        setuptools_build_py.run(self)

class build_ext(du_build_ext):
    def run(self):
        from Cython.Build.Dependencies import cythonize
        self.distribution.ext_modules[:] = cythonize(
            self.distribution.ext_modules,
            language_level=3)
        du_build_ext.run(self)

# From ginac/Makefile.am
libpynac_la_SOURCES = "\
  add.cpp archive.cpp assume.cpp basic.cpp \
  cmatcher.cpp constant.cpp context.cpp ex.cpp expair.cpp \
  expairseq.cpp exprseq.cpp fderivative.cpp function.cpp function_info.cpp \
  infinity.cpp inifcns.cpp inifcns_trig.cpp inifcns_zeta.cpp \
  inifcns_hyperb.cpp inifcns_trans.cpp inifcns_gamma.cpp inifcns_nstdsums.cpp \
  inifcns_orthopoly.cpp inifcns_hyperg.cpp inifcns_comb.cpp \
  lst.cpp matrix.cpp mpoly-giac.cpp mpoly-ginac.cpp \
  mpoly-singular.cpp mpoly.cpp mul.cpp normal.cpp numeric.cpp \
  operators.cpp power.cpp py_funcs.cpp \
  registrar.cpp relational.cpp remember.cpp \
  pseries.cpp print.cpp symbol.cpp upoly-ginac.cpp \
  utils.cpp wildcard.cpp templates.cpp infoflagbase.cpp sum.cpp \
  remember.h tostring.h utils.h compiler.h order.cpp useries.cpp"

# From ginac/Makefile.am
ginacinclude_HEADERS = "\
  ginac.h py_funcs.h add.h archive.h assertion.h \
  basic.h class_info.h cmatcher.h constant.h container.h context.h \
  ex.h ex_utils.h expair.h expairseq.h exprseq.h \
  fderivative.h flags.h function.h \
  inifcns.h infinity.h lst.h matrix.h mpoly.h mul.h \
  normal.h numeric.h operators.h optional.hpp \
  power.h print.h pseries.h ptr.h registrar.h relational.h extern_templates.h \
  symbol.h version.h wildcard.h order.h templates.h \
  infoflagbase.h assume.h upoly.h useries.h useries-flint.h sum.h"

extensions = [
    Extension('ginac.pynac',
              sources=(['ginac/pynac.pyx']
                       + [os.path.join('ginac/', f)
                          for f in libpynac_la_SOURCES.split()
                          # strangely this contains some .h files
                          if not f.endswith(".h")]),
              depends=([os.path.join('ginac/', f)
                        for f in libpynac_la_SOURCES.split()
                          if f.endswith(".h")]
                       + [os.path.join('ginac/', f)
                          for f in ginacinclude_HEADERS.split()]),
              include_dirs=[os.path.dirname(__file__) or "."]
)]

setup(ext_modules=extensions,
      include_dirs = [os.path.dirname(__file__) or "."] + sys.path,
      packages = ['ginac'],
      package_dir = {'ginac': 'ginac'},
      package_data = {'ginac': ['*.pxd', '*.h', '*.hpp']},
      cmdclass = {'build_py': build_py,
                  'build_ext': build_ext},
      )
