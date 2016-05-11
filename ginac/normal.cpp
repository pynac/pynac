/** @file normal.cpp
 *
 *  This file implements several functions that work on univariate and
 *  multivariate polynomials and rational functions.
 *  These functions include polynomial quotient and remainder, GCD and LCM
 *  computation, square-free factorization and rational function normalization. */

/*
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

#include "normal.h"
#include "basic.h"
#include "ex.h"
#include "add.h"
#include "constant.h"
#include "expairseq.h"
#include "fail.h"
#include "inifcns.h"
#include "lst.h"
#include "mul.h"
#include "numeric.h"
#include "power.h"
#include "relational.h"
#include "operators.h"
#include "matrix.h"
#include "pseries.h"
#include "symbol.h"
#include "utils.h"
#include "mpoly.h"

#include <algorithm>
#include <map>

namespace GiNaC {

/** Return pointer to first symbol found in expression.  Due to GiNaC's
 *  internal ordering of terms, it may not be obvious which symbol this
 *  function returns for a given expression.
 *
 *  @param e  expression to search
 *  @param x  first symbol found (returned)
 *  @return "false" if no symbol was found, "true" otherwise */
bool get_first_symbol(const ex &e, ex &x)
{
	if (is_exactly_a<symbol>(e)) {
		x = e;
		return true;
	} else if (is_exactly_a<add>(e) || is_exactly_a<mul>(e)) {
		for (size_t i=0; i<e.nops(); i++)
			if (get_first_symbol(e.sorted_op(i), x))
				return true;
	} else if (is_exactly_a<power>(e)) {
		if (get_first_symbol(e.op(0), x))
			return true;
	}
	return false;
}


/** Collect common factors in sums. This converts expressions like
 *  'a*(b*x+b*y)' to 'a*b*(x+y)'. */
ex collect_common_factors(const ex & e)
{
	if (is_exactly_a<add>(e) || is_exactly_a<mul>(e) || is_exactly_a<power>(e)) {

		exmap repl;
		ex factor = 1;
		ex r = find_common_factor(e, factor, repl);
		return factor.subs(repl, subs_options::no_pattern) * r.subs(repl, subs_options::no_pattern);

	} else
		return e;
}


/** Compute the integer content (= GCD of all numeric coefficients) of an
 *  expanded polynomial. For a polynomial with rational coefficients, this
 *  returns g/l where g is the GCD of the coefficients' numerators and l
 *  is the LCM of the coefficients' denominators.
 *
 *  @return integer content */
numeric ex::integer_content() const
{
	return bp->integer_content();
}

numeric basic::integer_content() const
{
	return *_num1_p;
}

numeric numeric::integer_content() const
{
	return abs();
}

numeric add::integer_content() const
{
	auto it = seq.begin();
	auto itend = seq.end();
	numeric c = *_num0_p, l = *_num1_p;
	while (it != itend) {
		GINAC_ASSERT(!is_exactly_a<numeric>(it->rest));
		GINAC_ASSERT(is_exactly_a<numeric>(it->coeff));
		c = gcd(ex_to<numeric>(it->coeff).numer(), c);
		l = lcm(ex_to<numeric>(it->coeff).denom(), l);
		it++;
	}
	GINAC_ASSERT(is_exactly_a<numeric>(overall_coeff));
	c = gcd(ex_to<numeric>(overall_coeff).numer(), c);
	l = lcm(ex_to<numeric>(overall_coeff).denom(), l);
	return c/l;
}

numeric mul::integer_content() const
{
#ifdef DO_GINAC_ASSERT
	epvector::const_iterator it = seq.begin();
	epvector::const_iterator itend = seq.end();
	while (it != itend) {
		GINAC_ASSERT(!is_exactly_a<numeric>(recombine_pair_to_ex(*it)));
		++it;
	}
#endif // def DO_GINAC_ASSERT
	GINAC_ASSERT(is_exactly_a<numeric>(overall_coeff));
	return abs(ex_to<numeric>(overall_coeff));
}


/*
 *  Normal form of rational functions
 */

/*
 *  Note: The internal normal() functions (= basic::normal() and overloaded
 *  functions) all return lists of the form {numerator, denominator}. This
 *  is to get around mul::eval()'s automatic expansion of numeric coefficients.
 *  E.g. (a+b)/3 is automatically converted to a/3+b/3 but we want to keep
 *  the information that (a+b) is the numerator and 3 is the denominator.
 */


