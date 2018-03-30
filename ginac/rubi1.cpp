#include <Python.h>
#include "numeric.h"
#include "power.h"
#include "add.h"
#include "mul.h"
#include "symbol.h"
#include "constant.h"
#include "relational.h"
#include "ex.h"
#include "utils.h"
#include "operators.h"
#include "inifcns.h"
#include "py_funcs.h"
#include "ex_utils.h"

#ifdef PYNAC_HAVE_LIBGIAC
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE

#include <stdexcept>
#include <functional>

#define Py_INCREF(op) (                         \
    _Py_INC_REFTOTAL  _Py_REF_DEBUG_COMMA       \
    ((PyObject*)(op))->ob_refcnt++) ; \
std::cerr << "+ " << long(op) << ", " << Py_REFCNT(op) << ", " << Py_TYPE(op)->tp_name << std::endl; std::cerr.flush();

#define Py_DECREF(op)                                   \
    do {                                                \
std::cerr << "- " << long(op) << ", " << Py_REFCNT(op) << ", " << Py_TYPE(op)->tp_name << std::endl; std::cerr.flush(); \
        if (_Py_DEC_REFTOTAL  _Py_REF_DEBUG_COMMA       \
        --((PyObject*)(op))->ob_refcnt != 0)            \
            _Py_CHECK_REFCNT(op)                        \
        else                                            \
        _Py_Dealloc((PyObject *)(op));                  \
    } while (0); std::cerr << "Done" << std::endl; std::cerr.flush();
#endif

#define DEBUG if (debug)

inline void py_error(const char* errmsg) {
        throw std::runtime_error((PyErr_Occurred() != nullptr) ? errmsg:
                        "pyerror() called but no error occurred!");
}

