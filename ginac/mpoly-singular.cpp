/** @file mpoly-singular.cpp
 *
 *  GiNaC Copyright (C) 1999-2008 Johannes Gutenberg University Mainz, Germany
 *  Copyright (C) 2016  Ralf Stephan
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

#ifdef HAVE_CONFIG_H
#include "pynac-config.h"
#endif

#include <factory/factory.h>

#include "upoly.h"
#include "normal.h"
#include "basic.h"
#include "ex.h"
#include "add.h"
#include "constant.h"
#include "expairseq.h"
#include "mul.h"
#include "numeric.h"
#include "power.h"
#include "operators.h"
#include "pseries.h"
#include "relational.h"
#include "symbol.h"
#include "inifcns.h"
#include "function.h"
#include "utils.h"
#include "fail.h"

namespace GiNaC {


static CanonicalForm num2canonical(const numeric& n)
{
        if (n.is_mpz() or n.is_mpq())
                return n.to_canonical();

        std::cerr << "\nn = " << n << "\n";
        std::cerr << "is_pyobject = " << n.is_pyobject() << "\n";
        throw std::runtime_error("num2canonical: can't happen");
}

static CanonicalForm replace_with_symbol(const ex& e, ex_int_map& map, exvector& revmap)
{
        // Expression already replaced? Then return the assigned symbol
        auto it = map.find(e);
        if (it != map.end()) {
                return Variable(it->second);
        }

        // Otherwise create new symbol and add to dict
        const int index = revmap.size() + 1;
        map.insert(std::make_pair(e, index));
        revmap.push_back(e);
        return Variable(index);
}

static symbol symbol_E;

static void add_to_pomap(power_ocvector_map& pomap,
                const ex& basis, const ex& expo, const numeric& num)
{
        auto pow = GiNaC::power(basis, expo);
        auto f = pomap.find(pow);
        if (f == pomap.end()) {
                ocvector vec;
                vec.push_back(num);
                pomap[pow] = std::move(vec);
        }
        else
                pomap[pow].push_back(num);
}

void ex::collect_powers(power_ocvector_map& pomap) const
{
        if (is_exactly_a<power>(*this)) {
                const power& the_pow = ex_to<power>(*this);
                if (is_exactly_a<numeric>(the_pow.op(1))) {
                        numeric n = ex_to<numeric>(the_pow.op(1));
                        if (n.is_rational())
                                add_to_pomap(pomap, the_pow.op(0), _ex1, n);
                }
                if (is_exactly_a<mul>(the_pow.op(1))) {
                        mul m = ex_to<mul>(the_pow.op(1));
                        numeric oc = ex_to<numeric>(m.overall_coeff);
                        if (oc.is_rational()) {
                                m.overall_coeff = _ex1;
                                ex mm = m.eval();
                                add_to_pomap(pomap, the_pow.op(0), mm, oc);
                        }
                }
        }
        else if (is_exactly_a<add>(*this)) {
                const add& a = ex_to<add>(*this);
                for (unsigned int i=0; i<a.nops(); i++)
                        a.op(i).collect_powers(pomap);
        }
        else if (is_exactly_a<mul>(*this)) {
                const mul& m = ex_to<mul>(*this);
                for (const auto & elem : m.get_sorted_seq())
                        m.recombine_pair_to_ex(elem).collect_powers(pomap);
        }
        else if (is_exactly_a<function>(*this)) {
                const function& f = ex_to<function>(*this);
                if (f.get_serial() == exp_SERIAL::serial) {
                        if (is_exactly_a<numeric>(f.op(0))) {
                                numeric n = ex_to<numeric>(f.op(0));
                                if (n.is_rational())
                                        add_to_pomap(pomap, symbol_E, _ex1, n);
                        }
                        if (is_exactly_a<mul>(f.op(0))) {
                                mul m = ex_to<mul>(f.op(0));
                                numeric oc = ex_to<numeric>(m.overall_coeff);
                                if (oc.is_rational()) {
                                        m.overall_coeff = _ex1;
                                        ex mm = m.eval();
                                        add_to_pomap(pomap, symbol_E, mm, oc);
                                }
                        }
                }
                else
                        add_to_pomap(pomap, f, _ex1, *_num1_p);
        }
        else if (is_exactly_a<constant>(*this)
                        or is_exactly_a<symbol>(*this))
                add_to_pomap(pomap, *this, _ex1, *_num1_p);
}


static void transform_powers(power_ocvector_map& pomap)
{
        for (auto& it : pomap) {
                numeric g(*_num1_p);
                for (const numeric& num : it.second)
                        g = g.gcd(num);
                (it.second)[0] = g;
        }
}

// Convert to Singular polynomial over QQ, filling replacement dicts
const CanonicalForm ex::to_canonical(ex_int_map& map,
                power_ocvector_map& pomap,
                exvector& revmap) const
{
        if (is_exactly_a<add>(*this))
        {
                const add& a = ex_to<add>(*this);
                CanonicalForm p(0);
                for (const auto& termex : a.seq) {
                        p = p + a.recombine_pair_to_ex(termex).to_canonical(map, pomap, revmap);
                }
                p = p + a.overall_coeff.to_canonical(map, pomap, revmap);
                return p;
        }
        else if (is_exactly_a<numeric>(*this))
        {
                numeric num = ex_to<numeric>(*this);
                if (num.is_real()) {
                        if (num.is_rational())
                                return num2canonical(num);
                        else
                                return replace_with_symbol(num, map, revmap);
                } else { // complex
                        numeric re = num.real();
                        numeric im = num.imag();
                        CanonicalForm re_p, im_p;
                        if (re.is_rational())
                                re_p = num2canonical(re);
                        else
                                re_p = replace_with_symbol(re, map, revmap);
                        if (im.is_rational())
                                im_p = num2canonical(im);
                        else
                                im_p = replace_with_symbol(im, map, revmap);
                        return re_p + im_p * replace_with_symbol(I, map, revmap);
                }
        }
        else if (is_exactly_a<mul>(*this))
        {
                const mul& m = ex_to<mul>(*this);
                CanonicalForm p = num2canonical(*_num1_p);
                for (const auto& termex : m.seq) {
                        p = p * m.recombine_pair_to_ex(termex).to_canonical(map, pomap, revmap);
                }
                p = p * m.overall_coeff.to_canonical(map, pomap, revmap);
                return p;
        }
        else if (is_exactly_a<power>(*this))
        {
                const power& pow = ex_to<power>(*this);
                if (is_exactly_a<numeric>(pow.exponent)) {
                        numeric expo = ex_to<numeric>(pow.exponent);
                        if (expo.is_rational()) {
                                auto var = replace_with_symbol(pow.basis, map, revmap);
                                auto it = pomap.find(pow.basis);
                                if (it == pomap.end())
                                        throw std::runtime_error("can't happen in ex::to_canonical");
                                numeric n = expo.div(it->second[0]);
                                revmap[var.level()-1] = GiNaC::power(it->first, it->second[0]);
                                return ::power(var, n.to_int());
                        }
                }
                if (is_exactly_a<mul>(pow.exponent)) {
                        mul m = ex_to<mul>(pow.exponent);
                        numeric oc = ex_to<numeric>(m.overall_coeff);
                        if (oc.is_rational()) {
                                m.overall_coeff = _ex1;
                                ex mm = m.eval();
                                auto it = pomap.find(GiNaC::power(pow.basis, mm));
                                if (it == pomap.end())
                                        throw std::runtime_error("can't happen in ex::to_canonical");
                                auto var = replace_with_symbol(it->first, map, revmap);
                                numeric n = oc.div(it->second[0]);
                                revmap[var.level()-1] = GiNaC::power(it->first, it->second[0]);
                                return ::power(var, n.to_int());
                        }
                }
                return replace_with_symbol(*this, map, revmap);
        }

        return replace_with_symbol(*this, map, revmap);
}

static numeric can2num(const CanonicalForm& f)
{
        if (f.isOne())
                return *_num1_p;
        if (f.isImm())
                return numeric(f.intval());
        mpz_t bignum;
        f.mpzval(bignum);
        return numeric(bignum);
}

static ex coeff_to_ex(const CanonicalForm& f, const exvector& revmap)
{
        if (f.isOne())
                return _ex1;
        if (f.isImm())
                return numeric(f.intval());
        if (f.inZ())
                throw std::runtime_error("can't happen in coeff_to_ex");
        if (f.inQ()) {
                CanonicalForm num = f.num();
                CanonicalForm den = f.den();
                return can2num(num)/can2num(den);
        }
        ex res = _ex0;
        for ( CFIterator it = f; it.hasTerms(); it++ ) {
                res += mul(GiNaC::power(revmap.at(f.level()-1), it.exp()),
                                 coeff_to_ex(it.coeff(), revmap));
        }
        return res;
}

static ex canonical_to_ex(const CanonicalForm& f, const exvector& revmap)
{
        if (f.isOne())
                return _ex1;
        if (f.inCoeffDomain()) {
                if (f.isImm())
                        return numeric(f.intval());
                if (f.inZ())
                        throw std::runtime_error("can't happen in canonical_to_ex #1");
                if (f.inQ()) {
                        CanonicalForm num = f.num();
                        CanonicalForm den = f.den();
                        mpz_t bigintnum, bigintden;
                        if (num.isImm()) {
                                mpz_init(bigintnum);
                                mpz_set_si (bigintnum, num.intval());
                        }
                        else
                                num.mpzval(bigintnum);
                        numeric n(bigintnum);
                        if (den.isImm()) {
                                mpz_init(bigintden);
                                mpz_set_si (bigintden, den.intval());
                        }
                        else
                                den.mpzval(bigintden);
                        numeric d(bigintden);
                        return n/d;
                }
                else {
                        throw std::runtime_error("can't happen in canonical_to_ex #2");
                }
        }

        ex e = _ex0;
        for ( CFIterator i = f; i.hasTerms(); i++ ) {
                e += mul(coeff_to_ex(i.coeff(), revmap),
                                   power(revmap.at(f.level()-1), i.exp()));
        }
        return e;
}

// GCD of two exes which are in polynomial form
// If giac is requested we stand back
#ifndef PYNAC_HAVE_LIBGIAC
ex gcdpoly(const ex &a, const ex &b, ex *ca=nullptr, ex *cb=nullptr, bool check_args=true)
{
        if (a.is_zero())
                return b;
        if (b.is_zero())
                return a;

        // GCD of numerics
	if (is_exactly_a<numeric>(a) && is_exactly_a<numeric>(b)) {
		numeric g = gcd(ex_to<numeric>(a), ex_to<numeric>(b));
		if ((ca != nullptr) || (cb != nullptr)) {
			if (g.is_zero()) {
				if (ca != nullptr)
					*ca = _ex0;
				if (cb != nullptr)
					*cb = _ex0;
			} else {
				if (ca != nullptr)
					*ca = ex_to<numeric>(a) / g;
				if (cb != nullptr)
					*cb = ex_to<numeric>(b) / g;
			}
		}
		return g;
	}

	// Partially factored cases (to avoid expanding large expressions)
	if (is_exactly_a<mul>(a)) {
		if (is_exactly_a<mul>(b) && b.nops() > a.nops())
			goto factored_b;
factored_a:
		size_t num = a.nops();
		exvector g; g.reserve(num);
		exvector acc_ca; acc_ca.reserve(num);
		ex part_b = b;
		for (size_t i=0; i<num; i++) {
			ex part_ca, part_cb;
			g.push_back(gcdpoly(a.op(i), part_b, &part_ca, &part_cb, check_args));
			acc_ca.push_back(part_ca);
			part_b = part_cb;
		}
		if (ca != nullptr)
			*ca = (new mul(acc_ca))->setflag(status_flags::dynallocated);
		if (cb != nullptr)
			*cb = part_b;
		return (new mul(g))->setflag(status_flags::dynallocated);
	} else if (is_exactly_a<mul>(b)) {
		if (is_exactly_a<mul>(a) && a.nops() > b.nops())
			goto factored_a;
factored_b:
		size_t num = b.nops();
		exvector g; g.reserve(num);
		exvector acc_cb; acc_cb.reserve(num);
		ex part_a = a;
		for (size_t i=0; i<num; i++) {
			ex part_ca, part_cb;
			g.push_back(gcdpoly(part_a, b.op(i), &part_ca, &part_cb, check_args));
			acc_cb.push_back(part_cb);
			part_a = part_ca;
		}
		if (ca != nullptr)
			*ca = part_a;
		if (cb != nullptr)
			*cb = (new mul(acc_cb))->setflag(status_flags::dynallocated);
		return (new mul(g))->setflag(status_flags::dynallocated);
	}

	// Input polynomials of the form poly^n are sometimes also trivial
	if (is_exactly_a<power>(a)) {
		ex p = a.op(0);
		const ex& exp_a = a.op(1);
		if (is_exactly_a<power>(b)) {
			ex pb = b.op(0);
			const ex& exp_b = b.op(1);
			if (p.is_equal(pb)) {
				// a = p^n, b = p^m, gcd = p^min(n, m)
				if (exp_a < exp_b) {
					if (ca != nullptr)
						*ca = _ex1;
					if (cb != nullptr)
						*cb = power(p, exp_b - exp_a);
					return power(p, exp_a);
				} else {
					if (ca != nullptr)
						*ca = power(p, exp_a - exp_b);
					if (cb != nullptr)
						*cb = _ex1;
					return power(p, exp_b);
				}
			} else {
				ex p_co, pb_co;
				ex p_gcd = gcdpoly(p, pb, &p_co, &pb_co, check_args);
				if (p_gcd.is_equal(_ex1)) {
					// a(x) = p(x)^n, b(x) = p_b(x)^m, gcd (p, p_b) = 1 ==>
					// gcd(a,b) = 1
					if (ca != nullptr)
						*ca = a;
					if (cb != nullptr)
						*cb = b;
					return _ex1;
					// XXX: do I need to check for p_gcd = -1?
				} else {
					// there are common factors:
					// a(x) = g(x)^n A(x)^n, b(x) = g(x)^m B(x)^m ==>
					// gcd(a, b) = g(x)^n gcd(A(x)^n, g(x)^(n-m) B(x)^m
					if (exp_a < exp_b) {
						return power(p_gcd, exp_a)*
							gcdpoly(power(p_co, exp_a), power(p_gcd, exp_b-exp_a)*power(pb_co, exp_b), ca, cb, false);
					} else {
						return power(p_gcd, exp_b)*
							gcdpoly(power(p_gcd, exp_a - exp_b)*power(p_co, exp_a), power(pb_co, exp_b), ca, cb, false);
					}
				} // p_gcd.is_equal(_ex1)
			} // p.is_equal(pb)

		} else {
			if (p.is_equal(b)) {
				// a = p^n, b = p, gcd = p
				if (ca != nullptr)
					*ca = power(p, a.op(1) - 1);
				if (cb != nullptr)
					*cb = _ex1;
				return p;
			} 

			ex p_co, bpart_co;
			ex p_gcd = gcdpoly(p, b, &p_co, &bpart_co, false);

			if (p_gcd.is_equal(_ex1)) {
				// a(x) = p(x)^n, gcd(p, b) = 1 ==> gcd(a, b) = 1
				if (ca != nullptr)
					*ca = a;
				if (cb != nullptr)
					*cb = b;
				return _ex1;
			} else {
				// a(x) = g(x)^n A(x)^n, b(x) = g(x) B(x) ==> gcd(a, b) = g(x) gcd(g(x)^(n-1) A(x)^n, B(x))
				return p_gcd*gcdpoly(power(p_gcd, exp_a-1)*power(p_co, exp_a), bpart_co, ca, cb, false);
			}
		} // is_exactly_a<power>(b)

	} else if (is_exactly_a<power>(b)) {
		ex p = b.op(0);
		if (p.is_equal(a)) {
			// a = p, b = p^n, gcd = p
			if (ca != nullptr)
				*ca = _ex1;
			if (cb != nullptr)
				*cb = power(p, b.op(1) - 1);
			return p;
		}

		ex p_co, apart_co;
		const ex& exp_b(b.op(1));
		ex p_gcd = gcdpoly(a, p, &apart_co, &p_co, false);
		if (p_gcd.is_equal(_ex1)) {
			// b=p(x)^n, gcd(a, p) = 1 ==> gcd(a, b) == 1
			if (ca != nullptr)
				*ca = a;
			if (cb != nullptr)
				*cb = b;
			return _ex1;
		} else {
			// there are common factors:
			// a(x) = g(x) A(x), b(x) = g(x)^n B(x)^n ==> gcd = g(x) gcd(g(x)^(n-1) A(x)^n, B(x))

			return p_gcd*gcdpoly(apart_co, power(p_gcd, exp_b-1)*power(p_co, exp_b), ca, cb, false);
		} // p_gcd.is_equal(_ex1)
	}


        ex_int_map map;
        exvector revmap;
        On(SW_RATIONAL);
        setCharacteristic(0);
        power_ocvector_map pomap;
        a.collect_powers(pomap);
        b.collect_powers(pomap);
        transform_powers(pomap);
        CanonicalForm p = a.to_canonical(map, pomap, revmap);
        CanonicalForm q = b.to_canonical(map, pomap, revmap);
        CanonicalForm d = gcd(p, q);
        ex res = canonical_to_ex(d, revmap);

        if (ca != nullptr) {
                ex quo;
                if (divide(a, res, quo))
                        *ca = quo;
                else
                        throw(std::runtime_error("can't happen in gcdpoly"));
        }
        if (cb != nullptr) {
                ex quo;
                if (divide(b, res, quo))
                        *cb = quo;
                else
                        throw(std::runtime_error("can't happen in gcdpoly"));
        }
        return res;
}

ex resultantpoly(const ex & ee1, const ex & ee2, const ex & s)
{
        ex_int_map map;
        exvector revmap;
        On(SW_RATIONAL);
        setCharacteristic(0);
        power_ocvector_map pomap;
        ee1.collect_powers(pomap);
        ee2.collect_powers(pomap);
        transform_powers(pomap);
        CanonicalForm p = ee1.to_canonical(map, pomap, revmap);
        CanonicalForm q = ee2.to_canonical(map, pomap, revmap);
        Variable v;
        auto it = map.find(s);
        if (it != map.end())
                v = it->second;
        else
                v = Variable(int(revmap.size() + 1));
        CanonicalForm d = ::resultant(p, q, v);
        return canonical_to_ex(d, revmap);
}

bool factorpoly(const ex& the_ex, ex& res_prod)
{
        if (is_exactly_a<numeric>(the_ex)
            or is_exactly_a<constant>(the_ex)
            or is_exactly_a<function>(the_ex)
            or is_exactly_a<symbol>(the_ex))
                return false;

        if (is_exactly_a<mul>(the_ex)) {
                res_prod = _ex1;
                const mul& m = ex_to<mul>(the_ex);
                bool all_prime = true;
                for (const auto & elem : m.get_sorted_seq()) {
                        ex prod;
                        ex term = m.recombine_pair_to_ex(elem);
                        bool res = factor(term, prod);
                        if (res) {
                                res_prod = mul(res_prod, prod);
                                all_prime = false;
                        }
                        else
                                res_prod = mul(res_prod, term);
                }
                res_prod = mul(res_prod, m.op(m.nops()));
                return not all_prime;
        }

        if (is_exactly_a<power>(the_ex)) {
                const power& pow = ex_to<power>(the_ex);
                ex prod;
                bool res = factor(pow.op(0), prod);
                if (not res)
                        return false;
                res_prod = power(prod, pow.op(1));
                return true;
        }

        if (not is_exactly_a<add>(the_ex))
                throw(std::runtime_error("can't happen in factor"));


        ex_int_map map;
        exvector revmap;

        power_ocvector_map pomap;
        the_ex.collect_powers(pomap);
        transform_powers(pomap);
        CanonicalForm p = the_ex.to_canonical(map, pomap, revmap);
        CFFList factors = factorize(p);

        if (factors.length() == 1 or factors.isEmpty())
                return false;

        res_prod = _ex1;
        for (CFFListIterator iter = factors; iter.hasItem(); iter++) {
                res_prod = mul(res_prod,
                                power(canonical_to_ex(iter.getItem().factor(),
                                                revmap).expand(),
                                        iter.getItem().exp()));
        }
        return true;
}
#endif //PYNAC_HAVE_LIBGIAC

} // namespace GiNaC

