bool rubi_integrate(ex e, ex x, ex& ret)
{
        try {
                ret = rubi(e, x);
        catch (rubi_exception) {
                return false;
        }
        return true;
}

static ex rubi(ex e, ex x)
{
        if (not is_exactly_a<symbol>(x))
                throw std::runtime_error("rubi(): not a symbol");
        if (not has_symbol(e))
                return e*x;
        if (e.is_equal(x))
                return power(x, _ex2) / _ex2;
        ex the_ex = e.expand();
        if (is_exactly_a<add>(the_ex)) {
                const add& m = ex_to<add>(the_ex);
                exvector vec;
                for (unsigned int i=0; i<m.nops(); i++)
                        vec.push_back(rubi(m.op(i)), x);
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
                if (ebas.has_symbol(x) and not m.has_symbol(x)) {
                        if (ebas.is_linear(x, a, b))
                                return rubi111(a, b, m, x);
                        else if (ebas.is_quadratic(x, a, b, c))
                                return rubi121(a, b, c, m, x);
                        else
                                return rubi1x1(ebas, m, x);
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

static ex rubi121(ex ae, ex be, ex ce, ex pe, ex x)
{
        if (not is_exactly_a<numeric>(ae)
            or not is_exactly_a<numeric>(be)
            or not is_exactly_a<numeric>(ce)
            or not is_exactly_a<numeric>(pe))
                throw rubi_exception();

        const numeric& a = ex_to<numeric>(ae);
        const numeric& b = ex_to<numeric>(be);
        const numeric& c = ex_to<numeric>(ce);
        const numeric& p = ex_to<numeric>(pe);
        
        if (b*b - a*c*(*_num4_p)).is_zero()) {
                if (p.is_equal(*_num_1_2_p)) {
                        return (x*c + b/(*_num2_p)) * power(x*x*c+x*b+a, _ex_1_2) * rubi111(b/(*_num2_p), c, *_num_1_p, x);
                }
                return (x*c*_ex2 + b) * power(x*x*c+x*b+a, p) / c / _ex2 / (_ex2*p+_ex1);
        }

        if (not (p*(*_num4_p)).is_integer()) {
                numeric qq = b*b - a*c*(*_num4_p));
                ex q;
                if (qq.is_negative())
                        q = power(ex(qq), _ex1_2);
                else
                        q = ex
        }
}
