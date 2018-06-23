//////////////////////////////////////////////////////////////////////
//
// Partial implementation of rule based integration (Rubi)
//
// Ref.: Rich, Albert D., and David J. Jeffrey. "A knowledge
//       repository for indefinite integration based on transformation
//       rules." International Conference on Intelligent Computer
//       Mathematics. Springer, Berlin, Heidelberg, 2009.
//
// Author: 2018: Ralf Stephan <ralf@ark.in-berlin.de>
// License: GPL2+
//
//////////////////////////////////////////////////////////////////////

#include <Python.h>
#include "numeric.h"
#include "power.h"
#include "add.h"
#include "mul.h"
#include "symbol.h"
#include "constant.h"
#include "relational.h"
#include "ex.h"
#include "normal.h"
#include "upoly.h"
#include "utils.h"
#include "operators.h"
#include "inifcns.h"
#include "py_funcs.h"
#include "ex_utils.h"
#include "wildcard.h"
#include "function.h"

#ifdef PYNAC_HAVE_LIBGIAC
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#endif

#include <stdexcept>
#include <functional>

#if PY_MAJOR_VERSION >= 3
#define PyString_FromString PyUnicode_FromString
#endif

#define DEBUG if (debug)
static bool debug=true;

inline void py_error(const char* errmsg) {
        throw std::runtime_error((PyErr_Occurred() != nullptr) ? errmsg:
                        "pyerror() called but no error occurred!");
}

