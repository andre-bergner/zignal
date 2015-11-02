#include <iostream>
#include <array>
#include <cmath>
#include <boost/proto/proto.hpp>

using namespace std;
using namespace boost;


template< typename I >  struct placeholder        { using arity = I; };
template< typename T >  struct placeholder_arity  { using type = typename T::arity; };



namespace transforms {

    using namespace proto;

    template< typename X >
    struct max_fn : callable {
        using result_type = X;
        X operator()( X const & x , X const & y ) const { return std::max(x,y); }
    };

    template< typename X >
    struct min_fn : callable {
        using result_type = X;
        X operator()( X const & x , X const & y ) const { return std::min(x,y); }
    };

    template< typename X >
    struct zero_fn : callable {
        using result_type = X;
        X operator()() const { return {}; }
    };

    struct max_fn_int : max_fn<int> {};

    struct max_delay : or_
        <   when
            <   terminal<_>
            ,   call< zero_fn<int>() >
            >
        ,   when
            <   subscript< terminal<placeholder<mpl::int_<0>>> , terminal<int> >
            ,   _value(_right)      // TODO max_delay(right) + grammar: _[x]: x must be convertible to int !
            >
        ,   when
            <   unary_expr< _, max_delay >
            ,   max_delay(_child)
            >
        ,   when
            <   binary_expr< _, max_delay, max_delay >
            ,   call< min_fn<int>( max_delay(_left), max_delay(_right) ) >
            >
        >
    {};


    struct static_max_delay : or_
        <   when
            <   terminal<_>
            ,   mpl::int_<0>()
            >
        ,   when
            <   subscript< terminal<placeholder<mpl::int_<0>>> , terminal<placeholder<_>> >  // TODO prohibit _ (plac<0>) as arg
            ,   placeholder_arity< _value(_right) >()
            >
        ,   when
            <   unary_expr< _ , static_max_delay>
            ,   static_max_delay( _child )
            >
        ,   when
            <   binary_expr< _ , static_max_delay, static_max_delay >
            ,   mpl::max< static_max_delay(_left), static_max_delay(_right) >()
            >
        >
    {};

}

using transforms::max_delay;
using transforms::static_max_delay;

template< typename Expr >
using static_max_delay_int = typename boost::result_of<static_max_delay(Expr)>::type;


const proto::terminal< placeholder< mpl::int_<0> >>::type   _   = {{}};
const proto::terminal< placeholder< mpl::int_<1> >>::type   _1  = {{}};
const proto::terminal< placeholder< mpl::int_<2> >>::type   _2  = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3  = {{}};
const proto::terminal< placeholder< mpl::int_<4> >>::type   _4  = {{}};
const proto::terminal< placeholder< mpl::int_<1337> >>::type   _1337  = {{}};


int main()
{
    auto delay_times_1  =  _[_1] + 1.5 * _1  +  12 * _[-3];
    auto delay_times_2  =  _[-1337] + 1.5 * _1  +  12 * _[-3] + _[_3] + _[_4];
    auto static_delay_1 =  _[_3] + _[_1337];

    //proto::display_expr( delay_times_2 );

    //std::array< float, max_delay{}(delay_times_2) >

    //std::cout << max_delay{}(delay_times_2) << std::endl;
    std::cout << static_max_delay{}(delay_times_2) << std::endl;
    std::cout << static_max_delay{}(static_delay_1) << std::endl;

    std::cout << static_max_delay_int<decltype(static_delay_1)>::value << std::endl;

    using state_t = std::array< float, static_max_delay_int<decltype(static_delay_1)>::value >;
    state_t s;
}


