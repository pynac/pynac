
#include "numeric.h"
#include "power.h"
#include "add.h"
#include "mul.h"
#include "symbol.h"
#include "relational.h"
#include "ex.h"
#include "utils.h"
#include "operators.h"
#include "inifcns.h"
#include "py_funcs.h"

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
static ex rubi121(ex ae, ex be, ex ce, ex pe, ex x);
static ex rubi1x1(ex bas, ex exp, ex xe);

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
        if (not is_exactly_a<symbol>(xe))
                throw std::runtime_error("rubi(): not a symbol");
        const symbol& x = ex_to<symbol>(xe);
        if (not has_symbol(e, x))
                return e*x;
        if (e.is_equal(x))
                return power(x, _ex2) / _ex2;
        ex the_ex = e.expand();
        if (is_exactly_a<add>(the_ex)) {
                const add& m = ex_to<add>(the_ex);
                exvector vec;
                for (unsigned int i=0; i<m.nops(); i++)
                        vec.push_back(rubi(m.op(i), ex(x)));
                return add(vec);
        }
        if (is_exactly_a<mul>(the_ex)) {
                const mul& m = ex_to<mul>(the_ex);
                exvector cvec, xvec;
                for (unsigned int i=0; i<m.nops(); i++) {
                        if (has_symbol(m.op(i), x))
                                xvec.push_back(m.op(i));
                        else
                                cvec.push_back(m.op(i));
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
                int bdeg = 0;
                for (const auto& bterm : bvec)
                        bdeg = bdeg + bterm.degree(x);

                if (xvec.size() == 2) {
                        if (bdeg == 2) {
                                ex a, b, c, d;
                                if (not bvec[0].is_linear(x, a, b)
                                    or not bvec[1].is_linear(x, c, d))
                                        throw rubi_exception();
                                return rubi112(a, b, evec[0], c, d, evec[1], x);
                        }
                }
        }
        ex a, b, c;
        if (is_exactly_a<power>(e)) {
                const power& p = ex_to<power>(e);
                const ex& ebas = p.op(0);
                const ex& m = p.op(1);
                if (has_symbol(ebas, x) and not has_symbol(m, x)) {
                        if (ebas.is_linear(x, a, b))
                                return rubi111(a, b, m, ex(x));
                        else if (ebas.is_quadratic(x, a, b, c))
                                return rubi121(a, b, c, m, ex(x));
                        else
                                return rubi1x1(ebas, m, ex(x));
                }
        }
        return rubi11(the_ex, x);
}