/** Create a symbol for replacing the expression "e" (or return a previously
 *  assigned symbol). The symbol and expression are appended to repl, for
 *  a later application of subs().
 *  @see ex::normal */
static ex replace_with_symbol(const ex & e, exmap & repl, exmap & rev_lookup)
{
	// Since the repl contains replaced expressions we should search for them
	ex e_replaced = e.subs(repl, subs_options::no_pattern);

	// Expression already replaced? Then return the assigned symbol
	auto it = rev_lookup.find(e_replaced);
	if (it != rev_lookup.end())
		return it->second;

	// Otherwise create new symbol and add to list, taking care that the
	// replacement expression doesn't itself contain symbols from repl,
	// because subs() is not recursive
	ex es = (new symbol)->setflag(status_flags::dynallocated);
	repl.insert(std::make_pair(es, e_replaced));
	rev_lookup.insert(std::make_pair(e_replaced, es));
	return es;
}

/** Create a symbol for replacing the expression "e" (or return a previously
 *  assigned symbol). The symbol and expression are appended to repl, and the
 *  symbol is returned.
 *  @see basic::to_rational
 *  @see basic::to_polynomial */
static ex replace_with_symbol(const ex & e, exmap & repl)
{
	// Since the repl contains replaced expressions we should search for them
	ex e_replaced = e.subs(repl, subs_options::no_pattern);

	// Expression already replaced? Then return the assigned symbol
	for (const auto& elem : repl)
		if (elem.second.is_equal(e_replaced))
			return elem.first;

	// Otherwise create new symbol and add to list, taking care that the
	// replacement expression doesn't itself contain symbols from repl,
	// because subs() is not recursive
	ex es = (new symbol)->setflag(status_flags::dynallocated);
	repl.insert(std::make_pair(es, e_replaced));
	return es;
}


/** Function object to be applied by basic::normal(). */
struct normal_map_function : public map_function {
	int level;
	normal_map_function(int l) : level(l) {}
	ex operator()(const ex & e) override { return normal(e, level); }
};

/** Default implementation of ex::normal(). It normalizes the children and
 *  replaces the object with a temporary symbol.
 *  @see ex::normal */
