/** @file py_func.cpp
 *
 *  Function table containing the functions used in the Sage - Pynac
 *  interface.
 *  */

#ifndef   	PY_FUNC_H_
# define   	PY_FUNC_H_

#include "Python.h"
#include "basic.h"
#include "constant.h"
#include "ex.h"

#include <stdexcept>
#include <vector>
#include <iostream>

namespace GiNaC {
  typedef std::multiset<unsigned> paramset;

  struct py_funcs_struct {
    PyObject* (*py_binomial)(PyObject* a, PyObject* b);
    PyObject* (*py_binomial_int)(int n, unsigned int k);
	PyObject* (*py_gcd)(PyObject* a, PyObject* b);
	PyObject* (*py_lcm)(PyObject* a, PyObject* b);
	PyObject* (*py_real)(PyObject* a);
	PyObject* (*py_imag)(PyObject* a);
	PyObject* (*py_numer)(PyObject* a);
	PyObject* (*py_denom)(PyObject* a);
	PyObject* (*py_conjugate)(PyObject* a);
    int       (*py_is_rational)(PyObject* a);
    int       (*py_is_crational)(PyObject* a);
    int       (*py_is_real)(PyObject* a);
    int       (*py_is_integer)(PyObject* a);
    int       (*py_is_equal)(PyObject* a, PyObject* b);
    int       (*py_is_even)(PyObject* a);
    int       (*py_is_cinteger)(PyObject* a);
    int       (*py_is_prime)(PyObject* n);

	PyObject* (*py_int)(PyObject* n);
	PyObject* (*py_integer_from_long)(long int x);
	PyObject* (*py_integer_from_python_obj)(PyObject* x);

	PyObject* (*py_float)(PyObject* a, PyObject* parent);
	PyObject* (*py_RDF_from_double)(double x);

	PyObject* (*py_factorial)(PyObject* a);
	PyObject* (*py_fibonacci)(PyObject* n);
	PyObject* (*py_step)(PyObject* n);
	PyObject* (*py_doublefactorial)(PyObject* a);
	PyObject* (*py_bernoulli)(PyObject* n);
	PyObject* (*py_sin)(PyObject* n);
	PyObject* (*py_cos)(PyObject* n);
	PyObject* (*py_zeta)(PyObject* n);
	PyObject* (*py_exp)(PyObject* n);
	PyObject* (*py_log)(PyObject* n);
	PyObject* (*py_tan)(PyObject* n);
	PyObject* (*py_asin)(PyObject* n);
	PyObject* (*py_acos)(PyObject* n);
	PyObject* (*py_atan)(PyObject* n);
	PyObject* (*py_atan2)(PyObject* n, PyObject* y);
	PyObject* (*py_sinh)(PyObject* n);
	PyObject* (*py_cosh)(PyObject* n);
	PyObject* (*py_tanh)(PyObject* n);
	PyObject* (*py_asinh)(PyObject* n);
	PyObject* (*py_acosh)(PyObject* n);
	PyObject* (*py_atanh)(PyObject* n);
    PyObject* (*py_li)(PyObject* x, PyObject* n, PyObject* prec);
	PyObject* (*py_li2)(PyObject* n);
	PyObject* (*py_lgamma)(PyObject* n);
	PyObject* (*py_tgamma)(PyObject* n);
	PyObject* (*py_psi)(PyObject* n);
	PyObject* (*py_psi2)(PyObject* n, PyObject* b);
	PyObject* (*py_isqrt)(PyObject* n);
	PyObject* (*py_sqrt)(PyObject* n);
	PyObject* (*py_abs)(PyObject* n);
	PyObject* (*py_mod)(PyObject* n, PyObject* b);
	PyObject* (*py_smod)(PyObject* n, PyObject* b);
	PyObject* (*py_irem)(PyObject* n, PyObject* b);
	PyObject* (*py_iquo)(PyObject* n, PyObject* b);
	PyObject* (*py_iquo2)(PyObject* n, PyObject* b);
	int       (*py_int_length)(PyObject* x);

	PyObject* (*py_eval_constant)(unsigned serial, PyObject* parent);
	PyObject* (*py_eval_unsigned_infinity)();
	PyObject* (*py_eval_infinity)();
	PyObject* (*py_eval_neg_infinity)();

	// we use this to check if the element lives in a domain of positive
	// characteristic, in which case we have to do modulo reductions
	int (*py_get_parent_char)(PyObject* o);

	// printing helpers
	std::string* (*py_latex)(PyObject* o, int level);
	std::string* (*py_repr)(PyObject* o, int level);

	// archive helper
	std::string* (*py_dumps)(PyObject* o);
	PyObject* (*py_loads)(PyObject* s);

    PyObject* (*exvector_to_PyTuple)(GiNaC::exvector seq);
    GiNaC::ex (*pyExpression_to_ex)(PyObject* s);
    PyObject* (*ex_to_pyExpression)(GiNaC::ex e);
    std::string* (*py_print_function)(unsigned id, PyObject* args);
    std::string* (*py_latex_function)(unsigned id, PyObject* args);
    PyObject* (*subs_args_to_PyTuple)(const GiNaC::exmap & m, unsigned options, const GiNaC::exvector & seq);
    int (*py_get_ginac_serial)();
    PyObject* (*py_get_sfunction_from_serial)(unsigned id);
    unsigned (*py_get_serial_from_sfunction)(PyObject* f);
    unsigned (*py_get_serial_for_new_sfunction)(std::string &s, unsigned nargs);

    constant (*py_get_constant)(const char* name);

	std::string* (*py_print_fderivative)(unsigned id, PyObject* params, PyObject* args);
	std::string* (*py_latex_fderivative)(unsigned id, PyObject* params, PyObject* args);
	PyObject* (*paramset_to_PyTuple)(const GiNaC::paramset &s);

    PyObject* (*py_rational_power_parts)(PyObject* basis, PyObject* exp);
  }; 

  extern py_funcs_struct py_funcs;

}


#endif 	    /* !PY_FUNC_H_ */