static ex rubi111(ex a, ex b, ex m, ex x)
{
        if (b.is_integer_one() and a.is_zero())
                if (is_exactly_a<numeric>(m)) {
                        if (ex_to<numeric>(m).is_minus_one())
                                return log(x);
                        else
                                return power(x,m+1) / (m+1);
                }
                else throw rubi_exception();
        else
                if (is_exactly_a<numeric>(m)) {
                        if (ex_to<numeric>(m).is_minus_one())
                                return log(a+b*x) / b;
                        else
                                return power(a+b*x,m+1) / b / (m+1);
                }
                else throw rubi_exception();
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

// DON'T FORGET 9.1 !!!!!!!!!!!!
static ex rubi112(ex a, ex b, ex m, ex c, ex d, ex n, ex x)
{
        ex bcmad = (b*c - a*d).expand();
        if (bcmad.is_zero()) {
                if ((a/c-_ex1).info(info_flags::positive))
                        return rubi(power(a/c, m) * power(c+d*x, m+n), x);
                else
                        return rubi(power(c/a, n) * power(a+b*x, m+n), x);
        }
        ex bcpad = (b*c + a*d).expand();
        if (m.is_equal(n)) {
                if (m.is_equal(_ex_1_2)) {
                        // Rule 7.1.1.1
                        if ((b+d).is_zero()
                            and (a+c).info(info_flags::positive))
                                return power(a*c - b*(a-c)*x - b*b*x*x, m);
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
                                        return _ex2*rubi121(b,_ex0,-d,_ex_1,x).subs(x == sqrt(a+b*x)/sqrt(c+d*x));
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
                                return rubi121(a*c, _ex0, b*d, m, x);
                        if (is_exactly_a<numeric>(m)) {
                                // Fallback Rule 2.1.5
                                numeric fpm = ex_to<numeric>(m).frac();
                                return power(a+b*x,fpm) * power(c+d*x,fpm) / power(a*c+b*d*x*x,fpm) * rubi121(a*c, _ex0, b*d, m, x);
                        }
                }
        }
        else if ((m-n).info(info_flags::negative)) {
                // make m > n
                m.swap(n);
                a.swap(c);
                b.swap(d);
        }
        else if (not (m-n).info(info_flags::positive)) {
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
                if ((b*d*a*c).info(info_flags::positive)) { // never true?
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
        
        // Fallback Expansion Rule 3
        if (m.info(info_flags::positive)
            and m.info(info_flags::integer))
                return rubi(expand(expand(power(a+b*x,m)) * power(c+d*x,n)), x);
        if (n.info(info_flags::positive)
            and n.info(info_flags::integer))
                return rubi(expand(expand(power(c+d*x,n)) * power(a+b*x,m)), x);
        // Fallback Rule H.4
        if (m.is_equal(_ex_1) and a.is_zero())
                return _ex0-power(c+d*x, n+_ex1) / (c*(n+_ex1)) * _2F1(1, n+_ex1, n+_ex2, _ex1+d*x/c);
        if (n.is_equal(_ex_1) and c.is_zero())
                return _ex0-power(a+b*x, m+_ex1) / (a*(m+_ex1)) * _2F1(1, m+_ex1, m+_ex2, _ex1+b*x/a);
        throw rubi_exception();
}

static ex rubi121(ex a, ex b, ex c, ex p, ex x)
{
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
                return (_ex2*x*c+b)*power(x*x*c+x*b+a, p)/c/_ex2/(p*_ex2+_ex1) - p*qq/c/_ex2/(p*_ex2+_ex1) * rubi121(a,b,c,p-_ex1,x);
        }

        // Rule 3
        if ((p+_ex1).info(info_flags::negative) and (p*_ex4).info(info_flags::integer)) {
                if (p.is_equal(_ex_3/_ex2))
                        return _ex2 * (_ex2*x*c+b) / qq / sqrt(x*x*c+x*b+a);
                return (_ex2*x*c+b)*power(x*x*c+x*b+a, p+_ex1)/(p+_ex1)/qq - _ex2*c*(p*_ex2+_ex3)/(p+_ex1)/qq * rubi121(a,b,c,p+_ex1,x);
        }

        // Rule 4
        if (p.is_equal(_ex_1)) {
                if (is_exactly_a<numeric>(qq))
                // relaxed the square condition
                        return c/q * rubi(power(x*c+(b-q)/_ex2,_ex_1),x) - c/q * rubi(power(x*c+(b+q)/_ex2,_ex_1),x);
                if (qq.info(info_flags::negative))
                        return 2*atan(b/sqrt(-qq) + _ex2*c*x/sqrt(-qq)) / sqrt(-qq);
                // Rule 5 in the form b^2-4ac>0 not 4a-b^2/c>0
                if (qq.info(info_flags::positive)) {
                        ex t = rubi112(_ex1, _ex1/q, p, _ex1, _ex_1/q, p, x);
                        return power(_ex2*c*power(_ex_4*c/qq, p), _ex_1) * t.subs(x == x*c*_ex2+b);
                }
                throw rubi_exception();
        }

        // Rule 6
        if (p.is_equal(_ex_1_2)) {
                if (a.is_zero())
                        return _ex2 * rubi121(_ex1, _ex0, -c, _ex_1, x).subs(x == x * power(b*x + c*x*x, _ex_1_2));
                else
                        return _ex2 * rubi121(_ex4*c, _ex0, _ex_1, _ex_1, x).subs(x == (b+_ex2*c*x) * power(a + b*x + c*x*x, _ex_1_2));
        }

        // Fallback rule H
        return _ex0-power(x*x*c+x*b+a, p+_ex1) / (q*(p+_ex1) * power((_ex_2*x*c+q-b)/_ex2/q, p+_ex1)) * _2F1(-p, p+_ex1, p+_ex2, (_ex2*x*c+b+q)/_ex2/q);

        // Rule 7
        // return power(x*x*c+x*b+a, pe) / power(-(x*x*c+x*b+a)*c/qq, p) * rubi121(-a*c/qq, -b*c/qq, -c*c/qq, pe, x);
}

static ex rubi11(ex e, ex x)
{
        return _ex0;
}

static ex rubi1x1(ex bas, ex exp, ex xe)
{
       return _ex_1;
}

}
