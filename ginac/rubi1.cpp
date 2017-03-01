
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
static ex rubi121(ex ae, ex be, ex me, ex ce, ex de, ex ne, ex x);
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

static ex rubi121(ex ae, ex be, ex me, ex ce, ex de, ex ne, ex x)
{
        return _ex0;
}

static ex rubi121(ex ae, ex be, ex ce, ex pe, ex x)
{
        if (not is_exactly_a<numeric>(ae)
            or not is_exactly_a<numeric>(be)
            or not is_exactly_a<numeric>(ce))
                throw rubi_exception();

        const numeric& a = ex_to<numeric>(ae);
        const numeric& b = ex_to<numeric>(be);
        const numeric& c = ex_to<numeric>(ce);
        numeric qq = b*b - a*c*(*_num4_p);
        ex q = qq.sqrt_as_ex();
        
        // symbolic p --> rule H
        if (not is_exactly_a<numeric>(pe))
                return _ex0-power(x*x*c+x*b+a, pe+(*_num1_p)) / (q*(pe+(*_num1_p)) * power((_ex_2*x*c+q-b)/_ex2/q, pe+(*_num1_p))) * _2F1(-pe, pe+_ex1, pe+_ex2, (_ex2*x*c+b+q)/_ex2/q);
        
        const numeric& p = ex_to<numeric>(pe);
        
        // Rule 1
        if (qq.is_zero()) {
                if (p.is_equal(*_num_1_2_p)) {
                        return (x*c + b/(*_num2_p)) * power(x*x*c+x*b+a, _ex_1_2) * rubi111(b/(*_num2_p), c, *_num_1_p, x);
                }
                return (x*c*_ex2 + b) * power(x*x*c+x*b+a, pe) / c / _ex2 / (_ex2*p+_ex1);
        }

        // Rule 2
        if (p.is_positive() and (p*(*_num4_p)).is_integer()) {
                if (p.is_integer()) {
                        if (qq.is_square())
                                return power(c, -p) * rubi112((b-q)/(*_num2_p), c, p, (b+q)/(*_num2_p), c, p);
                        else
                                return rubi(power(x*x*c+x*b+a, pe).expand(), x);
                }
                return (_ex2*x*c+b)*power(x*x*c+x*b+a, pe)/c/_ex2/(p*(*_num2_p)+(*_num1_p)) - p*qq/c/_ex2/(p*(*_num2_p)+(*_num1_p)) * rubi121(ae,be,ce,pe-_ex1,x);
        }

        // Rule 3
        if (p < *_num_1_p and (p*(*_num4_p)).is_integer()) {
                if (p == *_num_3_2_p)
                        return _ex2 * (_ex2*x*c+b) / q / sqrt(x*x*c+x*b+a);
                return (_ex2*x*c+b)*power(x*x*c+x*b+a, pe+_ex1)/(pe+_ex1)/qq - _ex2*c*(p*(*_num2_p)+(*_num3_p))/(pe+_ex1)/qq * rubi121(ae,be,ce,pe+_ex1,x);
        }

        // Rule 4
        if (p == *_num_1_p) {
                if (qq.is_positive() and qq.is_square())
                        return ce/q * rubi111((be-q)/_ex2,ce,_ex_1,x) - ce/q * rubi111((be+q)/_ex2,ce,_ex_1,x);
                ex t = rubi121(qq, _ex0, _ex_1, x);
                return _ex_2 * t.subs(x == (_ex2*x*c+b));
        }

        // Rule 5
        // if ((a*(*_num4_p) - b*b/c).is_positive()) {
        //        ex t = rubi(power(_ex1-x*x/qq, pe), x);
        //        return power(_ex2*c*power(_ex_4*c/qq, pe), _ex_1) * t.subs(x == x*c*_ex2+b);
        //}

        // Fallback rule H
        return _ex0-power(x*x*c+x*b+a, p+(*_num1_p)) / (q*(p+(*_num1_p)) * power((_ex_2*x*c+q-b)/_ex2/q, p+(*_num1_p))) * _2F1(-pe, pe+_ex1, pe+_ex2, (_ex2*x*c+b+q)/_ex2/q);

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
