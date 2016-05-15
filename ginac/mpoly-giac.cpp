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

#ifdef HAVE_CONFIG_H
#include "pynac-config.h"
#endif

#ifdef PYNAC_HAVE_LIBGIAC

#include <string>
#include <iostream>
#include <sstream>
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE

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
#include "symbol.h"
#include "function.h"
#include "utils.h"

#include <giac/global.h>
#include <giac/gausspol.h>

namespace GiNaC {

inline giac::polynome&& gen2pol(const giac::gen& g) {
        return std::move(giac::polynome(giac::monomial<giac::gen>(g, 1)));
}

inline giac::gen num2gen(const numeric& n) {
        std::stringstream ss;
        ss << n;
        return giac::gen(std::string(ss.str()), giac::context0);
}

static giac::gen giac_one = giac::gen(std::string("1"), giac::context0);

static giac::polynome&& replace_with_symbol(const ex& e, ex_int_map& map, exvector& revmap)
{
        // Expression already replaced? Then return the assigned symbol
        auto it = map.find(e);
        if (it != map.end()) {
                const int dim = it->second;
                giac::monomial<giac::gen> mon(giac_one, dim, dim);
                return std::move(giac::polynome(mon));
        }

        // Otherwise create new symbol and add to dict
        const int index = revmap.size() + 1;
        map.insert(std::make_pair(e, index));
        revmap.push_back(e);
        giac::monomial<giac::gen> mon(giac_one, index, index);
        return std::move(giac::polynome(mon));
}

// Convert to giac polynomial over ZZ, filling replacement dicts
// TODO: special case numeric mpz_t, int instead of string interface
// TODO: is it faster to add/mul monomials instead of polynomes?
const giac::polynome&& ex::to_polynome(ex_int_map& map, exvector& revmap) const
{
        if (is_exactly_a<add>(*this))
        {
                const add& a = ex_to<add>(*this);
                giac::polynome&& p = gen2pol(giac::gen(std::string("0"), giac::context0));
                for (const auto& termex : a.seq) {
                        p = p + a.recombine_pair_to_ex(termex).to_polynome(map, revmap);
                }
                p = p + a.overall_coeff.to_polynome(map, revmap);
                return std::move(p);
        }
        else if (is_exactly_a<numeric>(*this))
        {
                const numeric& num = ex_to<numeric>(*this);
                if (num.is_real()) {
                        if (num.is_integer())
                                return gen2pol(num2gen(num));
                        else
                                return replace_with_symbol(num, map, revmap);
                } else { // complex
                        numeric re = num.real();
                        numeric im = num.imag();
                        giac::polynome re_p, im_p;
                        if (re.is_integer())
                                re_p = gen2pol(num2gen(re));
                        else
                                re_p = replace_with_symbol(re, map, revmap);
                        if (im.is_integer())
                                im_p = gen2pol(num2gen(im));
                        else
                                im_p = replace_with_symbol(im, map, revmap);
                        return std::move(re_p + im_p * replace_with_symbol(I, map, revmap));
                }
        }
        else if (is_exactly_a<mul>(*this))
        {
                const mul& m = ex_to<mul>(*this);
                giac::polynome&& p = gen2pol(giac::gen(std::string("1"), giac::context0));
                for (const auto& termex : m.seq) {
                        p *= m.recombine_pair_to_ex(termex).to_polynome(map, revmap);
                }
                p *= m.overall_coeff.to_polynome(map, revmap);
                return std::move(p);
        }
        else if (is_exactly_a<power>(*this))
        {
                const power& pow = ex_to<power>(*this);
                if (is_exactly_a<numeric>(pow.exponent)) {
                        numeric expo = ex_to<numeric>(pow.exponent);
                        if (pow.exponent.info(info_flags::posint))
                                return std::move(giac::pow(pow.basis.to_polynome(map, revmap), expo.to_int()));
                        else if (pow.exponent.info(info_flags::negint))
                                return std::move(giac::pow(power(pow.basis, _ex_1).to_polynome(map, revmap), -expo.to_int()));
                }
                return replace_with_symbol(*this, map, revmap);
        }

        return replace_with_symbol(*this, map, revmap);
}

static ex polynome_to_ex(giac::polynome p, ex_int_map& map)
{
        ex e = _ex0;
        /*
        for (const auto& mon : p.coord) {
                ex prod = _ex1;
                for (const auto& varno : mon.index)
                        prod */
        return e;
}

ex gcd(const ex &a, const ex &b, ex *ca=nullptr, ex *cb=nullptr, bool check_args=true)
{
        ex_int_map map;
        exvector revmap;
        giac::polynome p = a.to_polynome(map, revmap);
        giac::polynome q = b.to_polynome(map, revmap);
        giac::polynome d(p.dim);
        giac::gcd(p, q, d);

        return polynome_to_ex(d, map);
}

} // namespace GiNaC

#endif // HAVE_LIBGIAC
