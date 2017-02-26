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
        if (is_exactly_a<numeric>(e) or is_exactly_a<constant>(e))
                return _ex0;
        if (is_exactly_a<symbol>(e))
                if (e.is_equal(x))
                        return _ex1;
                else
                        return _ex0;
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
        return rubi11(the_ex, x);
}

static ex rubi11(ex e, symbol x)
{
        if (is_exactly_a<power>(e)) {
                const power& p = ex_to<power>(e);
                const ex& ebas = p.op(0);
                const ex& m = p.op(1);
                if (ebas.is_equal(x))
                        if (is_exactly_a<numeric>(m)) {
                                if (ex_to<numeric>(m).is_minus_one())
                                        return log(x);
                                else
                                        return power(x,m+1) / (m+1);
                        }
                        else throw rubi_exception();

                if (not ebas.has_symbol(x) and m.degree(x)==1)
                if (ebas.degree(x)
        }
}