ex basic::normal(exmap & repl, exmap & rev_lookup, int level) const
{
	if (nops() == 0)
		return (new lst(replace_with_symbol(*this, repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
	else {
		if (level == 1)
			return (new lst(replace_with_symbol(*this, repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
		else if (level == -max_recursion_level)
			throw(std::runtime_error("max recursion level reached"));
		else {
			normal_map_function map_normal(level - 1);
			return (new lst(replace_with_symbol(map(map_normal), repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
		}
	}
}


/** Implementation of ex::normal() for symbols. This returns the unmodified symbol.
 *  @see ex::normal */
ex symbol::normal(exmap & repl, exmap & rev_lookup, int level) const
{
	return (new lst(*this, _ex1))->setflag(status_flags::dynallocated);
}


/** Implementation of ex::normal() for a numeric. It splits complex numbers
 *  into re+I*im and replaces I and non-rational real numbers with a temporary
 *  symbol.
 *  @see ex::normal */
ex numeric::normal(exmap & repl, exmap & rev_lookup, int level) const
{
	numeric num = numer();
	ex numex = num;

	if (num.is_real()) {
		if (!num.is_integer())
			numex = replace_with_symbol(numex, repl, rev_lookup);
	} else { // complex
		numeric re = num.real(), im = num.imag();
		ex re_ex = re.is_rational() ? re : replace_with_symbol(re, repl, rev_lookup);
		ex im_ex = im.is_rational() ? im : replace_with_symbol(im, repl, rev_lookup);
		numex = re_ex + im_ex * replace_with_symbol(I, repl, rev_lookup);
	}

	// Denominator is always a real integer (see numeric::denom())
	return (new lst(numex, denom()))->setflag(status_flags::dynallocated);
}


/** Fraction cancellation.
 *  @param n  numerator
 *  @param d  denominator
 *  @return cancelled fraction {n, d} as a list */
static ex frac_cancel(const ex &n, const ex &d)
{
	ex num = n;
	ex den = d;
	numeric pre_factor = *_num1_p;

//std::clog << "frac_cancel num = " << num << ", den = " << den << std::endl;

	// Handle trivial case where denominator is 1
	if (den.is_equal(_ex1))
		return (new lst(num, den))->setflag(status_flags::dynallocated);

	// Handle special cases where numerator or denominator is 0
	if (num.is_zero())
		return (new lst(num, _ex1))->setflag(status_flags::dynallocated);
	if (den.expand().is_zero())
		throw(std::overflow_error("frac_cancel: division by zero in frac_cancel"));

	// Bring numerator and denominator to Z[X] by multiplying with
	// LCM of all coefficients' denominators
	numeric num_lcm = lcm_of_coefficients_denominators(num);
	numeric den_lcm = lcm_of_coefficients_denominators(den);
	num = multiply_lcm(num, num_lcm);
	den = multiply_lcm(den, den_lcm);
	pre_factor = den_lcm / num_lcm;

	// Cancel GCD from numerator and denominator
	ex cnum, cden;
	if (gcd(num, den, &cnum, &cden, false) != _ex1) {
		num = cnum;
		den = cden;
	}

	// Make denominator unit normal (i.e. coefficient of first symbol
	// as defined by get_first_symbol() is made positive)
	if (is_exactly_a<numeric>(den)) {
		if (ex_to<numeric>(den).is_negative()) {
			num *= _ex_1;
			den *= _ex_1;
		}
	} else {
		ex x;
		if (get_first_symbol(den, x)) {
			GINAC_ASSERT(is_exactly_a<numeric>(den.unit(x)));
			if (ex_to<numeric>(den.unit(x)).is_negative()) {
				num *= _ex_1;
				den *= _ex_1;
			}
		}
	}

	// Return result as list
//std::clog << " returns num = " << num << ", den = " << den << ", pre_factor = " << pre_factor << std::endl;
	return (new lst(num * pre_factor.numer(), den * pre_factor.denom()))->setflag(status_flags::dynallocated);
}


/** Implementation of ex::normal() for a sum. It expands terms and performs
 *  fractional addition.
 *  @see ex::normal */
ex add::normal(exmap & repl, exmap & rev_lookup, int level) const
{
	if (level == 1)
		return (new lst(replace_with_symbol(*this, repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
	else if (level == -max_recursion_level)
		throw(std::runtime_error("max recursion level reached"));

	// Normalize children and split each one into numerator and denominator
	exvector nums, dens;
	nums.reserve(seq.size()+1);
	dens.reserve(seq.size()+1);
	auto it = seq.begin(), itend = seq.end();
	while (it != itend) {
		ex n = ex_to<basic>(recombine_pair_to_ex(*it)).normal(repl, rev_lookup, level-1);
		nums.push_back(n.op(0));
		dens.push_back(n.op(1));
		it++;
	}
	ex n = ex_to<numeric>(overall_coeff).normal(repl, rev_lookup, level-1);
	nums.push_back(n.op(0));
	dens.push_back(n.op(1));
	GINAC_ASSERT(nums.size() == dens.size());

	// Now, nums is a vector of all numerators and dens is a vector of
	// all denominators
//std::clog << "add::normal uses " << nums.size() << " summands:\n";

	// Add fractions sequentially
	auto num_it = nums.begin(), num_itend = nums.end();
	auto den_it = dens.begin(), den_itend = dens.end();
//std::clog << " num = " << *num_it << ", den = " << *den_it << std::endl;
	ex num = *num_it++, den = *den_it++;
	while (num_it != num_itend) {
//std::clog << " num = " << *num_it << ", den = " << *den_it << std::endl;
		ex next_num = *num_it++, next_den = *den_it++;

		// Trivially add sequences of fractions with identical denominators
		while ((den_it != den_itend) && next_den.is_equal(*den_it)) {
			next_num += *num_it;
			num_it++; den_it++;
		}

		// Additiion of two fractions, taking advantage of the fact that
		// the heuristic GCD algorithm computes the cofactors at no extra cost
		ex co_den1, co_den2;
		ex g = gcd(den, next_den, &co_den1, &co_den2, false);
		num = ((num * co_den2) + (next_num * co_den1)).expand();
		den *= co_den2;		// this is the lcm(den, next_den)
	}
//std::clog << " common denominator = " << den << std::endl;

	// Cancel common factors from num/den
	return frac_cancel(num, den);
}


/** Implementation of ex::normal() for a product. It cancels common factors
 *  from fractions.
 *  @see ex::normal() */
ex mul::normal(exmap & repl, exmap & rev_lookup, int level) const
{
	if (level == 1)
		return (new lst(replace_with_symbol(*this, repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
	else if (level == -max_recursion_level)
		throw(std::runtime_error("max recursion level reached"));

	// Normalize children, separate into numerator and denominator
	exvector num; num.reserve(seq.size());
	exvector den; den.reserve(seq.size());
	ex n;
	auto it = seq.begin(), itend = seq.end();
	while (it != itend) {
		n = ex_to<basic>(recombine_pair_to_ex(*it)).normal(repl, rev_lookup, level-1);
		num.push_back(n.op(0));
		den.push_back(n.op(1));
		it++;
	}
	n = ex_to<numeric>(overall_coeff).normal(repl, rev_lookup, level-1);
	num.push_back(n.op(0));
	den.push_back(n.op(1));

	// Perform fraction cancellation
	return frac_cancel((new mul(num))->setflag(status_flags::dynallocated),
	                   (new mul(den))->setflag(status_flags::dynallocated));
}


/** Implementation of ex::normal([B) for powers. It normalizes the basis,
 *  distributes integer exponents to numerator and denominator, and replaces
 *  non-integer powers by temporary symbols.
 *  @see ex::normal */
ex power::normal(exmap & repl, exmap & rev_lookup, int level) const
{
	if (level == 1)
		return (new lst(replace_with_symbol(*this, repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
	else if (level == -max_recursion_level)
		throw(std::runtime_error("max recursion level reached"));

	// Normalize basis and exponent (exponent gets reassembled)
	ex n_basis = ex_to<basic>(basis).normal(repl, rev_lookup, level-1);
	ex n_exponent = ex_to<basic>(exponent).normal(repl, rev_lookup, level-1);
	n_exponent = n_exponent.op(0) / n_exponent.op(1);

	if (n_exponent.info(info_flags::integer)) {

		if (n_exponent.info(info_flags::positive)) {

			// (a/b)^n -> {a^n, b^n}
			return (new lst(power(n_basis.op(0), n_exponent), power(n_basis.op(1), n_exponent)))->setflag(status_flags::dynallocated);

		} else if (n_exponent.info(info_flags::negative)) {

			// (a/b)^-n -> {b^n, a^n}
			return (new lst(power(n_basis.op(1), -n_exponent), power(n_basis.op(0), -n_exponent)))->setflag(status_flags::dynallocated);
		}

	} else {

		if (n_exponent.info(info_flags::positive)) {

			// (a/b)^x -> {sym((a/b)^x), 1}
			return (new lst(replace_with_symbol(power(n_basis.op(0) / n_basis.op(1), n_exponent), repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);

		} else if (n_exponent.info(info_flags::negative)) {

			if (n_basis.op(1).is_equal(_ex1)) {

				// a^-x -> {1, sym(a^x)}
				return (new lst(_ex1, replace_with_symbol(power(n_basis.op(0), -n_exponent), repl, rev_lookup)))->setflag(status_flags::dynallocated);

			} else {

				// (a/b)^-x -> {sym((b/a)^x), 1}
				return (new lst(replace_with_symbol(power(n_basis.op(1) / n_basis.op(0), -n_exponent), repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
			}
		}
	}

	// (a/b)^x -> {sym((a/b)^x, 1}
	return (new lst(replace_with_symbol(power(n_basis.op(0) / n_basis.op(1), n_exponent), repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
}


/** Implementation of ex::normal() for pseries. It normalizes each coefficient
 *  and replaces the series by a temporary symbol.
 *  @see ex::normal */
ex pseries::normal(exmap & repl, exmap & rev_lookup, int level) const
{
	epvector newseq;
	auto i = seq.begin(), end = seq.end();
	while (i != end) {
		ex restexp = i->rest.normal();
		if (!restexp.is_zero())
			newseq.push_back(expair(restexp, i->coeff));
		++i;
	}
	ex n = pseries(relational(var,point), newseq);
	return (new lst(replace_with_symbol(n, repl, rev_lookup), _ex1))->setflag(status_flags::dynallocated);
}


/** Normalization of rational functions.
 *  This function converts an expression to its normal form
 *  "numerator/denominator", where numerator and denominator are (relatively
 *  prime) polynomials. Any subexpressions which are not rational functions
 *  (like non-rational numbers, non-integer powers or functions like sin(),
 *  cos() etc.) are replaced by temporary symbols which are re-substituted by
 *  the (normalized) subexpressions before normal() returns (this way, any
 *  expression can be treated as a rational function). normal() is applied
 *  recursively to arguments of functions etc.
 *
 *  @param level maximum depth of recursion
 *  @return normalized expression */
ex ex::normal(int level) const
{
	exmap repl, rev_lookup;

	ex e = bp->normal(repl, rev_lookup, level);
	GINAC_ASSERT(is_a<lst>(e));

	// Re-insert replaced symbols
	if (!repl.empty())
		e = e.subs(repl, subs_options::no_pattern);

	// Convert {numerator, denominator} form back to fraction
	return e.op(0) / e.op(1);
}

/** Get numerator of an expression. If the expression is not of the normal
 *  form "numerator/denominator", it is first converted to this form and
 *  then the numerator is returned.
 *
 *  @see ex::normal
 *  @return numerator */
ex ex::numer() const
{
	exmap repl, rev_lookup;

	ex e = bp->normal(repl, rev_lookup, 0);
	GINAC_ASSERT(is_a<lst>(e));

	// Re-insert replaced symbols
	if (repl.empty())
		return e.op(0);
	else
		return e.op(0).subs(repl, subs_options::no_pattern);
}

/** Get denominator of an expression. If the expression is not of the normal
 *  form "numerator/denominator", it is first converted to this form and
 *  then the denominator is returned.
 *
 *  @see ex::normal
 *  @return denominator */
ex ex::denom() const
{
	exmap repl, rev_lookup;

	ex e = bp->normal(repl, rev_lookup, 0);
	GINAC_ASSERT(is_a<lst>(e));

	// Re-insert replaced symbols
	if (repl.empty())
		return e.op(1);
	else
		return e.op(1).subs(repl, subs_options::no_pattern);
}

/** Get numerator and denominator of an expression. If the expresison is not
 *  of the normal form "numerator/denominator", it is first converted to this
 *  form and then a list [numerator, denominator] is returned.
 *
 *  @see ex::normal
 *  @return a list [numerator, denominator] */
ex ex::numer_denom() const
{
	exmap repl, rev_lookup;

	ex e = bp->normal(repl, rev_lookup, 0);
	GINAC_ASSERT(is_a<lst>(e));

	// Re-insert replaced symbols
	if (repl.empty())
		return e;
	else
		return e.subs(repl, subs_options::no_pattern);
}


/** Rationalization of non-rational functions.
 *  This function converts a general expression to a rational function
 *  by replacing all non-rational subexpressions (like non-rational numbers,
 *  non-integer powers or functions like sin(), cos() etc.) to temporary
 *  symbols. This makes it possible to use functions like gcd() and divide()
 *  on non-rational functions by applying to_rational() on the arguments,
 *  calling the desired function and re-substituting the temporary symbols
 *  in the result. To make the last step possible, all temporary symbols and
 *  their associated expressions are collected in the map specified by the
 *  repl parameter, ready to be passed as an argument to ex::subs().
 *
 *  @param repl collects all temporary symbols and their replacements
 *  @return rationalized expression */
ex ex::to_rational(exmap & repl) const
{
	return bp->to_rational(repl);
}

// GiNaC 1.1 compatibility function
ex ex::to_rational(lst & repl_lst) const
{
	// Convert lst to exmap
	exmap m;
	for (const auto & elem : repl_lst)
		m.insert(std::make_pair(elem.op(0), elem.op(1)));

	ex ret = bp->to_rational(m);

	// Convert exmap back to lst
	repl_lst.remove_all();
	for (const auto& elem : m)
		repl_lst.append(elem.first == elem.second);

	return ret;
}

ex ex::to_polynomial(exmap & repl) const
{
	return bp->to_polynomial(repl);
}

// GiNaC 1.1 compatibility function
ex ex::to_polynomial(lst & repl_lst) const
{
	// Convert lst to exmap
	exmap m;
	for (const auto & elem : repl_lst)
		m.insert(std::make_pair(elem.op(0), elem.op(1)));

	ex ret = bp->to_polynomial(m);

	// Convert exmap back to lst
	repl_lst.remove_all();
	for (const auto& elem : m)
		repl_lst.append(elem.first == elem.second);

	return ret;
}

/** Default implementation of ex::to_rational(). This replaces the object with
 *  a temporary symbol. */
ex basic::to_rational(exmap & repl) const
{
	return replace_with_symbol(*this, repl);
}

ex basic::to_polynomial(exmap & repl) const
{
	return replace_with_symbol(*this, repl);
}


/** Implementation of ex::to_rational() for symbols. This returns the
 *  unmodified symbol. */
ex symbol::to_rational(exmap & repl) const
{
	return *this;
}

/** Implementation of ex::to_polynomial() for symbols. This returns the
 *  unmodified symbol. */
ex symbol::to_polynomial(exmap & repl) const
{
	return *this;
}


/** Implementation of ex::to_rational() for a numeric. It splits complex
 *  numbers into re+I*im and replaces I and non-rational real numbers with a
 *  temporary symbol. */
ex numeric::to_rational(exmap & repl) const
{
	if (is_real()) {
		if (!is_rational())
			return replace_with_symbol(*this, repl);
	} else { // complex
		numeric re = real();
		numeric im = imag();
		ex re_ex = re.is_rational() ? re : replace_with_symbol(re, repl);
		ex im_ex = im.is_rational() ? im : replace_with_symbol(im, repl);
		return re_ex + im_ex * replace_with_symbol(I, repl);
	}
	return *this;
}

/** Implementation of ex::to_polynomial() for a numeric. It splits complex
 *  numbers into re+I*im and replaces I and non-integer real numbers with a
 *  temporary symbol. */
ex numeric::to_polynomial(exmap & repl) const
{
	if (is_real()) {
		if (!is_integer())
			return replace_with_symbol(*this, repl);
	} else { // complex
		numeric re = real();
		numeric im = imag();
		ex re_ex = re.is_integer() ? re : replace_with_symbol(re, repl);
		ex im_ex = im.is_integer() ? im : replace_with_symbol(im, repl);
		return re_ex + im_ex * replace_with_symbol(I, repl);
	}
	return *this;
}


/** Implementation of ex::to_rational() for powers. It replaces non-integer
 *  powers by temporary symbols. */
ex power::to_rational(exmap & repl) const
{
	if (exponent.info(info_flags::integer))
		return power(basis.to_rational(repl), exponent);
	else
		return replace_with_symbol(*this, repl);
}

/** Implementation of ex::to_polynomial() for powers. It replaces non-posint
 *  powers by temporary symbols. */
ex power::to_polynomial(exmap & repl) const
{
	if (exponent.info(info_flags::posint))
		return power(basis.to_rational(repl), exponent);
	else if (exponent.info(info_flags::negint))
	{
		ex basis_pref = collect_common_factors(basis);
		if (is_exactly_a<mul>(basis_pref) || is_exactly_a<power>(basis_pref)) {
			// (A*B)^n will be automagically transformed to A^n*B^n
			ex t = power(basis_pref, exponent);
			return t.to_polynomial(repl);
		}
		else
			return power(replace_with_symbol(power(basis, _ex_1), repl), -exponent);
	} 
	else
		return replace_with_symbol(*this, repl);
}


/** Implementation of ex::to_rational() for expairseqs. */
ex expairseq::to_rational(exmap & repl) const
{
	epvector s;
	s.reserve(seq.size());
	auto i = seq.begin(), end = seq.end();
	while (i != end) {
		s.push_back(split_ex_to_pair(recombine_pair_to_ex(*i).to_rational(repl)));
		++i;
	}
	ex oc = overall_coeff.to_rational(repl);
	if (oc.info(info_flags::numeric))
		return thisexpairseq(s, overall_coeff);
	else
		s.push_back(combine_ex_with_coeff_to_pair(oc, _ex1));
	return thisexpairseq(s, default_overall_coeff());
}

/** Implementation of ex::to_polynomial() for expairseqs. */
ex expairseq::to_polynomial(exmap & repl) const
{
	epvector s;
	s.reserve(seq.size());
	auto i = seq.begin(), end = seq.end();
	while (i != end) {
		s.push_back(split_ex_to_pair(recombine_pair_to_ex(*i).to_polynomial(repl)));
		++i;
	}
	ex oc = overall_coeff.to_polynomial(repl);
	if (oc.info(info_flags::numeric))
		return thisexpairseq(s, overall_coeff);
	else
		s.push_back(combine_ex_with_coeff_to_pair(oc, _ex1));
	return thisexpairseq(s, default_overall_coeff());
}


} // namespace GiNaC
