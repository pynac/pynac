/** @file numeric.h
 *
 *  Makes the interface to the underlying bignum package available. */

/*
 *  This is a modified version of code included with Ginac.  
 *  The modifications and modified version is:
 * 
 *      GiNaC-SAGE Copyright (C) 2008 William Stein
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


/*  The original copyright:
 *  GiNaC Copyright (C) 1999-2008 Johannes Gutenberg University Mainz, Germany
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GINAC_NUMERIC_H__
#define __GINAC_NUMERIC_H__

#include "basic.h"
#include "constant.h"
#include "ex.h"
#include "py_funcs.h"

#include <stdexcept>
#include <vector>
#include <iostream>

void ginac_pyinit_Integer(PyObject*);
void ginac_pyinit_Float(PyObject*);
void ginac_pyinit_I(PyObject*);

namespace GiNaC {
  enum Type {
    LONG,
    DOUBLE,
    PYOBJECT,
    MPFR,
    MPZ,
    MPQ,
    MPFC,
    MPQC
  };

  union Value {
    double _double;
    long int _long;
    PyObject* _pyobject;
  };

  class Number_T {  // abstract base class
  protected:
    Type t;
    Value v;

  public:
    Number_T();
    Number_T(const int& x);
    Number_T(const long int& x);
    Number_T(const unsigned int& x);
    Number_T(const unsigned long& x);
    Number_T(const double& x);
    Number_T(const Number_T& x);
    Number_T(const char* s);
    Number_T(PyObject* o);  // *STEALS a REFERENCE*
    // unarchive
    Number_T(const archive_node &n, lst &sym_lst);

    // Destructor
    ~Number_T();

    // archive
    void archive(archive_node &n) const;
    
    Number_T operator+(Number_T x) const;
    Number_T operator*(Number_T x) const;
    Number_T operator/(Number_T x) const;
    Number_T operator-(Number_T x) const;
    Number_T& operator=(const Number_T& x);

    Number_T operator()(const int& x);
    Number_T operator-();

    operator double() const;
    operator int() const;
    operator long int() const;

    unsigned hash() const;
    int compare_same_type(const Number_T& right) const;
    bool operator==(const Number_T& right) const;
    bool operator!=(const Number_T& right) const;
    bool operator<=(const Number_T& right) const;
    bool operator>=(const Number_T& right) const;
    bool operator<(const Number_T& right) const;
    bool operator>(const Number_T& right) const;
    int csgn() const;
    int get_parent_char() const;

    bool is_zero() const;
    bool is_positive() const;
    bool is_negative() const;
    bool is_integer() const;
    bool is_cinteger() const;
    bool is_pos_integer() const;
    bool is_nonneg_integer() const;
    bool is_even() const;
    bool is_odd() const;
    bool is_prime() const;
    bool is_rational() const;
    bool is_crational() const;
    bool is_real() const;

    Number_T numer() const;
    Number_T denom() const;
    Number_T lcm(Number_T b) const;
    Number_T gcd(Number_T b) const;

    Number_T conjugate() const;

    Number_T sin() const;
    Number_T cos() const;
    Number_T exp() const;
    Number_T log() const;
    Number_T tan() const;
    Number_T asin() const;
    Number_T acos() const;
    Number_T atan() const;
    Number_T atan(const Number_T& y) const;
    Number_T sinh() const;
    Number_T cosh() const;
    Number_T tanh() const;
    Number_T asinh() const;
    Number_T acosh() const;
    Number_T atanh() const;
    Number_T Li(const Number_T &n, PyObject* parent) const;
    Number_T Li2() const;
    Number_T zeta() const;
    Number_T lgamma() const;
    Number_T tgamma() const;
    Number_T psi() const;
    Number_T psi(const Number_T& y) const;
    Number_T factorial() const;
    Number_T doublefactorial() const;
    Number_T fibonacci() const;
    Number_T evalf(PyObject* parent) const;
    Number_T step() const;
    Number_T isqrt() const;
    Number_T sqrt() const;
    Number_T abs() const;
    Number_T mod(const Number_T &b) const;
    Number_T smod(const Number_T &b) const;
    Number_T irem(const Number_T &b) const;
    Number_T irem(const Number_T &b, Number_T& q) const;
    Number_T iquo(const Number_T &b) const;
    Number_T iquo(const Number_T &b, Number_T& q) const;

    int int_length() const;
    
    friend std::ostream& operator << (std::ostream& os, const Number_T& s);
    friend Number_T pow(const Number_T& base, const Number_T& exp);
    friend void coerce(Number_T& new_left, Number_T& new_right, const Number_T& left, const Number_T& right);
    friend PyObject* Number_T_to_pyobject(const Number_T& x);
  };

  std::ostream& operator << (std::ostream& os, const Number_T& s);
  Number_T pow(const Number_T& base, const Number_T& exp);

  /** Exception class thrown when a singularity is encountered. */
  class pole_error : public std::domain_error {
  public:
    explicit pole_error(const std::string& what_arg, int degree);
    int degree() const;
  private:
    int deg;
  };


  /** This class is a wrapper around CLN-numbers within the GiNaC class
   *  hierarchy. Objects of this type may directly be created by the user.*/
  class numeric : public basic
  {
    GINAC_DECLARE_REGISTERED_CLASS(numeric, basic)
    // member functions
	
    // other constructors
    public:
    numeric(int i);
    numeric(unsigned int i);
    numeric(long i);
    numeric(unsigned long i);
    numeric(long numer, long denom);
    numeric(double d);
    numeric(const char *);
    numeric(const Number_T& x);
    numeric(PyObject*);
    numeric(archive_node& n, const lst &sym_lst);
	
    // functions overriding virtual functions from base classes
  public:
    unsigned precedence() const {return 30;}
    bool info(unsigned inf) const;
    bool is_polynomial(const ex & var) const;
    int degree(const ex & s) const;
    int ldegree(const ex & s) const;
    ex coeff(const ex & s, int n = 1) const;
    bool has(const ex &other, unsigned options = 0) const;
    ex eval(int level = 0) const;
    ex evalf(int level = 0, PyObject* parent=NULL) const;
    ex subs(const exmap & m, unsigned options = 0) const { return subs_one_level(m, options); } // overwrites basic::subs() for performance reasons
    ex normal(exmap & repl, exmap & rev_lookup, int level = 0) const;
    ex to_rational(exmap & repl) const;
    ex to_polynomial(exmap & repl) const;
    numeric integer_content() const;
    ex smod(const numeric &xi) const;
    numeric max_coefficient() const;
    ex conjugate() const;
    ex real_part() const;
    ex imag_part() const;
  protected:
    /** Implementation of ex::diff for a numeric always returns 0.
     *  @see ex::diff */
    ex derivative(const symbol &s) const { return 0; }
    bool is_equal_same_type(const basic &other) const;
    unsigned calchash() const;
	
    // new virtual functions which can be overridden by derived classes
    // (none)
	
    // non-virtual functions in this class
  public:
    const numeric add(const numeric &other) const;
    const numeric sub(const numeric &other) const;
    const numeric mul(const numeric &other) const;
    const numeric div(const numeric &other) const;
    const numeric power(const numeric &other) const;
    const numeric & add_dyn(const numeric &other) const;
    const numeric & sub_dyn(const numeric &other) const;
    const numeric & mul_dyn(const numeric &other) const;
    const numeric & div_dyn(const numeric &other) const;
    const numeric & power_dyn(const numeric &other) const;
    const numeric & operator=(int i);
    const numeric & operator=(unsigned int i);
    const numeric & operator=(long i);
    const numeric & operator=(unsigned long i);
    const numeric & operator=(double d);
    const numeric & operator=(const char *s);
    const numeric inverse() const;
    numeric step() const;
    int csgn() const;
    int compare(const numeric &other) const;
    bool is_equal(const numeric &other) const;
    bool is_zero() const;
    bool is_positive() const;
    bool is_negative() const;
    bool is_integer() const;
    bool is_pos_integer() const;
    bool is_nonneg_integer() const;
    bool is_even() const;
    bool is_odd() const;
    bool is_prime() const;
    bool is_rational() const;
    bool is_real() const;
    bool is_cinteger() const;
    bool is_crational() const;
    bool operator==(const numeric &other) const;
    bool operator!=(const numeric &other) const;
    bool operator<(const numeric &other) const;
    bool operator<=(const numeric &other) const;
    bool operator>(const numeric &other) const;
    bool operator>=(const numeric &other) const;
    bool is_parent_pos_char() const;
    int get_parent_char() const;
    int to_int() const;
    long to_long() const;
    double to_double() const;
    PyObject* to_pyobject() const;
    const numeric real() const;
    const numeric imag() const;
    const numeric numer() const;
    const numeric denom() const;
    int int_length() const;
    //numeric(const Number_T &z);

  protected:
    void print_numeric(const print_context & c, const char *par_open,
		    const char *par_close, const char *imag_sym,
		    const char *mul_sym, unsigned level, bool latex) const;
    void do_print(const print_context & c, unsigned level) const;
    void do_print_latex(const print_latex & c, unsigned level) const;
    void do_print_csrc(const print_csrc & c, unsigned level) const;
    void do_print_csrc_cl_N(const print_csrc_cl_N & c, unsigned level) const;
    void do_print_tree(const print_tree & c, unsigned level) const;
    void do_print_python_repr(const print_python_repr & c, unsigned level) const;

    // member variables

    //protected:
  public:
    Number_T value;
  };


  // global constants

  extern numeric I;

  // global functions

  const numeric exp(const numeric &x);
  const numeric log(const numeric &x);
  const numeric sin(const numeric &x);
  const numeric cos(const numeric &x);
  const numeric tan(const numeric &x);
  const numeric asin(const numeric &x);
  const numeric acos(const numeric &x);
  const numeric atan(const numeric &x);
  const numeric atan(const numeric &y, const numeric &x);
  const numeric sinh(const numeric &x);
  const numeric cosh(const numeric &x);
  const numeric tanh(const numeric &x);
  const numeric asinh(const numeric &x);
  const numeric acosh(const numeric &x);
  const numeric atanh(const numeric &x);
  const numeric Li2(const numeric &x);
  const numeric zeta(const numeric &x);
  const numeric lgamma(const numeric &x);
  const numeric tgamma(const numeric &x);
  const numeric psi(const numeric &x);
  const numeric psi(const numeric &n, const numeric &x);
  const numeric factorial(const numeric &n);
  const numeric doublefactorial(const numeric &n);
  const numeric binomial(const numeric &n, const numeric &k);
  const numeric bernoulli(const numeric &n);
  const numeric fibonacci(const numeric &n);
  const numeric isqrt(const numeric &x);
  const numeric sqrt(const numeric &x);
  const numeric abs(const numeric &x);
  const numeric mod(const numeric &a, const numeric &b);
  const numeric smod(const numeric &a, const numeric &b);
  const numeric irem(const numeric &a, const numeric &b);
  const numeric iquo(const numeric &a, const numeric &b);
  const numeric iquo(const numeric &a, const numeric &b, numeric &r);
  const numeric gcd(const numeric &a, const numeric &b);
  const numeric lcm(const numeric &a, const numeric &b);

  // wrapper functions around member functions
  inline const numeric pow(const numeric &x, const numeric &y)
  { return x.power(y); }

  inline const numeric inverse(const numeric &x)
  { return x.inverse(); }

  inline numeric step(const numeric &x)
  { return x.step(); }

  inline int csgn(const numeric &x)
  { return x.csgn(); }

  inline bool is_zero(const numeric &x)
  { return x.is_zero(); }

  inline bool is_positive(const numeric &x)
  { return x.is_positive(); }

  inline bool is_negative(const numeric &x)
  { return x.is_negative(); }

  inline bool is_integer(const numeric &x)
  { return x.is_integer(); }

  inline bool is_pos_integer(const numeric &x)
  { return x.is_pos_integer(); }

  inline bool is_nonneg_integer(const numeric &x)
  { return x.is_nonneg_integer(); }

  inline bool is_even(const numeric &x)
  { return x.is_even(); }

  inline bool is_odd(const numeric &x)
  { return x.is_odd(); }

  inline bool is_prime(const numeric &x)
  { return x.is_prime(); }

  inline bool is_rational(const numeric &x)
  { return x.is_rational(); }

  inline bool is_real(const numeric &x)
  { return x.is_real(); }

  inline bool is_cinteger(const numeric &x)
  { return x.is_cinteger(); }

  inline bool is_crational(const numeric &x)
  { return x.is_crational(); }

  inline int to_int(const numeric &x)
  { return x.to_int(); }

  inline long to_long(const numeric &x)
  { return x.to_long(); }

  inline double to_double(const numeric &x)
  { return x.to_double(); }

  inline const numeric real(const numeric &x)
  { return x.real(); }

  inline const numeric imag(const numeric &x)
  { return x.imag(); }

  inline const numeric numer(const numeric &x)
  { return x.numer(); }

  inline const numeric denom(const numeric &x)
  { return x.denom(); }

  // numeric evaluation functions for class constant objects:

  ex ConstantEvalf(unsigned serial, PyObject* parent=NULL);
  ex UnsignedInfinityEvalf(unsigned serial, PyObject* parent=NULL);
  ex InfinityEvalf(unsigned serial, PyObject* parent=NULL);
  ex NegInfinityEvalf(unsigned serial, PyObject* parent=NULL);

} // namespace GiNaC

#endif // ndef __GINAC_NUMERIC_H__
