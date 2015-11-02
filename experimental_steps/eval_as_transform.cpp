#include <iostream>
#include <boost/proto/proto.hpp>

using namespace boost;

template< typename I >  struct placeholder        { using arity = I; };
template< typename T >  struct placeholder_arity  { using type = typename T::arity; };


namespace transforms {

    using namespace proto;

    template< typename T >
    struct placeholder_arity_f : callable
    {
        using result_type = int;
        
        int operator()() const { return T::arity::value; }
    };

    struct eval_it : or_
        <   when
            <   terminal< placeholder<_> >
            ,   placeholder_arity<_value>()
            //,   call< placeholder_arity_f< placeholder_arity<_value> >() >
            >
        ,   when
            <   _
            ,   _default<eval_it>
            >
        >
    {};

}

using transforms::eval_it;

const proto::terminal< placeholder< mpl::int_<0> >>::type   _   = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3  = {{}};
const proto::terminal< placeholder< mpl::int_<1337> >>::type   _1337  = {{}};


int main()
{
    std::cout << eval_it{}(_3 + 3) << std::endl;
    auto expr  =  (proto::lit(1) + 2) * _3;
    std::cout << eval_it{}(expr) << std::endl;
}