namespace GiNaC {

DECLARE_FUNCTION_2P(__tuple)
unsigned __tuple_SERIAL::serial = function::register_new(function_options("__tuple", 2));

class rubi_exception : public std::runtime_error {
        public:
                rubi_exception() : std::runtime_error("") {}
};

ex rubi(ex e, ex x);
static ex rubi_2prod(const exvector& bvec, const exvector& evec,
                const symbol& x);
static ex rubi_3prod(const exvector& bvec, const exvector& evec,
                const symbol& x);
static ex rubi11(ex e, ex x);
static ex rubi111(ex a, ex b, ex m, ex x);
static ex rubi112(ex ae, ex be, ex me, ex ce, ex de, ex ne, ex x);
static ex rubi113(ex a, ex b, ex m, ex c, ex d, ex n, ex e, ex f, ex p, ex x);
static ex rubi131(ex ae, ex be, ex ne, ex pe, ex x);
static ex rubi132(ex ce, ex me, ex ae, ex be, ex ne, ex pe, ex x);
static ex rubi133(ex a, ex b, ex c, ex d, ex n, ex p, ex q, ex x);
static ex rubi134(ex m, ex a, ex b, ex n, ex p, ex c, ex d, ex q, ex x);
static ex rubi141H(ex a, ex j, ex b, ex n, ex p, ex x);
static ex rubi142H(ex m, ex a, ex j, ex b, ex n, ex p, ex x);
static ex rubi1x1(ex bas, ex exp, ex xe);
static ex rubi211(ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi212(ex de, ex ee, ex me, ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi213(ex de, ex ee, ex me, ex fe, ex ge, ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi214(ex de, ex ee, ex me, ex fe, ex ge, ex ne, ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi222(ex de, ex me, ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi223(ex de, ex ee, ex qe, ex ae, ex be, ex ce, ex pe, ex x);
static bool rubi91(ex& the_ex, ex& f, const symbol& x);

static ex dist(const ex& f, const ex& p)
{
        if (not is_exactly_a<add>(p))
                return mul(f, p);
        ex s = _ex0;
        for (size_t i=0; i<p.nops(); ++i)
                s += f*p.op(i);
        return s;
}

static ex remove_content(const ex& e, const ex& x)
{
        if (not is_exactly_a<add>(e))
                return e;
        expairvec coeffs;
        e.coefficients(x, coeffs);
        ex lc = _ex1, d = _ex1;
        nonstd::optional<numeric> expo;
        for (const auto& p : coeffs) {
                if (is_exactly_a<numeric>(p.second)) {
                        const numeric& t = ex_to<numeric>(p.second);
                        if ((expo and expo.value() < t)
                            or not expo) {
                                expo = t;
                                lc = p.first;
                        }
                }
                d *= p.first.denom();
        }
        if (lc.is_negative_or_minus())
                d = _ex_1*d;
        return dist(d, e);
}

bool rubi_integrate(ex e, ex x, ex& ret)
{
        try {
                ret = rubi(e, x);
        }
        catch (rubi_exception) {
                return false;
        }
        return true;
}


ex rubi(ex e, ex xe)
{
        DEBUG std::cerr<<"r: "<<e<<std::endl;
        if (not is_exactly_a<symbol>(xe))
                throw std::runtime_error("rubi(): not a symbol");
        const symbol& x = ex_to<symbol>(xe);
        if (not has_symbol(e, x))
                return e*x;
        if (e.is_equal(x))
                return power(x, _ex2) / _ex2;
        ex the_ex = e.collect_powers();
        if (is_exactly_a<add>(the_ex)) {
                const add& m = ex_to<add>(the_ex);
                exvector vec;
                for (unsigned int i=0; i<m.nops(); i++)
                        vec.push_back(rubi(m.op(i), ex(x)));
                return add(vec);
        }
        if (is_exactly_a<mul>(the_ex)) {
                ex res_ex;
                bool ff = factor(the_ex, res_ex);
                if (ff)
                        the_ex = res_ex;
                if (not has_symbol(the_ex, x))
                        return the_ex*x;
                ex factor;
                if (rubi91(the_ex, factor, x))
                        return dist(factor, rubi(the_ex, x));
                const mul& mu = ex_to<mul>(the_ex);
                exvector cvec, xvec;
                for (unsigned int i=0; i<mu.nops(); i++) {
                        if (has_symbol(mu.op(i), x))
                                xvec.push_back(mu.op(i));
                        else
                                cvec.push_back(mu.op(i));
                }
                exvector bvec, evec;
                for (const auto& term : xvec) {
                        if (is_exactly_a<power>(term)) {
                                ex p = ex_to<power>(term);
                                if (is_exactly_a<power>(p.op(0))) {
                                        if (p.op(1).is_integer()) {
                                                bvec.push_back(p.op(0).op(0).expand());
                                                evec.push_back(p.op(1)*p.op(0).op(1));
                                                continue;
                                        }
                                }
                                bvec.push_back(p.op(0).expand());
                                evec.push_back(p.op(1));
                        }
                        else {
                                bvec.push_back(term.expand());
                                evec.push_back(_ex1);
                        }
                }
                // exponentials ---> bailout
                for (const auto& eterm : evec)
                        if (has_symbol(eterm, x))
                                throw rubi_exception();

                // a(b+cx)
                if (xvec.size() == 1
                    and not cvec.empty()
                    and evec[0].is_one()) {
                        ex a, b;
                        if (bvec[0].is_linear(x, a, b))
                                return mul(cvec)*power(a+b*x,_ex2)/_ex2/b;
                }
                if (not cvec.empty())
                        return mul(cvec) * rubi(mul(xvec), x);
                if (xvec.size() == 2)
                        return rubi_2prod(bvec, evec, x);
                if (xvec.size() == 3)
                        return rubi_3prod(bvec, evec, x);
                throw rubi_exception();
        }
        ex a, b, c;
        if (is_exactly_a<power>(e)) {
                const power& p = ex_to<power>(e);
                const ex& ebas = p.op(0);
                const ex& m = p.op(1);
                ex j, n;
                if (has_symbol(ebas, x) and not has_symbol(m, x)) {
                        ex w0=wild(0), w1=wild(1), w2=wild(2), w3=wild(3);
                        exvector w;
                        // (a+bu)^m
                        if (e.match(power(w0 + w1*w2, w3), w)) {
                                if (not w[2].is_equal(x)
                                    and w[2].is_linear(x,a,b)
                                    and not b.is_zero()
                                    and not has_symbol(w[0]*w[1], x)) {
                              DEBUG std::cerr<<"(a+bu)^m (1): "<<w[0]<<", "<<w[1]<<", "<<w[2]<<", "<<w[3]<<std::endl;
                                        return dist(_ex1/b, rubi111(w[0],w[1],w[3],x).subs(x == w[2]));
                                }
                                if (not w[1].is_equal(x)
                                    and w[1].is_linear(x,a,b)
                                    and not b.is_zero()
                                    and not has_symbol(w[0]*w[2], x)) {
                              DEBUG std::cerr<<"(a+bu)^m: (2)"<<w[0]<<", "<<w[1]<<", "<<w[2]<<", "<<w[3]<<std::endl;
                                        return dist(_ex1/b, rubi111(w[0],w[2],w[3],x).subs(x == w[1]));
                                }
                        }
                        if (ebas.is_linear(x, a, b))
                                return rubi111(a, b, m, ex(x));
                        if (ebas.is_quadratic(x, a, b, c)) {
                                if (b.is_zero())
                                        return rubi131(a, c, _ex2, m, ex(x));
                                else
                                        return rubi211(a, b, c, m, ex(x));
                        }
                        if (ebas.is_binomial(x, a, j, b, n)) {
                                if (j.is_zero())
                                        return rubi131(a, b, n, m, ex(x));
                                else if (n.is_zero())
                                        return rubi131(b, a, j, m, ex(x));
                                else
                                        return rubi141H(a, j, b, n, m, ex(x));
                        }
                        if (ebas.is_quadratic(x, a, b, c)) {
                                if (b.is_zero())
                                        return rubi131(a, c, _ex2, m, ex(x));
                                else
                                        return rubi211(a, b, c, m, ex(x));
                        }
                        return rubi1x1(ebas, m, ex(x));
                }
        }
        return rubi11(the_ex, x);
}

// term1^exp1 * term2^exp2
// this all presupposes the exponents inside the terms are posint
static ex rubi_2prod(const exvector& bvec, const exvector& evec,
                const symbol& x)
{
        ex a, b, c, d, e, j, k, m, n;
        DEBUG std::cerr<<bvec[0]<<","<<bvec[1]<<std::endl;
        if (bvec[0].is_linear(x, a, b)
            and bvec[1].is_linear(x, c, d))
                return rubi112(a, b, evec[0], c, d, evec[1], x);
        
        if (bvec[0].is_linear(x, c, d)
            and c.is_zero()
            and bvec[1].is_binomial(x, a, j, b, n)) {
                if (j.is_zero())
                        return rubi132(d, evec[0], a, b, n, evec[1], x);
                if (n.is_zero())
                        return rubi132(d, evec[0], b, a, j, evec[1], x);
                return power(d,evec[0]) * rubi142H(evec[0], a, j, b, n, evec[1], x);
        }
        if (bvec[1].is_linear(x, a, b)
            and a.is_zero()
            and bvec[0].is_binomial(x, c, j, d, n)) {
                if (j.is_zero())
                        return rubi132(b, evec[1], c, d, n, evec[0], x);
                if (n.is_zero())
                        return rubi132(b, evec[1], d, c, j, evec[0], x);
                return power(b,evec[1]) * rubi142H(evec[1], c, j, d, n, evec[0], x);
        }
        if (bvec[0].is_linear(x, c, d)
            and bvec[1].is_binomial(x, a, j, b, n)) {
                if (j.is_zero() and n.is_equal(_ex2))
                        return rubi212(c, d, evec[0], a, _ex0, b, evec[1], x);
                if (n.is_zero() and j.is_equal(_ex2))
                        return rubi212(c, d, evec[0], b, _ex0, a, evec[1], x);
                throw rubi_exception();
        }
        if (bvec[1].is_linear(x, a, b)
            and bvec[0].is_binomial(x, c, j, d, n)) {
                if (j.is_zero() and n.is_equal(_ex2))
                        return rubi212(a, b, evec[1], c, _ex0, d, evec[0], x);
                if (n.is_zero() and j.is_equal(_ex2))
                        return rubi212(a, b, evec[1], d, _ex0, c, evec[0], x);
                throw rubi_exception();
        }
        if (bvec[0].is_linear(x, d, e)
            and bvec[1].is_quadratic(x, a, b, c))
                return rubi212(d, e, evec[0], a, b, c, evec[1], x);
        if (bvec[1].is_linear(x, d, e)
            and bvec[0].is_quadratic(x, a, b, c))
                return rubi212(d, e, evec[1], a, b, c, evec[0], x);
        if (bvec[0].is_binomial(x, a, j, b, n)
            and bvec[1].is_binomial(x, c, k, d, m)
            and (j*n).is_zero()
            and (k*m).is_zero()
            and (j+n).is_equal(k+m)) {
                if (n.is_zero()) {
                        a.swap(b);
                        j.swap(n);
                }
                if (m.is_zero()) {
                        c.swap(d);
                        k.swap(m);
                }
                return rubi133(a,b,c,d,n,evec[0],evec[1],x);
        }

        // other
        expairvec vec1, vec2;
        bvec[0].coefficients(x, vec1);
        bvec[1].coefficients(x, vec2);
        exvector c1, c2, e1, e2;
        firsts(c1, vec1);
        firsts(c2, vec2);
        seconds(e1, vec1);
        seconds(e2, vec2);
        DEBUG for (auto ee : e1)
              std::cerr << ee << ",";
        DEBUG std::cerr << std::endl;  
        DEBUG for (auto ee : e2)
              std::cerr << ee << ",";
        DEBUG std::cerr << std::endl;  

        if (ex_match_int(e1, {1})
            and ex_match_int(e2, {0,2,4})) {
                return rubi222(c1[0],evec[0],c2[0],c2[1],c2[2],evec[1],x);
        }
        if (ex_match_int(e2, {1})
            and ex_match_int(e1, {0,2,4})) {
                return rubi222(c2[0],evec[1],c1[0],c1[1],c1[2],evec[0],x);
        }
        if (ex_match_int(e1, {0,2})
            and ex_match_int(e2, {0,2,4})) {
                return rubi223(c1[0],c1[1],evec[0],c2[0],c2[1],c2[2],evec[1],x);
        }
        if (ex_match_int(e2, {0,2})
            and ex_match_int(e1, {0,2,4})) {
                return rubi223(c2[0],c2[1],evec[1],c1[0],c1[1],c1[2],evec[0],x);
        }
        if (ex_match_int(e1, {0,2})
            and ex_match_int(e2, {0,4})) {
                return rubi223(c1[0],c1[1],evec[0],c2[0],_ex0,c2[1],evec[1],x);
        }
        if (ex_match_int(e2, {0,2})
            and ex_match_int(e1, {0,4})) {
                return rubi223(c2[0],c2[1],evec[1],c1[0],_ex0,c1[1],evec[0],x);
        }
        throw rubi_exception();
}

// term1^exp1 * term2^exp2 * term3^exp3
// this all presupposes the exponents inside the terms are posint
static ex rubi_3prod(const exvector& bvec, const exvector& evec,
                const symbol& x)
{
        expairvec vec1, vec2, vec3;
        bvec[0].coefficients(x, vec1);
        bvec[1].coefficients(x, vec2);
        bvec[2].coefficients(x, vec3);
        exvector c1, c2, c3, ev1, ev2, ev3;
        firsts(c1, vec1);
        firsts(c2, vec2);
        firsts(c3, vec3);
        seconds(ev1, vec1);
        seconds(ev2, vec2);
        seconds(ev3, vec3);
        DEBUG for (auto e : ev1)
              std::cerr << e << ",";
        DEBUG std::cerr << std::endl;  
        DEBUG for (auto e : ev2)
              std::cerr << e << ",";
        DEBUG std::cerr << std::endl;  
        DEBUG for (auto e : ev3)
              std::cerr << e << ",";
        DEBUG std::cerr << std::endl;

        ex a, b, c, d, ee, f, g, m, n, p;
        // Three linear terms --> 1.1.1.3
        if (bvec[0].is_linear(x, a, b)
            and bvec[1].is_linear(x, c, d)
            and bvec[2].is_linear(x, ee, f))
                return rubi113(a, b, evec[0], c, d, evec[1], ee, f, evec[2], x);

        ex evm1 = exvec_max(ev1);
        ex evm2 = exvec_max(ev2);
        ex evm3 = exvec_max(ev3);
        // Two terms linear, one quadratic --> 1.2.1.3/4
        if (evm1.info(info_flags::posint)
            and evm2.info(info_flags::posint)
            and evm3.info(info_flags::posint)
            and (evm1+evm2+evm3).is_equal(_ex4)) {
                bool check = false;
                if (evm1.is_equal(_ex2)) {
                        check = bvec[2].is_linear(x, d, ee)
                                and bvec[1].is_linear(x, f, g)
                                and bvec[0].is_quadratic(x, a, b, c);
                        p = evec[0]; m = evec[2]; n = evec[1];
                }
                else if (evm2.is_equal(_ex2)) {
                        check = bvec[0].is_linear(x, d, ee)
                                and bvec[2].is_linear(x, f, g)
                                and bvec[1].is_quadratic(x, a, b, c);
                        p = evec[1]; m = evec[0]; n = evec[2];
                }
                else if (evm3.is_equal(_ex2)) {
                        check = bvec[0].is_linear(x, d, ee)
                                and bvec[1].is_linear(x, f, g)
                                and bvec[2].is_quadratic(x, a, b, c);
                        p = evec[2]; m = evec[0]; n = evec[1];
                }
                if (not check)
                        throw rubi_exception();
                return rubi214(d,ee,m,f,g,n,a,b,c,p,x);
        }
        //
        ex e1, e2, e3, e4, e5, e6;
        if (bvec[0].is_binomial(x, a, e1, b, e2)
            and bvec[1].is_binomial(x, c, e3, d, e4)
            and bvec[2].is_binomial(x, ee, e5, f, e6)
            and (e1*e2).is_zero()
            and (e3*e4).is_zero()
            and (e5*e6).is_zero()
            and (a*c*ee*b*d*f).is_zero()) {
                if (bvec[0].is_equal(x)) {
                        if (e4.is_zero()) {
                                e3.swap(e4);
                                c.swap(d);
                        }
                        if (e6.is_zero()) {
                                e5.swap(e6);
                                ee.swap(f);
                        }
                        if (not e4.is_equal(e6))
                                throw rubi_exception();
                        return rubi134(evec[0],c,d,e4,evec[1],ee,f,evec[2],x);
                }
                if (bvec[1].is_equal(x)) {
                        if (e2.is_zero()) {
                                e1.swap(e2);
                                a.swap(b);
                        }
                        if (e6.is_zero()) {
                                e5.swap(e6);
                                ee.swap(f);
                        }
                        if (not e2.is_equal(e6))
                                throw rubi_exception();
                        return rubi134(evec[1],a,b,e2,evec[0],ee,f,evec[2],x);
                }
                if (bvec[2].is_equal(x)) {
                        if (e4.is_zero()) {
                                e3.swap(e4);
                                c.swap(d);
                        }
                        if (e2.is_zero()) {
                                e1.swap(e2);
                                a.swap(b);
                        }
                        if (not e4.is_equal(e2))
                                throw rubi_exception();
                        return rubi134(evec[2],c,d,e4,evec[1],a,b,evec[0],x);
                }
        }
        throw rubi_exception();
}

// 1.1.1.1 (a+bx)^m
static ex rubi111(ex a, ex b, ex m, ex x)
{
        DEBUG std::cerr<<"r111: "<<a<<","<<b<<","<<m<<std::endl;
        if (b.is_one() and a.is_zero()) {
                if (m.is_minus_one())
                        return log(x);
                else
                        return power(x,m+1) / (m+1);
        }
        if (is_exactly_a<numeric>(m)) {
                if (ex_to<numeric>(m).is_minus_one())
                        return log(remove_content(a+b*x,x)) / b;
        }
        return power(a+b*x,m+1) / b / (m+1);
}

static ex _ellE(ex p, ex m)
{
        PyObject *pp = py_funcs.ex_to_pyExpression(p);
        PyObject *mp = py_funcs.ex_to_pyExpression(m);

        PyObject* mo = PyImport_ImportModule("sage.functions.special");
        if (mo == nullptr)
                py_error("Error importing functions.special");
        PyObject* ellfunc = PyObject_GetAttrString(mo, "elliptic_e");
        if (ellfunc == nullptr)
                py_error("Error getting elliptic_e attribute");

        PyObject* name = PyString_FromString(const_cast<char*>("__call__"));
        PyObject* pyresult = PyObject_CallMethodObjArgs(ellfunc, name, pp, mp, NULL);
        Py_DECREF(mo);
        Py_DECREF(name);
        Py_DECREF(ellfunc);
        if (pyresult == nullptr) {
                throw(std::runtime_error("_ellE() raised exception"));
        }
        if ( pyresult == Py_None ) {
                throw(std::runtime_error("elliptic_e::__call__ returned None"));
        }
        // convert output Expression to an ex
        ex eval_result = py_funcs.pyExpression_to_ex(pyresult);
        Py_DECREF(pyresult);
        if (PyErr_Occurred() != nullptr) {
                throw(std::runtime_error("_ellE(): python function (Expression_to_ex) raised exception"));
        }
        return eval_result;
}

static ex _ellF(ex p, ex m)
{
        PyObject *pp = py_funcs.ex_to_pyExpression(p);
        PyObject *mp = py_funcs.ex_to_pyExpression(m);

        PyObject* mo = PyImport_ImportModule("sage.functions.special");
        if (mo == nullptr)
                py_error("Error importing functions.special");
        PyObject* ellfunc = PyObject_GetAttrString(mo, "elliptic_f");
        if (ellfunc == nullptr)
                py_error("Error getting elliptic_f attribute");

        PyObject* name = PyString_FromString(const_cast<char*>("__call__"));
        PyObject* pyresult = PyObject_CallMethodObjArgs(ellfunc, name, pp, mp, NULL);
        Py_DECREF(mo);
        Py_DECREF(name);
        Py_DECREF(ellfunc);
        if (pyresult == nullptr) {
                throw(std::runtime_error("_ellF() raised exception"));
        }
        if ( pyresult == Py_None ) {
                throw(std::runtime_error("elliptic_f::__call__ returned None"));
        }
        // convert output Expression to an ex
        ex eval_result = py_funcs.pyExpression_to_ex(pyresult);
        Py_DECREF(pyresult);
        if (PyErr_Occurred() != nullptr) {
                throw(std::runtime_error("_ellE(): python function (Expression_to_ex) raised exception"));
        }
        return eval_result;
}

static ex _2F1(ex a, ex b, ex c, ex x)
{
        exvector avec, bvec;
        avec.push_back(a);
        avec.push_back(b);
        bvec.push_back(c);
        PyObject *lista = py_funcs.exvector_to_PyTuple(avec);
        PyObject *listb = py_funcs.exvector_to_PyTuple(bvec);
        PyObject *z = py_funcs.ex_to_pyExpression(x);

        PyObject* m = PyImport_ImportModule("sage.functions.hypergeometric");
        if (m == nullptr)
                py_error("Error importing hypergeometric");
        PyObject* hypfunc = PyObject_GetAttrString(m, "hypergeometric");
        if (hypfunc == nullptr)
                py_error("Error getting hypergeometric attribute");

        PyObject* name = PyString_FromString(const_cast<char*>("__call__"));
        PyObject* pyresult = PyObject_CallMethodObjArgs(hypfunc, name, lista, listb, z, NULL);
        Py_DECREF(m);
        Py_DECREF(name);
        Py_DECREF(hypfunc);
        if (pyresult == nullptr) {
                throw(std::runtime_error("numeric::hypergeometric_pFq(): python function hypergeometric::__call__ raised exception"));
        }
        if ( pyresult == Py_None ) {
                throw(std::runtime_error("numeric::hypergeometric_pFq(): python function hypergeometric::__call__ returned None"));
        }
        // convert output Expression to an ex
        ex eval_result = py_funcs.pyExpression_to_ex(pyresult);
        Py_DECREF(pyresult);
        if (PyErr_Occurred() != nullptr) {
                throw(std::runtime_error("numeric::hypergeometric_pFq(): python function (Expression_to_ex) raised exception"));
        }
        return eval_result;
}

static bool int_linear(ex mm, ex nn)
{
        if (not is_exactly_a<numeric>(mm)
            or not is_exactly_a<numeric>(nn))
                return false;
        const numeric& m = ex_to<numeric>(mm);
        const numeric& n = ex_to<numeric>(nn);
        return (m.info(info_flags::posint)
                or n.info(info_flags::posint)
                or ((m*3).is_integer() and (n*3).is_integer())
                or ((m*4).is_integer() and (n*4).is_integer())
                or ((m*2).is_integer() and (n*6).is_integer())
                or ((m*6).is_integer() and (n*2).is_integer())
                or (m+n+1).info(info_flags::negint)
                or ((m+n).is_integer()
                    and (m.is_rational() or n.is_rational())));
}

// 1.1.1.2 (a+bx)^m * (c+dx)^n
static ex rubi112(ex a, ex b, ex m, ex c, ex d, ex n, ex x)
{
        DEBUG std::cerr<<"r112: "<<a<<","<<b<<","<<m<<","<<c<<","<<d<<","<<n<<std::endl;
        if (m.is_zero())
                return rubi111(c, d, n, x);
        if (n.is_zero())
                return rubi111(a, b, m, x);
        if (a.is_zero() and c.is_zero()) {
                if ((m+n+1).is_zero())
                        return power(d/b,-m-1)/b * log(x);
                return x*power(b*x,m)*power(d*x,n)/(m+n+1);
        }
/*        if ((m.info(info_flags::posint) and not a.is_zero())
            or (n.info(info_flags::posint) and not c.is_zero())) {
                const ex& t = mul(power(a+b*x,m), power(c+d*x,n));
                DEBUG std::cerr<<"r112 expanded"<<std::endl;
                return rubi(t.expand(), x);
        }
        if (not ((a.is_zero() and m.is_positive())
                 or (c.is_zero() and n.is_positive()))
            and ((m.is_integer() and (m+1).is_negative())
                or (n.is_integer() and (n+1).is_negative()))) {
                const ex& t = mul(power(a+b*x,m), power(c+d*x,n));
                const ex& pf = parfrac(t, x);
                if (not pf.is_equal(t)) {
                        DEBUG std::cerr<<"parfrac1 "<<pf<<std::endl;
                        return rubi(pf, x);
                }
        }*/

        ex bcmad = (b*c - a*d).expand();
        if (bcmad.is_zero()) {
                // some 9.1 rules here
                if (m.is_integer()
                    and (not n.is_integer()
                         or (c+d*x).treesize() < (a+b*x).treesize()))
                        return dist(power(b/d,m), rubi111(c,d,m+n,x));
                if (n.is_integer()
                    and (not m.is_integer()
                         or (c+d*x).treesize() > (a+b*x).treesize()))
                        return dist(power(d/b,n), rubi111(a,b,m+n,x));
                if (not m.is_integer()
                    and not n.is_integer()
                    and not (b/d).is_positive())
                        return dist(power(a+b*x,m)/power(c+d*x,m), rubi111(c,d,m+n,x));
                if ((a/c-_ex1).is_positive())
                        return rubi(power(a/c, m) * power(c+d*x, m+n), x);
                return rubi(power(c/a, n) * power(a+b*x, m+n), x);
        }

        ex bcpad = (b*c + a*d).expand();
        if (m.is_equal(n)) {
                // Rule 1.1 simplified
                if (m.is_equal(_ex_1))
                        return log(b*x + a)/bcmad - log(d*x + c)/bcmad;
                // 2.1
                if (bcpad.is_zero()) {
                        if (is_exactly_a<numeric>(m)
                            and ex_to<numeric>(m).denom().is_equal(*_num2_p)) {
                                numeric mnum = ex_to<numeric>(m);
                                // Rule 2.1.1
                                ex g1 = gcd(a,b);
                                ex g2 = gcd(c,d);
                                if (mnum.is_positive())
                                        return x*power(g1*g2,m)*power(a/g1+b/g1*x, m)*power(c/g2+d/g2*x, m)/(_ex2*m+1) + dist(_ex2*a*c*m/(_ex2*m+1), rubi112(a, b, m-_ex1, c, d, m-_ex1, x));
                                // Rule 2.1.2.1
                                if (mnum.is_equal((*_num_3_p)/(*_num2_p)))
                                        return x/(a*c*sqrt(a+b*x)*sqrt(c+d*x));
                                // Rule 2.1.4.1
                                if (mnum.is_equal(*_num_1_2_p)) {
                                        if ((a+c).is_zero()) {
                                                if (a.info(info_flags::positive))
                                                        return acosh(b*x/a) / b;
                                                if (c.info(info_flags::positive))
                                                        return acosh(d*x/c) / d;
                                        }
                                }
                                // Rule 2.1.2.2
                                else
                                        return _ex0-x*power(a+b*x, m+_ex1)*power(c+d*x,m+_ex1)/_ex2/a/c/(m+_ex1) + dist((_ex2*m+_ex3)/_ex2/a/c/(m+_ex1), rubi112(a, b, m+_ex1, c, d, m+_ex1, x));
                        }
                        // Fallback Rule 2.1.3
                        if (m.info(info_flags::integer)
                            or (a.info(info_flags::positive)
                                and c.info(info_flags::positive)))
                                return rubi131(a*c, b*d, _ex2, m, x);
                        // Fallback Rule 2.1.4.2
                        if (m.is_equal(_ex_1_2)) {
                                if (b.is_negative_or_minus())
                                        return _ex2*rubi131(d,-b,_ex2,_ex_1,x).subs(x == sqrt(c+d*x)/sqrt(a+b*x));
                                return _ex2*rubi131(b,-d,_ex2,_ex_1,x).subs(x == sqrt(a+b*x)/sqrt(c+d*x));
                        }
                        // 2.1.5
                        if (not (_ex2*m).is_integer()) {
                                if (is_exactly_a<numeric>(m)) {
                                        numeric fpm = ex_to<numeric>(m).frac();
                                        return dist(power(a+b*x,fpm) * power(c+d*x,fpm) / power(a*c+b*d*x*x,fpm), rubi131(a*c, b*d, _ex2, m, x));
                                }
                                return dist(power(a+b*x,m) * power(c+d*x,m) / power(a*c+b*d*x*x,m), rubi131(a*c, b*d, _ex2, m, x));
                        }
                }
                if (m.is_equal(_ex_1_2)) {
                        // Rule 7.1.1.1
                        if ((b+d).is_zero()
                            and (a+c).is_positive())
                                return rubi211(a*c, -b*(a-c), -b*b, _ex_1_2, x);
                        // 7.1.1.2 symmetry
                        if (bcmad.is_positive()
                            and b.is_positive())
                                return _ex2/power(b,_ex1_2) * rubi131(bcmad,d,_ex2,_ex_1_2,x).subs(x==power(a+b*x,_ex1_2));
                        if (bcmad.is_negative()
                            and d.is_positive())
                                return _ex2/power(d,_ex1_2) * rubi131(_ex0-bcmad,b,_ex2,_ex_1_2,x).subs(x==power(c+d*x,_ex1_2));
                        // 7.1.1.3
                        if (not bcmad.is_zero()
                            and (b-d).is_zero())
                                return _ex2/b * rubi131(c-a,_ex1,_ex2,_ex_1_2,x).subs(x==power(a+b*x,_ex1_2));
                        // 7.1.1.4
                        if (is_exactly_a<numeric>(d))
                                return _ex2*rubi131(d,-b,_ex2,_ex_1,x).subs(x == power(c+d*x,_ex1_2)/power(a+b*x,_ex1_2));
                        return _ex2*rubi131(b,-d,_ex2,_ex_1,x).subs(x == power(a+b*x,_ex1_2)/power(c+d*x,_ex1_2));
                }
                if (m.is_negative() and (m+1).is_positive()) {
                        // 7.1.2
                        if (m.denom().is_equal(*_num3_p)
                            or m.denom().is_equal(*_num4_p))
                                return power(a+b*x,m)*power(c+d*x,m)/power(a*c+bcpad*x+b*d*x*x,m) * rubi211(a*c,bcpad,b*d,m,x);
                }
        }
        else if ((m-n).is_negative()) {
                // make m > n
                m.swap(n);
                a.swap(c);
                b.swap(d);
                bcmad = _ex0 - bcmad;
        }

        // Rule 1.2 - neither m nor n can be -1
        if ((m+n+_ex2).is_zero())
                return power(a+b*x, m+_ex1) * power(c+d*x, n+_ex1) / bcmad / (m+_ex1);

        // we know m>n
        if (bcpad.is_zero()) {
                if (is_exactly_a<numeric>(m) 
                    and is_exactly_a<numeric>(n) 
                    and ex_to<numeric>(m).denom().is_equal(*_num2_p)
                    and ex_to<numeric>(n).denom().is_equal(*_num2_p)) {
                        // Rule 2.4
                        if (m.info(info_flags::positive)
                            and n.info(info_flags::positive))
                                return power(c+d*x,n+1) * power(a+b*x, m) / d / (m+n+1) + dist(_ex2*a*m/(m+n+1), rubi112(c, d, n, a, b, m-1, x));
                        // Rule 2.5
                        if ((m+_ex1).info(info_flags::negative)) // not m == -1/2 !
                                return _ex0-power(c+d*x, n+_ex1) * power(a+b*x, m+_ex1) / _ex2 / b / c / (n+_ex1) + (m+n+_ex2)/_ex2/c/(n+_ex1) * rubi112(c, d, n+_ex1, a, b, m, x);
                }
                ex _ex5_4 = _ex5 / _ex4;
                if ((b*d/a/c).info(info_flags::positive)) {
                        // true for conjugates
                        // Rule 2.2
                        if (m.is_equal(_ex5_4)
                            and n.is_equal(_ex1_4))
                                return _ex2 / b / power(a+b*x,_ex1_4) / power(c+d*x,_ex1_4) + bcmad / _ex2 / b * rubi112(a,b,_ex5_4,c,d,_ex5_4,x);
                        // Rule 2.3
                        if (m.is_equal(_ex9/_ex4)
                            and n.is_equal(_ex1_4))
                                return _ex4/_ex5 / b / power(a+b*x,_ex5_4) / power(c+d*x,_ex5_4) - d / _ex5 / b * rubi112(a,b,_ex5_4,c,d,_ex5_4,x);
                }
        }
        
        // Rule 3
        bool e1p = is_exactly_a<numeric>(m) and m.info(info_flags::posint);
        bool e2p = is_exactly_a<numeric>(n) and n.info(info_flags::posint);
        if (e1p and e2p) {
                if (a.is_zero() or c.is_zero()) {
                        if (a.is_zero()) {
                                a.swap(c);
                                b.swap(d);
                                m.swap(n);
                                bcmad = _ex0-bcmad;
                        }
                        if ((_ex7*m+_ex4*n+_ex4).is_negative()
                            or (n.is_positive()
                                and (m-n-_ex1).is_positive())) {
                                        symbol t1, t2;
        DEBUG std::cerr<<"r112-3a: "<<a<<","<<b<<","<<m<<","<<c<<","<<d<<","<<n<<std::endl;
                                        return rubi((t2*power((t1-a)*d/b,n)).expand().subs(t1==a+b*x).subs(t2==power(a+b*x,m)),x);
                        }
                }
        }
        if (e1p or e2p) {
                if (not e1p) {
                        a.swap(c);
                        b.swap(d);
                        m.swap(n);
                        bcmad = _ex0-bcmad;
                }
                if ((c.is_zero()
                    and (_ex7*m+_ex4*n+_ex4).is_negative())
                    or (m+n+_ex2).is_positive()
                    or (_ex9*m+_ex5*n+_ex5).is_negative()
                    or not (is_exactly_a<numeric>(n) and n.is_integer())) {
                        const ex& t = mul(power(a+b*x,m), power(c+d*x,n));
                        const ex& pf = parfrac(t, x);
                        DEBUG std::cerr<<"parfrac2 "<<pf<<std::endl;
                        if (not pf.is_equal(t))
                                return rubi(pf, x);
              
        DEBUG std::cerr<<"r112-3c: "<<a<<","<<b<<","<<m<<","<<c<<","<<d<<","<<n<<std::endl;
                        symbol t1,t2;          
                        if ((c.is_zero()
                            and is_exactly_a<numeric>(m)
                            and m.info(info_flags::posint))
                         or (a.is_zero()
                            and is_exactly_a<numeric>(n)
                            and n.info(info_flags::posint)))
                                return rubi(t.expand(), x);
                        else
                                return rubi((t2*power((t1-c)*b/d,m)).expand().subs(t1==c+d*x).subs(t2==power(c+d*x,n)),x);
                }
        }

        // 4
        const ex _ex3_2 = _ex3/_ex2;
        const ex _ex_3_2 = _ex_3/_ex2;
        const ex _ex2_3 = _ex2/_ex3;
        const ex _ex_2_3 = _ex_2/_ex3;
        if (m.is_minus_one() or n.is_minus_one()) { // we know m>n
                if (n.is_minus_one()) {
                        a.swap(c);
                        b.swap(d);
                        m.swap(n);
                        bcmad = _ex0-bcmad;
                }
                // Rule 4.1
                if (n.is_positive())
                        return power(c+d*x,n)/b/n + dist(bcmad/b, rubi112(a,b,m,c,d,n-1,x));
                // Rule 4.2
                if ((n+_ex1).is_negative()) // we know m=-1
                        return _ex0 - power(c+d*x,n+_ex1)/(n+_ex1)/bcmad + b/bcmad * rubi112(a,b,m,c,d,n+1,x);
                const ex bcmad_b = bcmad / b;
                ex q;
                if (n.is_equal(_ex_1_3)
                    or n.is_equal(_ex_2_3)) {
                        if (bcmad_b.is_negative_or_minus()) {
                                if (is_exactly_a<numeric>(bcmad_b))
                                        q = power(_ex0-bcmad_b, _ex1_3);
                                else
                                        q = power(_ex0-bcmad,_ex1_3)/power(b,_ex1_3);
                        }
                        else {
                                if (is_exactly_a<numeric>(bcmad_b))
                                        q = power(bcmad_b, _ex1_3);
                                else
                                        q = power(bcmad,_ex1_3)/power(b,_ex1_3);
                        }
                }
                if (n.is_equal(_ex_1_3)) { // 4.3.1
                        if (bcmad_b.is_negative_or_minus())
                                return log(remove_content(a+b*x,x))/_ex2/b/q - _ex3_2/b/q * rubi111(q,_ex1,_ex_1,x).subs(x==power(c+d*x,_ex1_3)) + _ex3_2/b * rubi211(q*q,-q,_ex1,_ex_1,x).subs(x==power(c+d*x,_ex1_3));
                        return _ex0-log(remove_content(a+b*x,x))/_ex2/b/q - _ex3_2/b/q * rubi111(q,_ex_1,_ex_1,x).subs(x==power(c+d*x,_ex1_3)) + _ex3_2/b * rubi211(q*q,q,_ex1,_ex_1,x).subs(x==power(c+d*x,_ex1_3));
                }
                if (n.is_equal(_ex_2_3)) { // 4.3.2
                        if (bcmad_b.is_negative_or_minus())
                                return _ex0-log(remove_content(a+b*x,x))/_ex2/b/q/q + _ex3_2/b/q/q * rubi111(q,_ex1,_ex_1,x).subs(x==power(c+d*x,_ex1_3)) + _ex3_2/b/q * rubi211(q*q,-q,_ex1,_ex_1,x).subs(x==power(c+d*x,_ex1_3));
                        return _ex0-log(remove_content(a+b*x,x))/_ex2/b/q/q - _ex3_2/b/q/q * rubi111(q,_ex_1,_ex_1,x).subs(x==power(c+d*x,_ex1_3)) - _ex3_2/b/q * rubi211(q*q,q,_ex1,_ex_1,x).subs(x==power(c+d*x,_ex1_3));
                }
                // Rule 4.3.3
                if (is_exactly_a<numeric>(n)
                    and n.is_negative()
                    and ((n+_ex1).is_positive())) {
                        numeric p = ex_to<numeric>(n).denom();
                        return ex(p) * rubi132(_ex1, (n+1)*p-1, _ex0-bcmad, b, p, _ex_1, x).subs(x == power(c+d*x,p.inverse()));
                }

                // Fallback Rules H.4
                if (not (is_exactly_a<numeric>(n) and n.is_integer())) {
//                    if (bcmad.is_zero())
//                        return _ex0-power(c+d*x, n+_ex1) / (c*(n+_ex1)) * _2F1(1, n+_ex1, n+_ex2, _ex1+d*x/c);
                    return _ex0-power(c+d*x, n+1)/(n+1)/bcmad * _2F1(1, n+1, n+2, b*(c+d*x)/bcmad);
                }
        }
        
        // 5+6
        if (int_linear(m,n)) {
                // Rule 5.1
                if (((m+1).is_negative() and n.is_positive())
                         or ((n+1).is_negative() and m.is_positive())) {
                        if ((n+1).is_negative()) {
                                m.swap(n);
                                a.swap(c);
                                b.swap(d);
                                bcmad = _ex0 - bcmad;
                        }
                        if (not (n.is_integer()
                                 and not m.is_integer())
                            and not (((m+n+2).is_integer()
                                        and ((m+n+2).is_negative()
                                           or (m+n+2).is_zero()))
                                      and ((is_exactly_a<numeric>(m)
                                            and ex_to<numeric>(m).is_mpq())
                                           or (n+n+m+1).is_positive()
                                           or (n+n+m+1).is_zero())))
                                return power(a+b*x,m+1)*power(c+d*x,n)/b/(m+1) - dist(d*n/b/(m+1), rubi112(a,b,m+1,c,d,n-1,x));
                }
                // Rule 5.2
                if ((m+1).is_negative()
                    and not ((n+1).is_negative()
                             and (a.is_zero()
                                  or (not c.is_zero()
                                      and (m-n).is_negative()
                                      and n.is_integer()))))
                        return power(a+b*x,m+1)*power(c+d*x,n+1)/bcmad/(m+1) - dist(d*(m+n+2)/bcmad/(m+1), rubi112(a,b,m+1,c,d,n,x));
                if ((n+1).is_negative()
                    and not ((m+1).is_negative()
                             and (c.is_zero()
                                  or (not a.is_zero()
                                      and (m-n).is_positive()
                                      and m.is_integer()))))
                        return -power(a+b*x,m+1)*power(c+d*x,n+1)/bcmad/(n+1) + dist(b*(m+n+2)/bcmad/(n+1), rubi112(a,b,m,c,d,n+1,x));
                

                // Rule 6
                if (not (m+n+_ex1).is_zero()
                    and n.is_positive()
                    and not (m.info(info_flags::posint)
                             and (not n.is_integer()
                                  or (m.is_positive() and (m-n).is_negative()))))
                        return power(a+b*x,m+1)*power(c+d*x,n)/b/(m+n+1) + dist(n*bcmad/b/(m+n+1), rubi112(a,b,m,c,d,n-1,x));
                if (not (m+n+_ex1).is_zero()
                    and m.is_positive()
                    and not (n.info(info_flags::posint)
                             and (not m.is_integer()
                                  or (n.is_positive() and (m-n).is_positive()))))
                        return power(a+b*x,m)*power(c+d*x,n+1)/d/(m+n+1) - dist(m*bcmad/d/(m+n+1), rubi112(a,b,m-1,c,d,n,x));
        }

        // Rule 7
        // for 7.1 see under m==n above
        if (m.is_negative() and (m+1).is_positive()
            and n.is_negative() and (n+1).is_positive()) {
                if ((m-n).is_negative()) { // make m>n
                        m.swap(n);
                        a.swap(c);
                        b.swap(d);
                        bcmad = _ex0 - bcmad;
                }
                // 7.2.1
                if (m.is_equal(_ex_1_3) and n.is_equal(_ex_2_3)) {
                        ex exr3 = power(_ex3,_ex1_2);
                        if ((d/b).is_positive()) {
                                ex q = power(d/b, _ex1_3);
                                return _ex0-exr3*q/d * atan(_ex2*q*power(a+b*x,_ex1_3)/exr3/power(c+d*x,_ex1_3)+1/exr3) - q/_ex2/d*log(c+d*x) - _ex3_2*q/d*log(q*power(a+b*x,_ex1_3)/power(c+d*x,_ex1_3)-1);
                        }
                        else {
                                ex q = power(-d/b, _ex1_3);
                                return exr3*q/d * atan(_ex0-_ex2*q*power(a+b*x,_ex1_3)/exr3/power(c+d*x,_ex1_3)+1/exr3) + q/_ex2/d*log(c+d*x) + _ex3_2*q/d*log(q*power(a+b*x,_ex1_3)/power(c+d*x,_ex1_3)+1);
                        }
                }
                // 7.2.2
                if ((m+n+1).is_zero()) {
                        ex p = m.denom();
                        return p * rubi132(_ex1,p*(m+1)-1,b,_ex0-d,p,_ex_1,x).subs(x == power(a+b*x,_ex1/p)/power(c+d*x,_ex1/p));
                }
                // 7.3
                if (n.denom() > m.denom()) { // make denom(n)<denom(m)
                        m.swap(n);
                        a.swap(c);
                        b.swap(d);
                        bcmad = _ex0 - bcmad;
                }
                ex p = m.denom();
                return p/b * rubi132(_ex1,p*(m+1)-1,c-a*d/b,d/b,p,n,x).subs(x == power(a+b*x,_ex1/p));
        }
        // 8
        if ((m+n+_ex2).is_negative()
            and (m+n+_ex2).is_integer()) {
                if (m.is_equal(_ex_1)
                    or m.is_positive()) {
                        m.swap(n);
                        a.swap(c);
                        b.swap(d);
                        bcmad = _ex0 - bcmad;
                }
                if (not m.is_equal(_ex_1))
                        return power(a+b*x,m+1)*power(c+d*x,n+1)/bcmad/(m+1) - dist(d*(m+n+2)/bcmad/(m+1), rubi112(a,b,m+1,c,d,n,x));
        }
        // H.1
        if (a.is_zero() or c.is_zero()) {
                if (c.is_zero()) {
                        m.swap(n);
                        a.swap(c);
                        b.swap(d);
                        bcmad = _ex0 - bcmad;
                }
                // H.1.1
DEBUG std::cerr<<"r112H1: "<<a<<","<<b<<","<<m<<","<<c<<","<<d<<","<<n<<std::endl;
                if (not m.is_num_integer()
                    and (n.is_num_integer()
                         or (c.is_positive()
                            and not (n.is_equal(_ex_1_2)
                                     and (c*c-d*d).is_zero()
                                     and (-d/(b*c)).is_positive()))))
                        return power(c,n)*power(b*x,m+1)/b/(m+1) * _2F1(-n,m+1,m+2,-d*x/c);
                // H.1.2
                if (not n.is_num_integer()
                    and (m.is_num_integer()
                         or (-d/b/c).is_positive()))
                        return power(c+d*x,n+1)/d/(n+1)/power(-d/b/c,m) * _2F1(n+1,-m,n+2,1+d*x/c);
                // H.1.3
                if (not m.is_num_integer() and not n.is_num_integer()
                    and not c.is_positive()
                    and not (d/b/c).is_negative()) {
                        // H.1.3.1
                        if ((m.is_num_fraction()
                            and not (n.is_equal(_ex_1_2)
                                    and (c*c-d*d).is_zero()))
                            or not n.is_num_fraction()) {
                                ex fn,in;
                                if (is_exactly_a<numeric>(n)) {
                                        fn = ex_to<numeric>(n).frac();
                                        in = ex_to<numeric>(n).floor();
                                }
                                else {
                                        fn = n;
                                        in = _ex0;
                                }
                                return power(c,in)*power(c+d*x,fn)/power(1+d/c*x,fn) * rubi112(_ex0,b,m,_ex1,d/c,n,x);
                        }
                        // H.1.3.2
                        ex fm,im;
                        if (is_exactly_a<numeric>(m)) {
                                fm = ex_to<numeric>(m).frac();
                                im = ex_to<numeric>(m).floor();
                        }
                        else {
                                fm = m;
                                im = _ex0;
                        }
                        DEBUG std::cerr<<"r112H13 fac: "<<power(-b*c/d,im)*power(b*x,fm)*power(-x,_ex0-fm)*power(d/c,_ex0-fm)<<std::endl;
                        return dist(power(-b*c/d,im)*power(b*x,fm)*power(-x,_ex0-fm)*power(d/c,_ex0-fm), rubi(power(-d*x/c,m)*power(c+d*x,n),x));
                }
        }
        // H.2.2.1
        if (not m.is_integer()
            and n.is_integer())
                return power(bcmad,n)*power(a+b*x,m+1) * _2F1(m+1,_ex0-n,m+2,_ex0-d*(a+b*x)/bcmad)/power(b,n+1)/(m+1);
        if (not n.is_integer()
            and m.is_integer())
                return power(-bcmad,m)*power(c+d*x,n+1) * _2F1(n+1,_ex0-m,n+2,b*(c+d*x)/bcmad)/power(d,m+1)/(n+1);
        if (not n.is_integer()
            and not m.is_integer()) {

        }
        throw rubi_exception();
}

// 1.1.1.3 (a+bx)^m * (c+dx)^n * (e+fx)^p
static ex rubi113(ex a, ex b, ex m, ex c, ex d, ex n, ex e, ex f, ex p, ex x)
{
        DEBUG std::cerr<<"r113: "<<a<<","<<b<<","<<m<<","<<c<<","<<d<<","<<n<<","<<e<<","<<f<<","<<p<<std::endl;
        // 2?
        if ((m.is_one() and not a.is_zero())
            or (n.is_one() and not c.is_zero())
            or (p.is_one() and not e.is_zero()))
                return rubi(expand(power(a+b*x,m)*power(c+d*x,n)*power(e+f*x,p)), x);
        
        if (m.is_minus_one() or n.is_minus_one() or p.is_minus_one()) {
                if (not p.is_minus_one() and m.is_minus_one()) {
                        a.swap(e);
                        b.swap(f);
                        m.swap(p);
                }
                else if (not p.is_minus_one() and n.is_minus_one()) {
                        c.swap(e);
                        d.swap(f);
                        n.swap(p);
                }
                // 9.3
                if ((m+n+1).is_zero()
                    and (((m+1).is_positive() and m.is_negative())
                        or ((n+1).is_positive() and n.is_negative()))) {
                        if (not ((m+1).is_positive() and m.is_negative())) {
                                a.swap(c);
                                b.swap(d);
                                m.swap(n);
                        }
                        ex q = m.denom();
                        return q*rubi132(_ex1,q*(m+1)-1,b*e-a*f,_ex0-d*e+c*f,q,_ex_1,x).subs(x==power(a+b*x,_ex1/q)/power(c+d*x,_ex1/q));
                }
                // 17
                if ((m+n+1).info(info_flags::posint)
                    and (m.is_positive() or n.is_positive())) {
                        if (not m.is_positive()) {
                                a.swap(c);
                                b.swap(d);
                                m.swap(n);
                        }
                        return b/f*rubi112(a,b,m-1,c,d,n,x) - (b*e-a*f)/f*rubi113(a,b,m-1,c,d,n,e,f,_ex_1,x);
                }
        }

        // 1.1.3.H.1/2
        if ((m+n+p+2).is_zero()) {
                if (m.info(info_flags::integer)
                    and m.info(info_flags::negative)) {
                        m.swap(n);
                        a.swap(c);
                        b.swap(d);
                }
                else if (p.info(info_flags::integer)
                    and p.info(info_flags::negative)) {
                        p.swap(n);
                        e.swap(c);
                        f.swap(d);
                }
                if (n.info(info_flags::integer)
                    and n.info(info_flags::negative)) {
                        return power(b*c-a*d,n) * power(a+b*x,m+1) / (m+1) / power(b*e-a*f,n+1) / power(e+f*x,m+1) * _2F1(m+1, -n, m+2, -(d*e-c*f)*(a+b*x)/(b*c-a*d)/(e+f*x));
                }
                return power(a+b*x,m+1) * power(c+d*x,n) * power(e+f*x,p+1) / (m+1) / (b*e-a*f) * power((b*e-a*f)*(c+d*x)/(b*c-a*d)/(e+f*x),-n) * _2F1(m+1, -n, m+2, -(d*e-c*f)*(a+b*x)/(b*c-a*d)/(e+f*x));
        }
        // 1.1.3.A.1
        if (c.is_zero()) {
                c.swap(a);
                d.swap(b);
                n.swap(m);
        }
        if (e.is_zero()) {
                e.swap(a);
                f.swap(b);
                p.swap(m);
        }
        if (a.is_zero()) {
                if (c.info(info_flags::positive)
                    and not (m+1).is_zero()
                    and (p.info(info_flags::integer)
                         or e.info(info_flags::positive)))
                        return power(c,n)*power(e,p)*power(b*x,m+1)/b/(m+1) * appell_F1(m+1, -n, -p, m+2, -d/c*x, -f/e*x);
                if (e.info(info_flags::positive)
                    and not (m+1).is_zero()
                    and (n.info(info_flags::integer)
                         or c.info(info_flags::positive)))
                        return power(e,p)*power(c,n)*power(b*x,m+1)/b/(m+1) * appell_F1(m+1, -p, -n, m+2, -f/e*x, -d/c*x);
        }
        // 1.1.3.A.2
        if (m.info(info_flags::integer)) {
                m.swap(p);
                a.swap(e);
                b.swap(f);
        }
        else if (n.info(info_flags::integer)) {
                n.swap(p);
                c.swap(e);
                d.swap(f);
        }
        if (p.info(info_flags::integer))
                return power(b*e-a*f,p) * power(a+b*x,m+1) / power(b,p+1) / (m+1) / power(b / (b*c-a*d), n) * appell_F1(m+1, -n, -p, m+2, -d*(a+b*x)/(b*c-a*d), -f*(a+b*x)/(b*e-a*f));
        // 1.1.3.A.3
        return power(a+b*x,m+1) / b / (m+1) / power(b/(b*c-a*d),n) / power(b/(b*e-a*f),p) * appell_F1(m+1, -n, -p, m+2, -d*(a+b*x)/(b*c-a*d), -f*(a+b*x)/(b*e-a*f));
}

static void sqrt_nd(ex a, ex& numer, ex& denom)
{
        ex r = ex(power(a, _ex1_2)).numer_denom();
        numer = r.op(0);
        denom = r.op(1);
        DEBUG std::cerr<<"sqrt_nd: "<<a<<" = "<<numer<<" / "<<denom<<std::endl;
}

// 1.1.3.1 (a+bx^n)^p
static ex rubi131(ex ae, ex be, ex ne, ex pe, ex x)
{
        DEBUG std::cerr<<"r131: "<<ae<<","<<be<<","<<ne<<","<<pe<<std::endl;
        if (ae.is_zero()) {
                if ((ne*pe).is_minus_one())
                        return power(be,pe) * log(x);
                else
                        return x * power(be*power(x,ne),pe) / (ne*pe+1);
        }
        if (ne.is_one())
                return rubi111(ae, be, pe, x);
        if (pe.is_one())
                return rubi(ae + be*power(x,ne), x);
        ex _ex3_2 = _ex3/_ex2;
        ex _ex3_4 = _ex3/_ex4;
        ex _ex_3_4 = _ex_3/_ex4;
        // Rule 0
        if (is_exactly_a<numeric>(pe)
            and ae.is_zero()) {
                numeric p = ex_to<numeric>(pe);
                return rubi(power(be,p.floor()) * power(be*power(x,ne),p.frac()) / power(x, ne*p.frac()), x);
        }

        // Rule 1.1
        ex discr = pe + _ex1/ne + _ex1;
        if (discr.is_zero())
                return x * power(ae + be*power(x,ne), pe+_ex1) / ae;
        if (pe.info(info_flags::integer)) {
                // Rule 3
                if (ne.info(info_flags::negative))
                        return rubi(power(x, ne*pe) * power(ae + be*power(x,_ex0-ne), pe), x);
                // Rule 4.1.1.1
                if (ne.info(info_flags::integer)
                    and ne.info(info_flags::positive)
                    and pe.info(info_flags::positive))
                        return rubi(expand(power(ae + be*power(x,ne), pe)), x);
        }

        // 4.1.2
        if (ne.is_equal(_ex2)) {
                if (pe.is_equal(_ex_5/_ex4)
                    and (be/ae).info(info_flags::positive)) {
                        // Rule 4.1.2.1.1
                        if (ae.info(info_flags::positive))
                                return _ex2/power(ae,_ex5/_ex4)/power(be/ae,_ex1_2) * _ellE(_ex1_2 * atan(x*power(be/ae,_ex1_2)), _ex2);
                        // Rule 4.1.2.1.2
                        else
                                return power(1+be*x*x/ae,_ex1_4)/ae/power(ae+be*x*x,_ex1_4) * rubi131(_ex1, be/ae, _ex2, _ex_5/_ex4, x);
                }
                // Rule 4.1.2.2
                ex _ex2_3 = _ex2/_ex3;
                if (pe.is_equal(_ex_7/_ex6))
                        return power(power(ae+be*x*x,_ex2_3) * power(ae/(ae+be*x*x),_ex2_3), _ex_1) * rubi131(_ex1, -be, _ex2, _ex_1/_ex3, x).subs(x == x/sqrt(ae+be*x*x));
        }
        // 4.1.2.3
        ex e1 = pe + _ex1/ne;
        if (ne.info(info_flags::posint)
            and (pe+1).is_negative()
            and ((_ex2*pe).is_integer()
                 or (ne.is_equal(_ex2)
                     and ((_ex3*pe).is_integer()
                          or (_ex4*pe).is_integer()))
                 or (is_exactly_a<numeric>(e1)
                     and is_exactly_a<numeric>(pe)
                     and ex_to<numeric>(e1).denom() < ex_to<numeric>(pe).denom())))
                 return _ex0-x*power(ae+be*power(x,ne),pe+1)/ae/ne/(pe+1) + dist((ne*(pe+1)+1)/ae/ne/(pe+1), rubi131(ae,be,ne,pe+1,x));
        // 4.1.3
        if (pe.is_equal(_ex_1)) {
                // 4.1.3.2
                // presumably this can be simplified
                if (ne.is_equal(_ex2)) {
                        if (ae.is_positive()) {
                                if (be.is_positive())
                                        return _ex1/(sqrt(ae)*sqrt(be)) * atan(sqrt(be)*x/sqrt(ae));
                                else if (be.is_negative_or_minus())
                                        return _ex1/(sqrt(ae)*sqrt(-be)) * atanh(sqrt(-be)*x/sqrt(ae));
                        }
                        if (ae.is_negative_or_minus()) {
                                if (be.is_positive())
                                        return _ex_1/(sqrt(-ae)*sqrt(be)) * atanh(sqrt(be)*x/sqrt(-ae));
                                else if (be.is_negative_or_minus())
                                        return _ex_1/(sqrt(-ae)*sqrt(-be)) * atan(sqrt(-be)*x/sqrt(-ae));
                        }
                        ex adb = ae/be;
                        if ((adb).is_positive())
                                return sqrt(adb)/ae * atan(x/sqrt(adb));
                        if ((is_exactly_a<mul>(adb)
                             and ex_to<mul>(adb).get_overall_coeff().is_negative())
                            or adb.is_negative())
                                return sqrt(-adb)/ae * atanh(x/sqrt(-adb));
                        
                        return sqrt(adb)/ae * atan(x/sqrt(adb));
                }
                // Rule 4.1.3.1.1
                if (ne.is_equal(_ex3)) {
                        ex a13 = power(ae, _ex1_3);
                        ex a23 = power(ae, _ex2/_ex3);
                        ex b13 = power(be, _ex1_3);
                        ex b23 = power(be, _ex2/_ex3);
                        return _ex1_3/a23 * rubi111(a13,b13,_ex_1,x) + _ex1_3/a23 * rubi212(2*a13, -b13, _ex1, a23, -a13*b13, b23, _ex_1, x);
                }
                // Rules 4.1.3.1.2 not necessary because we expand cos(k*pi/5)
                ex nm32 = (ne-_ex3)/_ex2;
                // 4.1.3.1.3
                if (is_exactly_a<numeric>(nm32)
                    and nm32.info(info_flags::positive)
                    and nm32.info(info_flags::integer)) {
                        ex r,s;
                        numeric nm = ex_to<numeric>(nm32);
                        ex aob = ae/be;
                        if (aob.info(info_flags::negative))
                                aob = -aob;
                        if (is_exactly_a<numeric>(aob)
                            and aob.info(info_flags::real)) {
                                r = power(ex(ex_to<numeric>(aob).numer()), power(ne, _ex_1));
                                s = power(ex(ex_to<numeric>(aob).denom()), power(ne, _ex_1));
                        }
                        else {
                                ex t = aob.numer_denom();
                                r = power(t.op(0), power(ne,_ex_1));
                                s = power(t.op(1), power(ne,_ex_1));
                        }
                        int nn = nm.to_int() + 1;
                        ex sum = _ex0;
                        if (not (ae/be).info(info_flags::negative)) {
                                for (int k=1; k<=nn; ++k)
                                        sum = sum + rubi212(r, -s*cos(Pi*(2*k-1)/ne), r*r, _ex1, _ex_2*r*s*cos(Pi*(2*k-1)/ne), s*s, _ex_1, x);
                                return r/ae/ne*rubi111(r,s,_ex_1,x) + _ex2*r/ae/ne * sum;
                        }
                        else {
                                for (int k=1; k<=nn; ++k)
                                        sum = sum + rubi212(r, s*cos(Pi*(2*k-1)/ne), r*r, _ex1, _ex_2*r*s*cos(Pi*(2*k-1)/ne), s*s, _ex_1, x);
                                return r/ae/ne*rubi111(r,-s,_ex_1,x) + _ex2*r/ae/ne * sum;
                        }
                }
                // 4.1.3.2
                if (ne.is_equal(_ex4)) {
                        ex r, s;
                        if ((ae/be).is_positive()) {
                                sqrt_nd(ae/be, r, s);
                                return rubi223(r,-s,_ex1,ae,_ex0,be,_ex_1,x)/_ex2/r + rubi223(r,s,_ex1,ae,_ex0,be,_ex_1,x)/_ex2/r;
                        }
                        if ((ae/be).is_negative()) {
                                sqrt_nd(-ae/be, r, s);
                                return rubi211(r,_ex0,-s,_ex_1,x)*r/_ex2/ae + rubi211(r,_ex0,s,_ex_1,x)*r/_ex2/ae;
                        }
                }
        }

        // 4.1.4
        if (pe.is_equal(_ex_1_2)) {
                // 4.1.4.1
                if (ne.is_equal(_ex2)) {
                       bool aminus = false, bminus = false;
                       if (is_exactly_a<mul>(ae)
                           and ex_to<mul>(ae).get_overall_coeff().is_negative())
                                    aminus = true;
                       if (is_exactly_a<mul>(be)
                           and ex_to<mul>(be).get_overall_coeff().is_negative())
                                    bminus = true;
                       if (ae.info(info_flags::positive)) {
                               if (bminus or be.info(info_flags::negative))
                                       return 1/sqrt(-be)*asin(x*sqrt(-be)/sqrt(ae));
                               return 1/sqrt(be)*asinh(x*sqrt(be)/sqrt(ae));
                       }
                       else
                               return rubi131(_ex1, -be, _ex2, _ex_1, x).subs(x == x/sqrt(ae+be*x*x));
                       return 1/sqrt(-be) * asin(x*sqrt(-be)/sqrt(ae));
                }
                // 4.1.4.2
                if (ne.is_equal(_ex3)) {
                        ex q = power(be/ae, _ex1_3);
                        ex res = q.numer_denom();
                        ex r = res.op(0);
                        ex s = res.op(1);
                        ex r3 = power(_ex3, _ex1_2);
                        ex r3p = r3+1;
                        ex r3m = r3-1;
                        if ((ae).is_positive()) {
                                ex t = power(r3p*s + r*x, _ex2);
                                return _ex2*power(r3+2,_ex1_2)*(s+r*x)*power((s*s-r*s*x+r*r*x*x)/t,_ex1_2)/power(_ex3,_ex1_4)/r/power(ae+be*power(x,3),_ex1_2)/power(s*(s+r*x)/t,_ex1_2) * _ellF(asin((r*x-r3m*s)/(r*x+r3p*s)), _ex_7-_ex4*r3);
                        }
                        ex t = power(r*x - r3m*s, _ex2);
                        return _ex2*power(_ex2-r3,_ex1_2)*(s+r*x)*power((s*s-r*s*x+r*r*x*x)/t,_ex1_2)/power(_ex3,_ex1_4)/r/power(ae+be*power(x,3),_ex1_2)/power(_ex0-s*(s+r*x)/t,_ex1_2) * _ellF(asin((r*x+r3p*s)/(r*x-r3m*s)), _ex_7+_ex4*r3); 
                }
                // 4.1.4.3
                if (ne.is_equal(_ex4)) {
                        if ((be/ae).is_positive()) {
                                ex q = power(be/ae, _ex1_4);
                                return (q*q*x*x+1)*power((ae+be*power(x,4))/(q*q*x*x+1)/ae, _ex1_2)/_ex2/q/power(ae+be*power(x,4),_ex1_2) * _ellF(_ex2*atan(q*x),_ex1_2);
                        }
                        if (ae.is_negative() and be.is_positive()) {
                                ex q = power(-ae*be,_ex1_2);
                                return power(-ae+q*x*x,_ex1_2)*power(ae/q+x*x,_ex1_2)/power(_ex_2,_ex1_2)/power(-ae,_ex1_2)/power(ae+be*power(x,_ex4),_ex1_2) * _ellF(asin(x/power((ae+q*x*x)/_ex2/q,_ex1_2)), _ex1_2);
                        }
                        if ((be/ae).is_negative()
                            and ae.is_negative())
                                return power(be*power(x,_ex4)/ae+1,_ex1_2)/power(ae+be*power(x,_ex4),_ex1_2) * rubi131(_ex1,be/ae,_ex4,_ex_1_2,x);
                        //if ((be/ae).is_negative()
                        //    and ae.is_positive())
                                return _ex1/power(ae,_ex1_4)/power(-be,_ex1_4) * _ellF(asin(power(-be,_ex1_4)/power(ae,_ex1_4)*x), _ex_1);
                }
        }

        // 4.1.5
        if (pe.is_equal(_ex_1_4)) {
                if (ne.is_equal(_ex2)) {
                       // Rule 4.1.5.1
                       if ((be/ae).info(info_flags::positive))
                               return _ex2*x/power(ae+be*x*x,_ex1_4) - ae*rubi131(ae, be, _ex2, _ex_5/_ex4, x);
                       if ((be/ae).info(info_flags::negative)) {
                               // Rule 4.1.5.2.1
                               if (ae.info(info_flags::positive))
                                       return _ex2/power(ae,_ex1_4)/sqrt(-be/ae) * _ellE(_ex1_2*asin(sqrt(-be/ae)*x), _ex2);
                               // Rule 4.1.5.2.2
                               else
                                       return power(1+be/ae*x*x,_ex1_4)/power(ae+be*x*x,_ex1_4) * rubi131(_ex1, be/ae, _ex2, _ex_1_4, x);
                       }
                }
        }

        // 4.1.6
        if (pe.is_equal(_ex_3_4) and ne.is_equal(_ex2)) {
                if (ae.info(info_flags::positive)) {
                       if ((be/ae).info(info_flags::positive))
                               return _ex2/power(ae,_ex3_4)/sqrt(be/ae) * _ellF(_ex1_2*atan(sqrt(be/ae)*x), _ex2);
                       if ((be/ae).info(info_flags::negative))
                               return _ex2/power(ae,_ex3_4)/sqrt(-be/ae) * _ellF(_ex1_2*asin(sqrt(-be/ae)*x), _ex2);

                }
                else
                        return power(1+be/ae*x*x,_ex3_4)/power(ae+be*x*x,_ex3_4) * rubi131(_ex1, be/ae, _ex2, _ex_3_4, x);
        }

        // Rule 4.1.7
        if (pe.is_equal(_ex_1_3) and ne.is_equal(_ex2))
                return _ex3_2*sqrt(be*x*x)/be/x * rubi132(_ex1, _ex1, -ae, _ex1, _ex3, _ex_1_2, x).subs(x == power(ae+be*x*x, _ex1_3));

        if (ne.info(info_flags::integer)
            and ne.info(info_flags::positive)
            and is_exactly_a<numeric>(pe)) {
                numeric p = ex_to<numeric>(pe);
                numeric pp = p+1;
                if ((p*2).is_integer()
                    or (ne.is_equal(_ex2)
                        and ((p*4).is_integer() or (p*3).is_integer()))
                    or (is_exactly_a<numeric>(ne)
                        and ((p+ex_to<numeric>(ne).inverse()).denom() - p.denom()).is_negative())) {
                        // Fallback Rule 4.1.1.2
                        if (p.is_positive())
                                return x*power(ae+be*power(x,ne),pe)/(ne*pe+_ex1) + ae*ne*pe/(ne*pe+_ex1) * rubi131(ae, be, ne, pe-_ex1, x);
                        // Fallback Rule 4.1.2.3
                        if (pp.is_negative())
                                return x*power(ae+be*power(x,ne),pp)/ae/ne/pp + (ne*pp+_ex1)/ae/ne/pp * rubi131(ae, be, ne, pp, x);
                }
        }
        // Rule 1.2, loops if -1/2<n<0?
        if (not pe.is_equal(_ex_1)
            and discr.info(info_flags::integer)
            and discr.info(info_flags::negative))
                return _ex0-x*power(ae+be*power(x,ne),pe+1)/ae/ne/(pe+_ex1) + (ne*(pe+_ex1)+_ex1)/ae/ne/(pe+_ex1) * rubi131(ae, be, ne, pe+_ex1, x);
        if (ae.is_zero())
                throw rubi_exception();

        // H.7.1/2
        if ((not pe.is_integer()
             or not pe.is_positive())
             and not (_ex1/ne).is_integer()
             and (not (_ex1/ne+pe).is_integer()
                  or not (_ex1/ne+pe).is_negative())) {
                if (ae.is_positive() or pe.is_integer()) 
                        return power(ae,pe)*x * _2F1(_ex0-pe,_ex1/ne,_ex1/ne+1,_ex0-be/ae*power(x,ne));
                ex ip, fp;
                if (is_exactly_a<numeric>(pe)) {
                        fp = ex_to<numeric>(pe).frac();
                        ip = ex_to<numeric>(pe).floor();
                }
                else {
                        fp = pe;
                        ip = _ex0;
                }
                return power(ae,ip)*power(ae+be*power(x,ne),fp)/power(be/ae*power(x,ne)+1,fp) * rubi131(_ex1,be/ae,ne,pe, x);
        }
        return x*power(ae+be*power(x,ne),pe+1)/ae * _2F1(_ex1, pe+_ex1/ne+1, _ex1/ne+1, _ex0-be*power(x,ne)/ae);
}

// 1.1.3.2 (cx)^m * (a+bx^n)^p
static ex rubi132(ex ce, ex me, ex ae, ex be, ex ne, ex pe, ex x)
{
        DEBUG std::cerr<<"r132: "<<ce<<","<<me<<","<<ae<<","<<be<<","<<ne<<","<<pe<<std::endl;
        if (me.is_zero())
                return rubi131(ae,be,ne,pe,x);
        if (pe.is_integer() and pe.is_positive())
                return rubi(expand(power(ce*x,me)*power(ae+be*power(x,ne),pe)),x);
        if (ne.is_one())
                return rubi112(_ex0,ce,me,ae,be,pe,x);
        // 0
        if (ae.is_zero()) {
                if (me.is_integer() or ce.is_positive()) {
                        if (((me+1)/ne).is_integer())
                                return power(ce,me)/ne/power(be,(me+1)/ne-1) * rubi111(_ex0,be,pe+(me+1)/ne-1,x).subs(x==power(x,ne));
                        if (is_exactly_a<numeric>(pe)) {
                                numeric fp = ex_to<numeric>(pe).frac();
                                numeric ip = ex_to<numeric>(pe).floor();
                                return power(ce,me)*power(be,ip)*power(be*power(x,ne),fp)/power(x,ne*fp) * rubi111(_ex0,_ex1,me+ne*pe,x);
                        }
                }
                if (is_exactly_a<numeric>(me)) {
                        numeric fm = ex_to<numeric>(me).frac();
                        numeric im = ex_to<numeric>(me).floor();
                        return power(ce,im)*power(ce+x,fm)/power(x,fm) * power(x,me+1)*power(be*x,ne)/(me+ne+1);
                }
                return power(ce*x,me)*power(be*x,ne)*x/(me+ne+1);
        }
        // Rule 1.1
        if ((me-ne+_ex1).is_zero())
                return _ex1/ne * rubi111(ae, be, pe, x).subs(x == power(x,ne));

        // Rule 7.1.4.1.1.1
        if (ce.is_one() and me.is_one()
            and ne.is_equal(_ex3) and pe.is_minus_one()) {
                ex a13 = power(ae, _ex1_3);
                ex b13 = power(be, _ex1_3);
                const ex _ex2_3 = _ex2/_ex3;
                return _ex0 - power(_ex3*a13*b13, _ex_1) * rubi111(a13,b13,_ex_1,x) + power(_ex3*a13*b13, _ex_1) * rubi212(a13,b13,_ex1,power(ae,_ex2_3),_ex0-a13*b13,power(be,_ex2_3),_ex_1,x);
        }
        if (ce.is_one() and me.is_equal(_ex2)) {
                // 7.1.4.1.2.2.1
                if (ne.is_equal(_ex4) and pe.is_minus_one()) {
                        if ((ae/be).is_positive()) {
                                ex r,s;
                                sqrt_nd(ae/be, r, s);
                                return _ex1_2/s * rubi223(r,s,_ex1,ae,_ex0,be,_ex_1,x) - _ex1_2/s * rubi223(r,-s,_ex1,ae,_ex0,be,_ex_1,x);
                        }
                        else {
                                ex r,s;
                                sqrt_nd(-ae/be, r, s);
                                return s/_ex2/be * rubi211(r,_ex0,s,_ex_1,x) - s/_ex2/be * rubi211(r,_ex0,-s,_ex_1,x);
                        }
                }
                // 7.1.5.2
                if (ne.is_equal(_ex4) and pe.is_equal(_ex_1_2)) {
                        if ((ae/be).is_positive()) {
                                ex q = power(ae/be, _ex1_2);
                                return _ex1/q * rubi131(ae,be,4,_ex_1_2,x) - _ex1/q * rubi223(_ex1,-q,_ex1,ae,_ex0,ce,_ex_1_2,x);
                        }
                }
        }

        // 12
        if (not (pe.info(info_flags::integer)
                 and pe.info(info_flags::positive))) {
                // Fallback Rule 12.1
                if ((pe.info(info_flags::integer)
                    and pe.info(info_flags::negative))
                    or ae.info(info_flags::positive))
                        return power(ae,pe)*power(ce*x,me+_ex1)/ce/(me+_ex1) * _2F1(-pe, (me+_ex1)/ne, (me+_ex1)/ne+_ex1, -be*power(x,ne)/ae);
                // Fallback Rule 12.x
                else
                        return power(ae+be*power(x,ne),pe+_ex1)*power(ce*x,me+_ex1)/ae/ce/(me+_ex1) * _2F1(_ex1, (me+_ex1)/ne+pe+_ex1, (me+_ex1)/ne+_ex1, -be*power(x,ne)/ae);
        }
        // H
        if (pe.info(info_flags::integer)
            or ae.info(info_flags::negative))
                throw rubi_exception();
        return power(ce*x,me+1) * power(ae+be*power(x,ne),pe+1) / ae / ce / (me+1) * _2F1(_ex1, (me+1)/ne+pe+1, (me+1)/ne+1, -be/ae*power(x,ne));
}

// 1.1.3.3 (a+bx^n)^p * (c+dx^n)^q
static ex rubi133(ex a, ex b, ex c, ex d, ex n, ex p, ex q, ex x)
{
        DEBUG std::cerr<<"r133: "<<a<<","<<b<<","<<c<<","<<d<<","<<n<<","<<p<<","<<q<<std::endl;
        if (p.is_zero())
                return rubi131(c,d,n,q,x);
        if (q.is_zero())
                return rubi131(a,b,n,p,x);
        if (n.is_one())
                return rubi112(a,b,n,c,d,n,x);
        if ((n*(p+q+1)+1).is_zero()) {
                if (p.info(info_flags::integer)
                    and p.info(info_flags::negative))
                        return power(a,p)*x/power(c,p+1)/power(c+d*power(x,n),_ex1/n) * _2F1(_ex1/n, -p, _ex1+_ex1/n, -(b*c-a*d)*power(x,n)/a/(c+d*power(x,n)));
                if (q.info(info_flags::integer)
                    and q.info(info_flags::negative))
                        return power(c,q)*x/power(a,q+1)/power(a+b*power(x,n),_ex1/n) * _2F1(_ex1/n, -q, _ex1+_ex1/n, (b*c-a*d)*power(x,n)/c/(a+b*power(x,n)));
                return power(a+b*power(x,n),p)*x/c/power(c/a*(a+b*power(x,n))/(c+d*power(x,n)),p)/power(c+d*power(x,n),_ex1/n+p) * _2F1(_ex1/n, -p, _ex1+_ex1/n, -(b*c-a*d)*power(x,n)/a/(c+d*power(x,n)));
        }
        ex bcmad = b*c-a*d;
        // 9
        if (not bcmad.is_zero()
            and (p.is_minus_one() or q.is_minus_one())) {
                // 9.1
                if (p.is_minus_one() and q.is_minus_one())
                        return b/bcmad*rubi131(a,b,n,_ex_1,x) - d/bcmad*rubi131(c,d,n,_ex_1,x);
                if (p.is_minus_one()) {
                        a.swap(c);
                        b.swap(d);
                        p.swap(q);
                        bcmad = _ex0-bcmad;
                }
        }
        // 13.1 parts
        if (n.is_equal(_ex2) and p.is_equal(_ex_1_2) and q.is_equal(_ex_1_2)) {
                if (not bcmad.is_zero()) {
                        if (not (d/c).is_positive()) {
                                if (a.is_positive() and c.is_positive())
                                        return _ex1/power(a,_ex1_2)/power(c,_ex1_2)/power(-d/c,_ex1_2) * _ellF(asin(power(-d/c,_ex1_2)), b*c/a/d);
                                if ((a-b*c/d).is_positive() and c.is_positive())
                                        return _ex1/power(c,_ex1_2)/power(-d/c,_ex1_2)/power(a-b*c/d,_ex1_2) * _ellF(acos(power(-d/c,_ex1_2)), b*c/(b*c-a*d));
                        }
                }
        }
        // 13.2
        if (n.is_equal(_ex2) and p.is_equal(_ex1_2) and q.is_equal(_ex_1_2)) {
                if ((d/c).is_positive()) {
                        if ((b/a).is_positive())
                                return a * rubi133(a,b,c,d,_ex2,_ex_1_2,_ex_1_2,x) + b * rubi134(_ex2,a,b,_ex2,_ex_1_2,c,d,_ex_1_2,x);
                        return b/d * rubi133(c,d,a,b,_ex2,_ex1_2,_ex_1_2,x) - (b*c-a*d)/d * rubi133(a,b,c,d,_ex2,_ex_1_2,_ex_1_2,x);
                }
                if (c.is_positive()) {
                        if (a.is_positive())
                                return power(a,_ex1_2)/power(c,_ex1_2)*power(-d/c,_ex1_2) * _ellE(asin(power(-d/c,_ex1_2)*x), b*c/a/d);
                        if ((a-b*c/d).is_positive())
                                return _ex0-power(a-b*c/d,_ex1_2)/power(c,_ex1_2)/power(-d/c,_ex1_2) * _ellE(acos(power(-d/c,_ex1_2)*x), b*c/(b*c-a*d));
                        return power(a+b*x*x,_ex1_2)/power(1+b/a*x*x,_ex1_2) * rubi133(_ex1,b/a,c,d,_ex2,_ex1_2,_ex_1_2,x);
                }
                return power(1+d/c*x*x,_ex1_2)/power(c+d*x*x,_ex1_2) * rubi133(a,b,_ex1,d/c,_ex2,_ex1_2,_ex_1_2,x);
        }
        throw rubi_exception();
}

// 1.1.3.4 x^m * (a+bx^n)^p * (c+dx^n)^q
static ex rubi134(ex m, ex a, ex b, ex n, ex p, ex c, ex d, ex q, ex x)
{
        DEBUG std::cerr<<"r134H: "<<m<<","<<a<<","<<b<<","<<n<<","<<p<<","<<c<<","<<d<<","<<q<<std::endl;
        if (p.is_one() or q.is_one())
                return rubi((power(x,m) * power(a+b*power(x,n),p) * power(c+d*power(x,n),q)).expand(), x);
        if (n.is_one() and not (a.is_zero() or c.is_zero()))
                return rubi113(_ex0,_ex1,m,a,b,p,c,d,q,x);
        if (m.is_zero())
                return rubi133(a,b,c,d,n,p,q,x);
        // 0
        if (a.is_zero() or c.is_zero()) {
                if (not a.is_zero()) {
                        a.swap(c);
                        b.swap(d);
                        p.swap(q);
                }
                if (m.is_integer()) {
                        if (((m+1)/n).is_integer())
                                return _ex1/n/power(b,(m+1)/n-1) * rubi132(b,p+(m+1)/n-1,c,d,n,q,x);
                        if (is_exactly_a<numeric>(p)) {
                                numeric fp = ex_to<numeric>(p).frac();
                                numeric ip = ex_to<numeric>(p).floor();
                                return power(b,ip)*power(b*power(x,n),fp)/power(x,n*fp) * rubi132(_ex1,m+n*p,c,d,n,q,x);
                        }
                }
                if (not m.is_integer()) {
                }
        }

        if (not (b*c-a*d).is_zero()) { 
                // 6.1.3
                if (n.is_integer() and n.is_positive()
                    and (not m.is_integer() and m.info(info_flags::rational))) {
                        ex k = m.denom();
                        return k * rubi134(k*(m+1)-1,a,b,k*n,p,c,d,q,x).subs(x==power(x,_ex1/k));
                }
                // 6.1.11 (n==2)
                if (m.is_equal(_ex2) and n.is_equal(_ex2)
                    and p.is_equal(_ex_1_2) and q.is_equal(_ex_1_2)) {
                        if ((b/a).is_positive() and (d/c).is_positive())
                                return x/b*power(a+b*x*x,_ex1_2)/power(c+d*x*x,_ex1_2) - c/b * rubi133(a,b,c,d,_ex2,_ex1_2,-_ex3/_ex2,x);
                        return _ex1/b*power(a+b*x*x,_ex1_2)/power(c+d*x*x,_ex1_2) - a/b * rubi133(a,b,c,d,_ex2,_ex_1_2,_ex_1_2,x);
                }
        }
        // X
        if (m.is_equal(_ex_1) or (m-n).is_equal(_ex_1)
            or a.info(info_flags::negative)
            or c.info(info_flags::negative))
                throw rubi_exception();
        return power(a,p)*power(c,q)*power(x,m+1)/(m+1) * appell_F1((m+1)/n, -p, -q, (m+1)/n+1, -b/a*power(x,n), -d/c*power(x,n));
}

// 1.1.4.1 (ax^j + bx^n)^p
static ex rubi141H(ex a, ex j, ex b, ex n, ex p, ex x)
{
        DEBUG std::cerr<<"r141H: "<<a<<","<<j<<","<<b<<","<<n<<","<<p<<std::endl;
        if ((j*p+1).is_zero())
                return x * power(a*power(x,j)+b*power(x,n), p) / p / (n-j) / power(power((a*power(x,j)+b*power(x,n))/b/power(x,n), p)) * _2F1(-p, -p, -p+1, -a/b/power(x,n-j));
        if ((n*p+1).is_zero())
                return x * power(a*power(x,j)+b*power(x,n), p) / p / (j-n) / power(power((a*power(x,j)+b*power(x,n))/a/power(x,j), p)) * _2F1(-p, -p, -p+1, -b/a/power(x,j-n));
        return x * power(a*power(x,j)+b*power(x,n), p) / (j*p+1) / power(power((a*power(x,j)+b*power(x,n))/a/power(x,j), p)) * _2F1(-p, (j*p+1)/(n-j), (j*p+1)/(n-j)+1, -b/a*power(x,n-j));
}

// 1.1.4.2H x^m * (ax^j+bx^n)^p
static ex rubi142H(ex m, ex a, ex j, ex b, ex n, ex p, ex x)
{
        DEBUG std::cerr<<"r142x: "<<m<<","<<a<<","<<j<<","<<b<<","<<n<<","<<p<<std::endl;
        // Rule x
        if ((m+j*p+1).is_zero())
                return power(a*power(x,j)+b*power(x,n),p+1) / b / p / (n-j) / power(x,n+j*p) * _2F1(_ex1, _ex1, _ex1-p, -a/b/power(x,n-j));
        if ((m+n*p+1).is_zero())
                return power(a*power(x,j)+b*power(x,n),p+1) / a / p / (j-n) / power(x,j+n*p) * _2F1(_ex1, _ex1, _ex1-p, -b/a/power(x,j-n));
        if ((m+n+(p-1)*j+1).is_zero())
                return power(a*power(x,j)+b*power(x,n),p+1) / b / (p+1) / (n-j) / power(x,_ex2*n+j*(p+1)) * _2F1(_ex1, _ex2, _ex2-p, -a/b/power(x,n-j));
        if ((m+j+(p-1)*n+1).is_zero())
                return power(a*power(x,j)+b*power(x,n),p+1) / a / (p+1) / (j-n) / power(x,_ex2*j+n*(p+1)) * _2F1(_ex1, _ex2, _ex2-p, -b/a/power(x,j-n));
        return power(x,m-j+1) * power(a*power(x,j)+b*power(x,n),p+1) / a / (m+j*p+1)  * _2F1(_ex1, (m+n*p+1)/(n-j)+1, (m+j*p+1)/(n-j)+1, -b/a*power(x,n-j));
}

// 1.2.1.1 (a+bx+cx^2)^p
static ex rubi211(ex a, ex b, ex c, ex p, ex x)
{
        DEBUG std::cerr<<"r211: "<<a<<","<<b<<","<<c<<","<<p<<std::endl;
        if (b.is_zero())
                return rubi131(a,c,_ex2,p,x);
        if (c.is_zero())
                return rubi111(a,b,p,x);
        ex qq = b*b - a*c*_ex4;
        ex q = power(qq, _ex1_2);
        // Rule 1
        if (qq.is_zero()) {
                if (p.is_equal(_ex_1_2)) {
                        return (x*c + b/_ex2) * power(x*x*c+x*b+a, _ex_1_2) * rubi(power(x*c+b/_ex2, _ex_1), x);
                }
                ex mf;
                if (a.info(info_flags::negative)) {
                        a = -a;
                        b = -b;
                        c = -c;
                        mf = power(_ex_1, p);
                }
                else
                        mf = _ex1;
                return mf * _ex2/sqrt(c) * power(x*b/_ex2/sqrt(a)+b/_ex2/sqrt(c), _ex2*p+_ex1) / _ex2 / (_ex2*p+_ex1);
        }

        // Rule 2
        if (p.info(info_flags::positive) and (p*_ex4).info(info_flags::integer)) {
                if (p.info(info_flags::integer)) {
                        if (is_exactly_a<numeric>(qq)
                            and ex_to<numeric>(qq).is_square())
                                return power(c, -p) * rubi112((b-q)/_ex2, c, p, (b+q)/_ex2, c, p, x);
                        else
                                return rubi(expand(power(x*x*c+x*b+a, p)), x);
                }
                return (_ex2*x*c+b)*power(x*x*c+x*b+a, p)/c/_ex2/(p*_ex2+_ex1) - p*qq/c/_ex2/(p*_ex2+_ex1) * rubi211(a,b,c,p-_ex1,x);
        }

        // Rule 3
        if ((p+_ex1).info(info_flags::negative) and (p*_ex4).info(info_flags::integer)) {
                if (p.is_equal(_ex_3/_ex2))
                        return _ex2 * (_ex2*x*c+b) / qq / sqrt(x*x*c+x*b+a);
                return (_ex2*x*c+b)*power(x*x*c+x*b+a, p+_ex1)/(p+_ex1)/qq - _ex2*c*(p*_ex2+_ex3)/(p+_ex1)/qq * rubi211(a,b,c,p+_ex1,x);
        }

        // 4
        if (p.is_equal(_ex_1)) {
                // Rule 4.1, relaxed the square condition
                if (qq.info(info_flags::positive))
                        return c/q * rubi(power(x*c+(b-q)/_ex2,_ex_1),x) - c/q * rubi(power(x*c+(b+q)/_ex2,_ex_1),x);
                // Rule 4.2
                if (not b.is_zero()) {
                        ex d = _ex1 - _ex4*a*c/b/b;
                        if (d.info(info_flags::rational)
                            and ((d*d).is_one()
                                    or not qq.info(info_flags::rational)))
                                return _ex_2/b * rubi131(d, _ex_1, _ex2, _ex_1, x).subs(x == x*c*_ex2/b+_ex1);
                }
                // Rule 4.3
                return _ex_2*rubi131(qq, _ex_1, _ex2, _ex_1, x).subs(x == x*c*_ex2+b);
                // the same?
                return 2*atan(b/sqrt(-qq) + _ex2*c*x/sqrt(-qq)) / sqrt(-qq);
        }
        
        // Rule 5
        if ((4*a-b*b/c).info(info_flags::positive)) {
                ex t = rubi131(_ex1, _ex_1/qq, _ex2, _ex_1_2, x);
                return dist(power(_ex2*c*power(_ex_4*c/qq, p), _ex_1),
                                t.subs(x == (x*c*_ex2+b).normal(0,
                                                false, true)));
        }

        // Rule 6
        if (p.is_equal(_ex_1_2)) {
                if (qq.is_one())
                        return asin(_ex2*x + b/c);
                if (a.is_zero())
                        return _ex2 * rubi131(_ex1, -c, _ex2, _ex_1, x).subs(x == x * power(b*x + c*x*x, _ex_1_2));
                return _ex2 * rubi131(_ex4*c, _ex_1, _ex2, _ex_1, x).subs(x == (b+_ex2*c*x) * power(a + b*x + c*x*x, _ex_1_2));
        }

        // Fallback rule H
        //return _ex0-power(x*x*c+x*b+a, p+_ex1) / (q*(p+_ex1) * power((_ex_2*x*c+q-b)/_ex2/q, p+_ex1)) * _2F1(-p, p+_ex1, p+_ex2, (_ex2*x*c+b+q)/_ex2/q);

        return -power(a+b*x+c*x*x,p+1) / q / (p+1) / power((q-b-_ex2*c*x)/_ex2/q,p+1) * _2F1(-p, p+1, p+2, (b+q+_ex2*c*x)/_ex2/q);

        // Rule 7
        // return power(x*x*c+x*b+a, pe) / power(-(x*x*c+x*b+a)*c/qq, p) * rubi211(-a*c/qq, -b*c/qq, -c*c/qq, pe, x);
}

static bool nice_sqrt_helper(ex e)
{
        if (is_exactly_a<numeric>(e)) {
                if (not ex_to<numeric>(e).is_positive())
                        DEBUG std::cerr<<"nice_sqrt warning"<<std::endl;
                return true;
        }
        if (is_exactly_a<power>(e))
                return e.op(1).info(info_flags::rational)
                        or e.op(1).info(info_flags::even);
        if (is_exactly_a<mul>(e)) {
                for (const auto& term : e)
                        if (not nice_sqrt_helper(term))
                                return false;
                return true;
        }
        if (is_exactly_a<add>(e)) {
                ex f;
                bool r = factor(e, f);
                if (not r or is_exactly_a<add>(f))
                        return false;
                return nice_sqrt_helper(f);
        }
        return false;
}

static bool has_nice_sqrt(ex e)
{
        if (e.is_negative())
                return false;
        return nice_sqrt_helper(e);
}

// 1.2.1.2 (d+ex)^m * (a+bx+cx^2)^p
static ex rubi212(ex d, ex e, ex m, ex a, ex b, ex c, ex p, ex x)
{
        DEBUG std::cerr<<"r212: "<<d<<","<<e<<","<<m<<","<<a<<","<<b<<","<<c<<","<<p<<std::endl;
        if (p.is_zero())
                return rubi111(d,e,m,x);
        if (m.is_zero())
                return rubi211(a,b,c,p,x);
//        if ((m.is_integer()
//             and ((not d.is_zero() and m.is_positive())
//                     or (m-1).is_positive()))
//            or (p.is_integer()
//             and ((not (a*b).is_zero() and p.is_positive())
//                     or (p-1).is_positive())))
//                return rubi(expand(power(d+e*x,m)
//                                        * power(a+b*x+c*x*x,p)), x);
        // Rule 1
        ex qq = b*b - _ex4*a*c;
        if (m.is_one()) {
                if (p.is_minus_one()) {
                         if ((b*e).is_equal(_ex2*c*d)) // int(Qr/Pq) rule
                                 return e*log(remove_content(a+b*x+c*x*x,x))/_ex2/c;
                }
                // Rule 1.1
                if ((_ex2*c*d - b*e).is_zero()) {
                        if (p.is_minus_one())
                                return d/b * log(remove_content(a+b*x+c*x*x,x));
                        else
                                return d/b/(p+1) * power(a+b*x+c*x*x, p+1);
                }
                // Rule 1.2.1
                if (p.is_integer() and p.is_positive()) {
                        if ((c*d*d - b*d*e + a*e*e).is_zero())
                                return rubi112(d,e,p+1,a/d,c/e,p,x);
                        return rubi(expand((d+e*x) * power(a+b*x+c*x*x, p)),x);
                }
                // Rule 1.2.2
                if (p.is_minus_one()
                    and not qq.is_zero()) {
                        if (b.is_zero())
                                return d * rubi(power(a+c*x*x,_ex_1),x) + e * rubi(x/(a+c*x*x),x);
                        if (has_nice_sqrt(qq)) {
                                ex q = power(qq, _ex1_2);
                                return (c*d-e*(b-q)/_ex2)/q * rubi111((b-q)/_ex2,c,_ex_1,x) - (c*d-e*(b+q)/_ex2)/q * rubi111((b+q)/_ex2,c,_ex_1,x);
                        }
                        else {
                                return (2*c*d-b*e)/_ex2/c * rubi211(a,b,c,_ex_1,x) + e/_ex2/c * rubi212(b,_ex2*c,_ex1,a,b,c,_ex_1,x);
                        }
                }
        }

        // Rule 2
        if (qq.is_zero()) {
                if (p.is_integer())
                        return power(c,-p) * rubi112(d,e,m,d/_ex2,c,_ex2*p,x);
                // 2.2.1
                if ((_ex2*c*d-b*e).is_zero()) {
                        if (m.is_integer()) {
                                if ((m/_ex2).is_integer())
                                        return power(e,m)/power(c,m/_ex2) * rubi211(a,b,c,p+m/_ex2,x);
                                return power(e,m-1)*power(c,(m-1)/_ex2) * rubi212(d,e,_ex1,a,b,c,p+(m-1)/_ex2,x);
                        }
                        return power(a+b*x+c*x*x,p)/power(d+e*x,_ex2*p) * rubi111(d,e,m+_ex2*p,x);
                }
                // 2.2.2
                if (is_exactly_a<numeric>(p)) {
                        numeric fp = ex_to<numeric>(p).frac();
                        numeric ip = ex_to<numeric>(p).floor();
                        return power(a+b*x+c*x*x,fp)/power(c,ip)/power(b/_ex2+c*x,_ex2*fp) * rubi112(d,e,m,b/_ex2,c,_ex2*p,x);
                }
        }
        ex QQ = c*d*d - b*d*e + a*e*e;
        // 3.2.8.2
        if (not qq.is_zero()
            and QQ.is_zero()
            and p.is_positive()
            and (((m+2).is_positive()
                  and m.is_negative())
                 or (m+p+1).is_zero())
            and not (m+_ex2*p+1).is_zero()
            and (_ex2*p).is_integer()) {
               if (b.is_zero())
                       return power(d+e*x,m+1)*power(a+c*x*x,p)/e/(m+_ex2*p+1) -dist(2*c*d*p/e/e/(m+_ex2*p+1), rubi212(d,e,m+1,a,_ex0,c,p-1,x));
               return power(d+e*x,m+1)*power(a+b*x+c*x*x,p)/e/(m+_ex2*p+1) -dist(p*(2*c*d-b*e)/e/e/(m+_ex2*p+1), rubi212(d,e,m+1,a,b,c,p-1,x));
        }

        throw rubi_exception();
}

// 1.2.1.3 (d+ex)^m * (f+gx) * (a+bx+cx^2)^p
static ex rubi213(ex d, ex e, ex m, ex f, ex g, ex a, ex b, ex c, ex p, ex x)
{
        DEBUG std::cerr<<"r213: "<<d<<","<<e<<","<<m<<","<<f<<","<<g<<","<<a<<","<<b<<","<<c<<","<<p<<std::endl;
        if ((e*f-d*g).is_zero())
                return f/d * rubi212(d,e,m+1,a,b,c,p,x);
        ex qq = b*b - _ex4*a*c;
        // 6
        if (qq.is_zero()
            and is_exactly_a<numeric>(p)) {
                numeric fp = ex_to<numeric>(p).frac();
                numeric ip = ex_to<numeric>(p).floor();
                return power(a+b*x+c*x*x,fp)/power(c,ip)/power(b*b+c*x,_ex2*fp) * rubi113(d,e,m,f,g,_ex1,b/_ex2,c,_ex2*p, x);
        }
        // 8.2.4
        if (not (m+_ex2*p+_ex2).is_zero()
            and (not m.is_equal(_ex2) or d.is_zero())
            and (c*d*d - b*d*e + a*e*e).is_zero()
            and not qq.is_zero()) {
                if (b.is_zero())
                        return g*power(d+e*x,m)*power(a+c*x*x,p+1)/c/(m+_ex2*p+_ex2) + dist((m*(d*g+e*f)+_ex2*e*f*(p+1))/e/(m+_ex2*p+_ex2), rubi212(d,e,m,a,_ex0,c,p,x));
                return g*power(d+e*x,m)*power(a+b*x+c*x*x,p+1)/c/(m+_ex2*p+_ex2) + dist((m*(g*(c*d-b*e)+c*e*f)+e*(p+1)*(_ex2*c*f-b*g))/c/e/(m+_ex2*p+_ex2), rubi212(d,e,m,a,b,c,p,x));
        }
        throw rubi_exception();
}

// 1.2.1.4 (d+ex)^m * (f+gx)^n * (a+bx+cx^2)^p
static ex rubi214(ex de, ex ee, ex me, ex fe, ex ge, ex ne, ex ae, ex be, ex ce, ex pe, ex x)
{
        DEBUG std::cerr<<"r214: "<<de<<","<<ee<<","<<me<<","<<fe<<","<<ge<<","<<ne<<","<<ae<<","<<be<<","<<ce<<","<<pe<<std::endl;
        if (me.is_one())
                return rubi213(fe,ge,ne,de,ee,ae,be,ce,pe,x);
        if (ne.is_one())
                return rubi213(de,ee,me,fe,ge,ae,be,ce,pe,x);
        // 9
        if (ae.is_zero() and not pe.is_integer()) {
                if (de.is_zero()
                    and not (ne.is_integer() and ne.is_positive())) {
                        if (be.is_zero())
                                return power(ee*x,me)*power(ce*x*x,pe)/power(x,me+pe)/power(ce*x,pe) * rubi113(_ex0,_ex1,me+pe,fe,ge,ne,_ex0,ce,pe,x);
                        return power(ee*x,me)*power(be*x+ce*x*x,pe)/power(x,me+pe)/power(be+ce*x,pe) * rubi113(_ex0,_ex1,me+pe,fe,ge,ne,be,ce,pe,x);
                }
                if (fe.is_zero()
                    and not (me.is_integer() and me.is_positive())) {
                        if (be.is_zero())
                                return power(ge*x,ne)*power(ce*x*x,pe)/power(x,ne+pe)/power(ce*x,pe) * rubi113(_ex0,_ex1,ne+pe,de,ee,me,_ex0,ce,pe,x);
                        return power(ge*x,ne)*power(be*x+ce*x*x,pe)/power(x,ne+pe)/power(be+ce*x,pe) * rubi113(_ex0,_ex1,ne+pe,de,ee,me,be,ce,pe,x);
                }
        }
        throw rubi_exception();
}

// 1.2.2.2 (dx)^m * (a+bx^2+cx^4)^p
static ex rubi222(ex de, ex me, ex ae, ex be, ex ce, ex pe, ex x)
{
        DEBUG std::cerr<<"r222: "<<de<<","<<me<<","<<ae<<","<<be<<","<<ce<<","<<pe<<std::endl;
        ex bbmac = be*be - _ex4*ae*ce;
        if (not bbmac.is_zero()) {
                // 10
                if (pe.is_minus_one()) {
                        // 10.3
                        if (de.is_one()
                            and me.is_integer()
                            and me.is_positive()
                            and (me-4).is_negative()
                            and bbmac.is_negative()) {
                                ex q = power(ae/ce, _ex1_2);
                                ex r = power(_ex2*q-be*ce, _ex1_2);
                                // 10.3.1
                                if ((ae*ce).is_positive())
                                        return _ex1_2 * rubi223(q,_ex1,_ex1,ae,be,ce,_ex_1,x) - _ex1_2 * rubi223(q,_ex_1,_ex1,ae,be,ce,_ex_1,x);
                                // 10.3.2
                                if (me.is_equal(_ex3))
                                        return _ex1_2/ce/r * rubi212(q,r,_ex1,q,r,_ex1,_ex_1,x) - _ex1_2/ce/r * rubi212(q,-r,_ex1,q,-r,_ex1,_ex_1,x);
                                // 10.3.3
                                if (me.is_one() or me.is_equal(_ex2))
                                        return _ex1_2/ce/r * rubi212(_ex0,_ex1,me-1,q,-r,_ex1,_ex_1,x) - _ex1_2/ce/r * rubi212(_ex0,_ex1,me-1,q,r,_ex1,_ex_1,x);
                        }
                        ex q = power(bbmac, _ex1_2);
                        // 10.4
                        if (me.is_equal(_ex2)
                            or (me-_ex2).is_positive())
                                return de*de/_ex2*(be/q+1) * rubi212(_ex0,de,me-2,be/_ex2+q/_ex2,_ex0,ce,_ex_1,x) - de*de/_ex2*(be/q-1) * rubi212(_ex0,de,me-2,be/_ex2-q/_ex2,_ex0,ce,_ex_1,x);
                        // 10.5
                        return ce/q * rubi212(_ex0,de,me,be/_ex2-q/_ex2,_ex0,ce,_ex_1,x) - ce/q * rubi212(_ex0,de,me,be/_ex2+q/_ex2,_ex0,ce,_ex_1,x);
                }
        }

        throw rubi_exception();
}

// 1.2.2.3 (d+ex^2)^q * (a+bx^2+cx^4)^p
static ex rubi223(ex d, ex e, ex q, ex a, ex b, ex c, ex p, ex x)
{
        DEBUG std::cerr<<"r223: "<<d<<","<<e<<","<<q<<","<<a<<","<<b<<","<<c<<","<<p<<std::endl;
        // 6.3.2.2.1
        if (b.is_zero() and q.is_one() and p.is_equal(_ex_1_2)) {
                ex cdpae = c*d*d + a*e*e;
                if ((c/a).is_negative()) {
                        if (cdpae.is_zero()) {
                                if (a.is_positive())
                                        return d/power(a,_ex1_2) * rubi133(_ex1,e/d,_ex1,-e/d,_ex2,_ex1_2,_ex_1_2, x);
                        }
                }
        }
        ex bbmac = b*b - _ex4*a*c;
        // 5.1.1
        if (not bbmac.is_zero()
            and not (c*d*d - b*d*e + a*e*e).is_zero()
            and q.is_one() and p.is_minus_one()) {
                ex cdmae = c*d*d - a*e*e;
                ex tdembc = _ex2*d/e - b/c;
                // 5.1.1.1
                if (cdmae.is_zero()) {
                        // 5.1.1.1.1
                        if (tdembc.is_positive()) {
                                ex Q = power(tdembc, _ex1_2);
                                return e/_ex2/c * rubi211(d/e,Q,_ex1,_ex_1,x) + e/_ex2/c * rubi211(d/e,-Q,_ex1,_ex_1,x);
                        }
                        // 5.1.1.1.2
                        if (bbmac.is_positive()) {
                                ex Q = power(bbmac, _ex1_2);
                                return (e/_ex2+(_ex2*c*d-b*e)/_ex2/Q) * rubi211(b/_ex2-Q/_ex2,_ex0,c,_ex_1,x) + (e/_ex2-(_ex2*c*d-b*e)/_ex2/Q) * rubi211(b/_ex2+Q/_ex2,_ex0,c,_ex_1,x);
                        }
                        // 5.1.1.1.3
                        if ((d*e).is_negative()) {
                                ex Q = power(_ex_2*d/e, _ex1_2);
                                return dist(e/_ex2/c/Q, rubi212(Q,_ex_2,_ex1,d/e,Q,_ex_1,_ex_1,x)) + dist(e/_ex2/c/Q, rubi212(Q,_ex2,_ex1,d/e,-Q,_ex_1,_ex_1,x));
                        }
                }
                // 5.1.1.2.1
                if (bbmac.is_positive()) {
                        ex Q = power(bbmac, _ex1_2);
                        ex t = (_ex2*c*d - b*e)/_ex2/Q;
                        return (e/_ex2+t) * rubi211(b/_ex2-Q/_ex2,_ex0,c,_ex_1,x) + (e/_ex2-t) * rubi211(b/_ex2+Q/_ex2,_ex0,c,_ex_1,x);
                }
                // 5.1.1.2.2.1
                if (b.is_zero()) {
                        ex Q = power(a*c, _ex1_2);
                        return (d*Q+a*e)/_ex2/a/e * rubi223(Q,c,_ex1,a,_ex0,c,_ex_1,x) + (d*Q-a*e)/_ex2/a/e * rubi223(Q,-c,_ex1,a,_ex0,c,_ex_1,x);
                }
                // 5.1.1.2.2.2
                ex Q = power(a/c, _ex1_2);
                ex r = power(_ex2*Q-b/c, _ex1_2);
                return _ex1_2/c/Q/r * rubi212(d*r,-d+e*Q,_ex1,Q,-r,_ex1,_ex_1,x) + _ex1_2/c/Q/r * rubi212(d*r,d-e*Q,_ex1,Q,r,_ex1,_ex_1,x);
        }
        throw rubi_exception();
}

static ex rubi11(ex e, ex x)
{
        DEBUG std::cerr<<"r11: "<<e<<std::endl;
        throw rubi_exception();
}

static ex rubi1x1(ex bas, ex exp, ex xe)
{
        DEBUG std::cerr<<"r1x1: "<<bas<<","<<exp<<std::endl;
        throw rubi_exception();
}

static bool rubi91(ex& the_ex, ex& f, const symbol& x)
{
        ex e = the_ex;
        ex w0=wild(0), w1=wild(1), w2=wild(2), w3=wild(3), w4=wild(4), w5=wild(5);
        exvector w;
        // u(c(a+bx)^n)^p
        if (e.match(w1 * power(w2 * power(w3, w4), w0), w)
            or e.match(w1*w5 * power(w2*power(w3, w4), w0), w) ) {
                ex a,b,core = w[3];
                if (core.is_linear(x, a, b)
                    and not b.is_zero()) {
                        ex p = w[0];
                        if (not p.is_integer()) {
                                ex fp,ip;
                                if (is_exactly_a<numeric>(p)) {
                                        fp = ex_to<numeric>(p).frac();
                                        ip = ex_to<numeric>(p).floor();
                                }
                                else {
                                        fp = p;
                                        ip = _ex0;
                                }
                                ex u, ww5;
                                if (w.size() < 6)
                                        ww5  = NaN;
                                else 
                                        ww5 = w[5];
                                if (ww5.is_equal(NaN))
                                        u = w[1];
                                else if (w[1].is_equal(NaN))
                                        u = ww5;
                                else
                                        u = w[1]*ww5;
                                f = power(w[2],ip)*power(w[2]*power(core,w[4]),fp)/power(core,w[4]*fp);
                                the_ex = u*power(core,w[4]*p);
                DEBUG std::cerr<<"u(c(a+bx)^n)^p: "<<a<<","<<b<<","<<u<<","<<w[4]<<","<<w[0]<<std::endl;
                DEBUG std::cerr<<"u(c(a+bx)^n)^p: f="<<f<<", the_ex="<<the_ex<<std::endl;
                                return true;
                        }
                }
        }
        // u(av)^m(bv)^n
        // TODO: simplify by using constant wildcards
        bool matched = false;
        if (e.match(w0 * power(w1*w5,w2) *power(w3*w5,w4), w)
            and not w[2].is_integer()
            and not w[4].is_integer()
            and not (w[2]+w[4]).is_integer()) {
                DEBUG std::cerr<<"u(av)^m(bv)^n: "<<e<<std::endl;
                DEBUG std::cerr<<"u(av)^m(bv)^n: "<<w[0]<<","<<w[1]<<","<<w[2]<<","<<w[3]<<","<<w[4]<<","<<w[5]<<std::endl;
#if 0
                if (not is_exactly_a<power>(w[0])
                and not is_exactly_a<add>(w[1])
                and not is_exactly_a<add>(w[3]))
                        matched = true;
//                if (is_exactly_a<power>(w[0]))
//...
                ex a,b,av, m=w[2], n=w[4], v=av=w[1], bv=w[3], u=w[0];
                // TODO: simplify this by implementing zero wildcards
                ex t = __tuple(av, bv);
                if (t.match(__tuple(w0*w1,w1), w)) {
                        a = w[0]; b = _ex1; v = w[1];
                }
                else if (t.match(__tuple(w1,w0*w1), w)) {
                        b = w[0]; a = _ex1; v = w[1];
                }
                else if (t.match(__tuple(w0*w1,w2*w1), w)) {
                        a = w[0]; b = w[2]; v = w[1];
                }
                else {
                        a = av; b = bv;
                }
#endif
                ex u=w[0], a=w[1], b=w[3], v=w[5], m=w[2], n=w[4];
                DEBUG std::cerr<<"u,a,b,v: "<<u<<","<<a<<","<<b<<","<<v<<std::endl;
                ex fp,ip;
                if (is_exactly_a<numeric>(n)) {
                        fp = ex_to<numeric>(n).frac();
                        ip = ex_to<numeric>(n).floor();
                }
                else if (is_exactly_a<numeric>(m)) {
                        fp = ex_to<numeric>(m).frac();
                        ip = ex_to<numeric>(m).floor();
                }
                else {
                        fp = n;
                        ip = _ex0;
                }
                f = power(b,ip)*power(b*v,fp)/power(a,ip)/power(a*v,fp);
                the_ex = u*power(a*v,m+n);
                DEBUG std::cerr<<"f: "<<f<<std::endl;
                return true;
        }
        // uv^m(bv)^n
        if (e.match(w0 * power(w1,w2) *power(w3*w1,w4), w)) {
                DEBUG std::cerr<<"uv^m(bv)^n: "<<w[0]<<","<<w[1]<<","<<w[2]<<","<<w[3]<<","<<w[4]<<","<<std::endl;
                if (not has_symbol(w[3], x)) {
                        f = power(w[3], -w[2]);
                        the_ex = w[0]*power(w[1]*w[3],w[2]+w[4]);
                DEBUG std::cerr<<"uv^m(bv)^n: f="<<f<<", the_ex="<<the_ex<<std::endl;
                        return true;
                }
        }
        if (e.match(w0 * w1 *power(w3*w1,w4), w)) {
                DEBUG std::cerr<<"uv(bv)^n: "<<e<<std::endl;
                DEBUG std::cerr<<"uv(bv)^n: "<<w[0]<<","<<w[1]<<","<<w[3]<<","<<w[4]<<","<<std::endl;
                if (not has_symbol(w[3], x)) {
                        f = power(w[3], _ex_1);
                        the_ex = w[0]*power(w[1]*w[3], w[4]+1);
                DEBUG std::cerr<<"uv(bv)^n: f="<<f<<", the_ex="<<the_ex<<std::endl;
                        return true;
                }
        }
        return false;
}

}
