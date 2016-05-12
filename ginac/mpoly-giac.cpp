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

#ifdef HAVE_GIAC

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

#include "giac.h"

namespace GiNaC {

ex find_common_factor(const ex & e, ex & factor, exmap & repl)
{
        return _ex0;
}

ex gcd(const ex &a, const ex &b, ex *ca=nullptr, ex *cb=nullptr, bool check_args=true)
{
        return _ex0;
}

} // namespace GiNaC

#endif // HAVE_GIAC