namespace GiNaC {

class rubi_exception : public std::runtime_error {
        public:
                rubi_exception() : std::runtime_error("") {}
};

ex rubi(ex e, ex x);
static ex rubi11(ex e, ex x);
static ex rubi111(ex a, ex b, ex m, ex x);
static ex rubi112(ex ae, ex be, ex me, ex ce, ex de, ex ne, ex x);
static ex rubi113(ex a, ex b, ex m, ex c, ex d, ex n, ex e, ex f, ex p, ex x);
static ex rubi131(ex ae, ex be, ex ne, ex pe, ex x);
static ex rubi132(ex ce, ex me, ex ae, ex be, ex ne, ex pe, ex x);
static ex rubi133(ex a, ex b, ex c, ex d, ex n, ex p, ex q, ex x);
static ex rubi134H(ex m, ex a, ex b, ex n, ex p, ex c, ex d, ex q, ex x);
static ex rubi141H(ex a, ex j, ex b, ex n, ex p, ex x);
static ex rubi142H(ex m, ex a, ex j, ex b, ex n, ex p, ex x);
static ex rubi1x1(ex bas, ex exp, ex xe);
static ex rubi211(ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi212(ex de, ex ee, ex me, ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi223(ex de, ex ee, ex qe, ex ae, ex be, ex ce, ex pe, ex x);

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

static bool debug=false;

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
        ex the_ex = e;
        (void)the_ex.expand();
        if (is_exactly_a<add>(the_ex)) {
                const add& m = ex_to<add>(the_ex);
                exvector vec;
                for (unsigned int i=0; i<m.nops(); i++)
                        vec.push_back(rubi(m.op(i), ex(x)));
                return add(vec);
        }
        if (is_exactly_a<mul>(the_ex)) {
                const mul& mu = ex_to<mul>(the_ex);
                exvector cvec, xvec;
                for (unsigned int i=0; i<mu.nops(); i++) {
                        if (has_symbol(mu.op(i), x))
                                xvec.push_back(mu.op(i));
                        else
                                cvec.push_back(mu.op(i));
                }
                if (not cvec.empty())
                        return mul(cvec) * rubi(mul(xvec), x);
                exvector bvec, evec;
                for (const auto& term : xvec) {
                        if (is_exactly_a<power>(term)) {
                                bvec.push_back(ex_to<power>(term).op(0));
                                evec.push_back(ex_to<power>(term).op(1));
                        }
                        else {
                                bvec.push_back(term);
                                evec.push_back(_ex1);
                        }
                }
                // exponentials ---> bailout
                for (const auto& eterm : evec)
                        if (has_symbol(eterm, x))
                                throw rubi_exception();

                if (xvec.size() == 2) {
                        ex a, b, c, d, j, k, m, n;
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
                }
                if (xvec.size() == 3) {
                        ex a, b, c, d, ee, f;
                        if (bvec[0].is_linear(x, a, b)
                            and bvec[1].is_linear(x, c, d)
                            and bvec[2].is_linear(x, ee, f))
                                return rubi113(a, b, evec[0], c, d, evec[1], ee, f, evec[2], x);
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
                                        if (not e3.is_equal(e5))
                                                throw rubi_exception();
                                        return rubi134H(evec[0],c,d,e4,evec[1],ee,f,evec[2],x);
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
                                        if (not e1.is_equal(e5))
                                                throw rubi_exception();
                                        return rubi134H(evec[1],a,b,e2,evec[0],ee,f,evec[2],x);
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
                                        if (not e3.is_equal(e1))
                                                throw rubi_exception();
                                        return rubi134H(evec[2],c,d,e4,evec[1],a,b,evec[0],x);
                                }
                        }
                }
                throw rubi_exception();
        }
        ex a, b, c;
        if (is_exactly_a<power>(e)) {
                const power& p = ex_to<power>(e);
                const ex& ebas = p.op(0);
                const ex& m = p.op(1);
                ex j, n;
                if (has_symbol(ebas, x) and not has_symbol(m, x)) {
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
        else {
                if (is_exactly_a<numeric>(m)) {
                        if (ex_to<numeric>(m).is_minus_one())
                                return log(a+b*x) / b;
                        else
                                return power(a+b*x,m+1) / b / (m+1);
                }
                else throw rubi_exception();
        }
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

// 1.1.1.2 (a+bx)^m * (c+dx)^n
static ex rubi112(ex a, ex b, ex m, ex c, ex d, ex n, ex x)
{
        DEBUG std::cerr<<"r112: "<<a<<","<<b<<","<<m<<","<<c<<","<<d<<","<<n<<std::endl;
        if (m.is_zero())
                return rubi111(c, d, n, x);
        if (n.is_zero())
                return rubi111(a, b, m, x);
        if (a.is_zero() and c.is_zero())
                return rubi111(_ex0, b*d, m+n, x);

        // Rule 3
        bool e1p = is_exactly_a<numeric>(m) and m.info(info_flags::posint);
        bool e2p = is_exactly_a<numeric>(n) and n.info(info_flags::posint);
        if ((e1p and e2p)
            or (e1p and c.is_zero())
            or (e2p and a.is_zero()))
                return rubi((power(a+b*x, m) * power(c+d*x, n)).expand(), x);
        ex bcmad = (b*c - a*d).expand();
        if (bcmad.is_zero()) {
                if ((a/c-_ex1).is_positive())
                        return rubi(power(a/c, m) * power(c+d*x, m+n), x);
                else
                        return rubi(power(c/a, n) * power(a+b*x, m+n), x);
        }
        else if ((e1p and not a.is_zero())
                 or (e2p and not c.is_zero()) )
                return rubi((power(a+b*x, m) * power(c+d*x, n)).expand(), x);

        ex bcpad = (b*c + a*d).expand();
        if (m.is_equal(n)) {
                if (m.is_equal(_ex_1_2)) {
                        // Rule 7.1.1.1
                        if ((b+d).is_zero()
                            and (a+c).is_positive())
                                return power(a*c - b*(a-c)*x - b*b*x*x, m);
                        return _ex2 * (rubi(1/(b-d*power(x,_ex2)), x)).subs(x == sqrt(a+b*x)/sqrt(c+d*x));
                }

                // Rule 1.1 simplified
                else if (m.is_equal(_ex_1))
                        return log(b*x + a)/bcmad - log(d*x + c)/bcmad;
                // 2.1
                if (bcpad.is_zero()) {
                        if (is_exactly_a<numeric>(m)
                            and ex_to<numeric>(m).denom().is_equal(*_num2_p)) {
                                numeric mnum = ex_to<numeric>(m);
                                // Rule 2.1.2.1
                                if (mnum.is_equal((*_num_3_p)/(*_num2_p)))
                                        return x/(a*c*sqrt(a+b*x)*sqrt(c+d*x));
                                // Rule 2.1.4
                                if (mnum.is_equal(*_num_1_2_p)) {
                                        if ((a+c).is_zero()) {
                                                if (a.info(info_flags::positive))
                                                        return acosh(b*x/a) / b;
                                                if (c.info(info_flags::positive))
                                                        return acosh(d*x/c) / d;
                                        }
                                        if ((a*c).info(info_flags::positive))
                                                return asin(-b*x/a) / sqrt(-b*d);
                                        return _ex2*rubi131(b,-d,_ex2,_ex_1,x).subs(x == sqrt(a+b*x)/sqrt(c+d*x));
                                }
                                // Rule 2.1.1
                                if (mnum.numer().is_positive())
                                        return x*power(a+b*x, m)*power(c+d*x, m)/(_ex2*m+_ex1) + _ex2*a*c*m/(_ex2*m+_ex1) * rubi112(a, b, m-_ex1, c, d, m-_ex1, x);
                                // Rule 2.1.2.2
                                else
                                        return _ex0-x*power(a+b*x, m+_ex1)*power(c+d*x,m+_ex1)/_ex2/a/c/(m+_ex1) + (_ex2*m+_ex3)/_ex2/a/c/(m+_ex1) * rubi112(a, b, m+_ex1, c, d, m+_ex1, x);
                                        
                        }
                        // Rule 2.1.3
                        if (m.info(info_flags::integer)
                            or (a.info(info_flags::positive)
                                and c.info(info_flags::positive)))
                                return rubi131(a*c, b*d, _ex2, m, x);
                        if (is_exactly_a<numeric>(m)) {
                                // Fallback Rule 2.1.5
                                numeric fpm = ex_to<numeric>(m).frac();
                                return power(a+b*x,fpm) * power(c+d*x,fpm) / power(a*c+b*d*x*x,fpm) * rubi131(a*c, b*d, _ex2, m, x);
                        }
                }
        }
        else if ((m-n).is_negative()) {
                // make m > n
                m.swap(n);
                a.swap(c);
                b.swap(d);
                bcmad = _ex0 - bcmad;
//                return rubi112(a, b, m, c, d, n, x);
        }
        else if (not (m-n).info(info_flags::positive)) {
                throw rubi_exception();
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
                        if (n.info(info_flags::positive))
                                return power(c+d*x, n+_ex1) * power(a+b*x, m) / b / (m+n+_ex1) + _ex2*c*m/(m+n+_ex1) * rubi112(c, d, n, a, b, m-_ex1, x);
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
        
        // 4
        const ex _ex3_2 = _ex3/_ex2;
        const ex _ex_3_2 = _ex_3/_ex2;
        const ex _ex2_3 = _ex2/_ex3;
        const ex _ex_2_3 = _ex_2/_ex3;
        if (m.is_minus_one() or n.is_minus_one()) { // we know m>n
                // Rule 4.1
                if (n.is_minus_one() and m.is_positive())
                        return power(a+b*x,m)/d/m - expand(bcmad/d * rubi(power(a+b*x,m-1)/(c+d*x),x));
                // Rule 4.2
                if ((n+_ex1).is_negative()) // we know m=-1
                        return _ex0 - power(c+d*x,n+_ex1)/(n+_ex1)/bcmad + expand(b/bcmad * rubi(power(c+d*x,n+_ex1)/(a+b*x),x));
                const ex bcmad_b = bcmad / d;
                if (m.is_equal(_ex_1_3)) { // we know n=-1
                        // Rule 4.3.1.1
                        if (bcmad_b.is_positive()) {
                                ex q = power(bcmad_b, _ex1_3);
                                return _ex0-log(c+d*x)/_ex2/d/q - _ex3_2/d/q * rubi111(q,_ex_1,_ex_1,x).subs(x==power(a+b*x,_ex1_3)) + _ex3_2/d * rubi211(q*q,q,_ex1,_ex_1,x).subs(x==power(a+b*x,_ex1_3));
                        }
                        // Rule 4.3.1.2
                        else if (bcmad_b.is_negative()) {
                                ex q = power(_ex0-bcmad_b, _ex1_3);
                                return log(c+d*x)/_ex2/d/q - _ex3_2/d/q * rubi111(q,_ex1,_ex_1,x).subs(x==power(a+b*x,_ex1_3)) + _ex3_2/d * rubi211(q*q,-q,_ex1,_ex_1,x).subs(x==power(a+b*x,_ex1_3));
                        }
                }
                if (m.is_equal(_ex_2_3)) { // we know n=-1
                        // Rule 4.3.2.1
                        if (bcmad_b.is_positive()) {
                                ex q = power(bcmad_b, _ex1_3);
                                return _ex0-log(c+d*x)/_ex2/d/q/q - _ex3_2/d/q/q * rubi111(q,_ex_1,_ex_1,x).subs(x==power(a+b*x,_ex1_3)) - _ex3_2/d/q * rubi211(q*q,q,_ex1,_ex_1,x).subs(x==power(a+b*x,_ex1_3));
                        }
                        // Rule 4.3.2.2
                        else if (bcmad_b.is_negative()) {
                                ex q = power(_ex0-bcmad_b, _ex1_3);
                                return _ex0-log(c+d*x)/_ex2/d/q/q + _ex3_2/d/q/q * rubi111(q,_ex1,_ex_1,x).subs(x==power(a+b*x,_ex1_3)) + _ex3_2/d/q * rubi211(q*q,-q,_ex1,_ex_1,x).subs(x==power(a+b*x,_ex1_3));
                        }
                }
                // Rule 4.3.3
                if (is_exactly_a<numeric>(m)
                    and m.is_negative()
                    and ((m+_ex1).is_positive())) {
                        numeric p = ex_to<numeric>(m).denom();
                        return ex(p) * rubi132(_ex1, (m+_ex1)*p-_ex1, bcmad, d, p, _ex_1, x).subs(x == power(a+b*x,p.inverse()));
                }

                // Fallback Rules H.4
                if (m.is_minus_one() // n < -1
                    and not n.is_integer()) {
                        if (a.is_zero())
                                return _ex0-power(c+d*x, n+_ex1) / (c*(n+_ex1)) * _2F1(1, n+_ex1, n+_ex2, _ex1+d*x/c);
                        return _ex0-power(c+d*x, n+_ex1) / (bcmad*(n+_ex1)) * _2F1(1, n+_ex1, n+_ex2, (b*(c+d*x)/bcmad).normal());
                }
                if (n.is_minus_one() // m > -1
                    and not m.is_integer()) {
                        if (c.is_zero())
                                return _ex0-power(a+b*x, m+_ex1) / (a*(m+_ex1)) * _2F1(1, m+_ex1, m+_ex2, _ex1+b*x/a);
                        return power(a+b*x, m+_ex1) / (bcmad*(m+_ex1)) * _2F1(1, m+_ex1, m+_ex2, (-d*(a+b*x)/bcmad).normal());
                }
        }
        
        // Rule 5
        if (not bcmad.is_zero()
            and ((m+1).is_negative() or (n+1).is_negative())) {
                if (m.is_positive()) {
                        m.swap(n);
                        a.swap(c);
                        b.swap(d);
                        bcmad = _ex0 - bcmad;
                }
                if (n.is_positive())
                        return power(a+b*x,m+1)*power(c+d*x,n)/b/(m+1) - d*n/b/(m+1) * rubi112(a,b,m+1,c,d,n-1,x);
                return power(a+b*x,m+1)*power(c+d*x,n+1)/bcmad/(m+1) - d*(m+n+2)/bcmad/(m+1) * rubi112(a,b,m+1,c,d,n,x);
        }

        // Rule 6
        if (not bcmad.is_zero()
            and not (m+n+_ex1).is_zero()) {
                if (not n.is_positive()) {
                        m.swap(n);
                        a.swap(c);
                        b.swap(d);
                        bcmad = _ex0 - bcmad;
                }
                if (n.is_positive())
                        return power(a+b*x,m+1)*power(c+d*x,n)/b/(m+n+1) + n*bcmad/b/(m+n+1) * rubi112(a,b,m,c,d,n-1,x);
        }
        throw rubi_exception();
}

// 1.1.1.3 (a+bx)^m * (c+dx)^n * (e+fx)^p
static ex rubi113(ex a, ex b, ex m, ex c, ex d, ex n, ex e, ex f, ex p, ex x)
{
        DEBUG std::cerr<<"r113: "<<a<<","<<b<<","<<m<<","<<c<<","<<d<<","<<n<<","<<e<<","<<f<<","<<p<<std::endl;
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
        ex _ex3_2 = _ex3/_ex2;
        ex _ex3_4 = _ex3/_ex4;
        ex _ex_3_4 = _ex_3/_ex4;
        if (ne.is_one())
                return rubi111(ae, be, pe, x);
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
                        return rubi(expand(power(ae + power(be,ne), pe)), x);
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
        // 4.1.3
        if (pe.is_equal(_ex_1)) {
                // 4.1.3.2
                // presumably this can be simplified
                if (ne.is_equal(_ex2)) {
                        bool aminus = false, bminus = false;
                        if (is_exactly_a<mul>(ae)
                           and ex_to<mul>(ae).get_overall_coeff().is_negative())
                                    aminus = true;
                        if (is_exactly_a<mul>(be)
                           and ex_to<mul>(be).get_overall_coeff().is_negative())
                                    bminus = true;
                        if (ae.is_positive()) {
                                if (be.is_positive())
                                        return _ex1/(sqrt(ae)*sqrt(be)) * atan(sqrt(be)*x/sqrt(ae));
                                else if (bminus or be.is_negative())
                                        return _ex1/(sqrt(ae)*sqrt(-be)) * atanh(sqrt(-be)*x/sqrt(ae));
                        }
                        if (aminus or ae.is_negative()) {
                                if (be.is_positive())
                                        return _ex_1/(sqrt(-ae)*sqrt(be)) * atanh(sqrt(be)*x/sqrt(-ae));
                                else if (bminus or be.is_negative())
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
                       if (ae.info(info_flags::positive)) {
                               if (be.info(info_flags::positive))
                                       return 1/sqrt(be)*asinh(x*sqrt(be)/sqrt(ae));
                               if (be.info(info_flags::negative))
                                       return 1/sqrt(-be)*asin(x*sqrt(-be)/sqrt(ae));
                       }
                       else
                               return rubi131(_ex1, -be, _ex2, _ex_1, x).subs(x == x/sqrt(ae+be*x*x));
                       return 1/sqrt(-be) * asin(x*sqrt(-be)/sqrt(ae));
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

        return x*power(ae+be*power(x,ne),pe+1)/ae * _2F1(_ex1, pe+_ex1/ne+1, _ex1/ne+1, _ex0-be*power(x,ne)/ae);
}

// 1.1.3.2 (cx)^m * (a+bx^n)^p
static ex rubi132(ex ce, ex me, ex ae, ex be, ex ne, ex pe, ex x)
{
        DEBUG std::cerr<<"r132: "<<ce<<","<<me<<","<<ae<<","<<be<<","<<ne<<","<<pe<<std::endl;
        if (me.is_zero())
                return rubi131(ae,be,ne,pe,x);
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
        if ((n*(p+q+1)+1).is_zero()) {
                if (p.info(info_flags::integer)
                    and p.info(info_flags::negative))
                        return power(a,p)*x/power(c,p+1)/power(c+d*power(x,n),_ex1/n) * _2F1(_ex1/n, -p, _ex1+_ex1/n, -(b*c-a*d)*power(x,n)/a/(c+d*power(x,n)));
                if (q.info(info_flags::integer)
                    and q.info(info_flags::negative))
                        return power(c,q)*x/power(a,q+1)/power(a+b*power(x,n),_ex1/n) * _2F1(_ex1/n, -q, _ex1+_ex1/n, (b*c-a*d)*power(x,n)/c/(a+b*power(x,n)));
                return power(a+b*power(x,n),p)*x/c/power(c/a*(a+b*power(x,n))/(c+d*power(x,n)),p)/power(c+d*power(x,n),_ex1/n+p) * _2F1(_ex1/n, -p, _ex1+_ex1/n, -(b*c-a*d)*power(x,n)/a/(c+d*power(x,n)));
        }
        throw rubi_exception();
}

// 1.1.3.4H x^m * (a+bx^n)^p * (c+dx^n)^q
static ex rubi134H(ex m, ex a, ex b, ex n, ex p, ex c, ex d, ex q, ex x)
{
        DEBUG std::cerr<<"r134H: "<<m<<","<<a<<","<<b<<","<<n<<","<<p<<","<<c<<","<<d<<","<<q<<std::endl;
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

        // Rule 6
        if (p.is_equal(_ex_1_2)) {
                if (a.is_zero())
                        return _ex2 * rubi131(_ex1, -c, _ex2, _ex_1, x).subs(x == x * power(b*x + c*x*x, _ex_1_2));
                else
                        return _ex2 * rubi131(_ex4*c, _ex_1, _ex2, _ex_1, x).subs(x == (b+_ex2*c*x) * power(a + b*x + c*x*x, _ex_1_2));
        }

        // Rule 5, loops with 1.1.2.2.1.3
        //if ((4*a-b*b/c).info(info_flags::positive)) {
        //        ex t = rubi112(_ex1, _ex1/q, p, _ex1, _ex_1/q, p, x);
        //        return power(_ex2*c*power(_ex_4*c/qq, p), _ex_1) * t.subs(x == x*c*_ex2+b);
        //}
        // Fallback rule H
        //return _ex0-power(x*x*c+x*b+a, p+_ex1) / (q*(p+_ex1) * power((_ex_2*x*c+q-b)/_ex2/q, p+_ex1)) * _2F1(-p, p+_ex1, p+_ex2, (_ex2*x*c+b+q)/_ex2/q);

        return -power(a+b*x+c*x*x,p+1) / q / (p+1) / power((q-b-_ex2*c*x)/_ex2/q,p+1) * _2F1(-p, p+1, p+2, (b+q+_ex2*c*x)/_ex2/q);

        // Rule 7
        // return power(x*x*c+x*b+a, pe) / power(-(x*x*c+x*b+a)*c/qq, p) * rubi211(-a*c/qq, -b*c/qq, -c*c/qq, pe, x);
}

// 1.2.1.2 (d+ex)^m * (a+bx+cx^2)^p
static ex rubi212(ex de, ex ee, ex me, ex ae, ex be, ex ce, ex pe, ex x)
{
        DEBUG std::cerr<<"r212: "<<de<<","<<ee<<","<<me<<","<<ae<<","<<be<<","<<ce<<","<<pe<<std::endl;
        // Rule 1
        if (me.is_one()) {
                // Rule 1.1
                if ((_ex2*ce*de - be*ee).is_zero()) {
                        if (pe.is_minus_one())
                                return de/be * log(ae+be*x+ce*x*x);
                        else
                                return de/be/(pe+1) * power(ae+be*x+ce*x*x, pe+1);
                }
                // Rule 1.2.1
                if (pe.is_integer() and pe.is_positive()) {
                        if ((ce*de*de - be*de*ee + ae*ee*ee).is_zero())
                                return rubi112(de,ee,pe+1,ae/de,ce/ee,pe,x);
                        return rubi(expand((de+ee*x) * power(ae+be*x+ce*x*x, pe)),x);
                }
                ex qq = be*be - _ex4*ae*ce;
                // Rule 1.2.2
                if (pe.is_minus_one()
                    and not qq.is_zero()) {
                        if (be.is_zero())
                                return de * rubi(power(ae+ce*x*x,_ex_1),x) + ee * rubi(x/(ae+ce*x*x),x);
                        ex q = power(qq, _ex1_2);
                        if ((is_exactly_a<power>(qq)
                            and ex_to<power>(q).op(1).info(info_flags::even))
                            or (is_exactly_a<numeric>(qq)
                                and ex_to<numeric>(qq).is_square()))
                                return (ce*de-ee*(be-q)/_ex2)/q * rubi111((be-q)/_ex2,ce,_ex_1,x) - (ce*de-ee*(be+q)/_ex2)/q * rubi111((be+q)/_ex2,ce,_ex_1,x);
                        else
                                return (2*ce*de-be*ee)/_ex2/ce * rubi211(ae,be,ce,_ex_1,x) + ee/_ex2/ce * rubi212(be,_ex2*ce,_ex1,ae,be,ce,_ex_1,x);
                }
        }
        throw rubi_exception();
}

static ex rubi223(ex de, ex ee, ex qe, ex ae, ex be, ex ce, ex pe, ex x)
{
        // Rule 5
        if (qe.is_one() and pe.is_minus_one()) {
                ex bbmac = be*be - _ex4*ae*ce;
                ex cdmae = ce*de*de - ae*ee*ee;
                ex tdembc = _ex2*de/ee - be/ce;
                if (be.is_zero()) {
                        if (cdmae.is_zero()) {
                                if ((de*ee).is_positive()) {
                                        ex q = power(_ex2*de/ee, _ex1_2);
                                        return ee/_ex2/ce * rubi211(de/ee,q,_ex1,_ex_1,x) + ee/_ex2/ce * rubi211(de/ee,-q,_ex1,_ex_1,x);
                                }
                        }
                }
                if (not bbmac.is_zero()) {
                        if (cdmae.is_zero()) {
                                // 5.1.1.1.1
                                if (tdembc.is_positive()) {
                                        ex q = power(tdembc, _ex1_2);
                                        return ee/_ex2/ce * rubi211(de/ee,q,_ex1,_ex_1,x) + ee/_ex2/ce * rubi211(de/ee,-q,_ex1,_ex_1,x);
                                }
                                if (bbmac.is_positive()) {
                                        ex q = power(bbmac, _ex1_2);
                                        return (ee/_ex2+(_ex2*ce*de-be*ee)/_ex2/q) * rubi211(be/_ex2-q/_ex2,_ex0,ce,_ex_1,x) + (ee/_ex2-(_ex2*ce*de-be*ee)/_ex2/q) * rubi211(be/_ex2+q/_ex2,_ex0,ce,_ex_1,x);
                                }
                        }
                }
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

}
