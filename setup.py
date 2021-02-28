from setuptools import setup
from setuptools.extension import Extension
from distutils.command.build_ext import build_ext as du_build_ext
import os
import sys

class build_ext(du_build_ext):
    def run(self):
        from Cython.Build.Dependencies import cythonize
        self.distribution.ext_modules[:] = cythonize(
            self.distribution.ext_modules,
            language_level=3,
            include_path=['.'] + sys.path)
        du_build_ext.run(self)

extensions = [
    Extension('ginac.libpynac', sources=(
        ['ginac/pynac.pyx']
        # From ginac/Makefile.am, removed .h files
        + [os.path.join('ginac/', f)
           for f in "\
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
  order.cpp useries.cpp".split()]
        ),
        # From configure.ac
        define_macros=[('PYNAC_ARCHIVE_VERSION', '3'),
                       ('PYNAC_ARCHIVE_AGE', '0')],
)]

setup(ext_modules=extensions,
      include_dirs = ['.'] + sys.path,
      package_dir = {'ginac': 'ginac'},
      package_data = {'ginac': ['*.pxd', '*.h']},
      cmdclass = {'build_ext': build_ext},
      )
