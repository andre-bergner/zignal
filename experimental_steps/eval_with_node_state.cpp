//   -----------------------------------------------------------------------------------------------
//    Copyright 2015 André Bergner. Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//      --------------------------------------------------------------------------------------------

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
        >
    ,   when
        <   _
        ,   _default<eval_it>
        >
    >
    {};


    struct count_leaves : or_
    <   when
        <   terminal<placeholder<_>>
        ,   mpl::int_<1>()
        >
    ,   when
        <   terminal<_>
        ,   mpl::int_<1>()
        >
    ,   when
        <   unary_expr< _ , count_leaves>
        ,   count_leaves( _child )
        >
    ,   when
        <   binary_expr< _ , count_leaves, count_leaves >
        ,   mpl::plus< count_leaves(_left), count_leaves(_right) >()
        >
    >
    {};



    struct iplus : std::plus<int>, callable {};

    struct CountLeaves : or_
    <   when
        <   terminal<_>
        ,   mpl::int_<1>() >
    ,   otherwise
        <   fold<_, mpl::int_<0>(), iplus(CountLeaves, _state) > >
    >
    {};



    struct eval_it_inc : or_
    <   when
        <   terminal< placeholder<_> >
        ,   mpl::int_<0>()
        >
    ,   when
        <   terminal<_>
        ,   _value
        >
//    ,   when
//        <   plus< eval_it_inc, eval_it_inc >
//        ,   eval_it_inc( _left, mpl::plus<eval_it_inc(_right,_state),_state>() )
//        >
    ,   when
        <   plus< eval_it_inc, eval_it_inc >
        ,   iplus( iplus( eval_it_inc(_left) , eval_it_inc(_right) )
                 , mpl::plus< count_leaves(_left) , count_leaves(_right) >()
                 )
        >
        // TODO plus x y -> (x+y) * state++
        // plus( eval_it_inc(_left,state+1) , eval_it_inc(_right,eval_it_inc(_left,state+1).state+1) >
    ,   when
        <   _
        ,   _default<eval_it_inc>
        >
    >
    {};


}

using transforms::eval_it;
using transforms::count_leaves;
using transforms::CountLeaves;
using transforms::eval_it_inc;

const proto::terminal< placeholder< mpl::int_<0> >>::type   _   = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3  = {{}};
const proto::terminal< placeholder< mpl::int_<1337> >>::type   _1337  = {{}};


int main()
{
    std::cout << eval_it{}(_3 + 3) << std::endl;
    auto expr  =  (proto::lit(1) + 2) * _3;
    std::cout << eval_it{}(expr) << std::endl;

    std::cout << count_leaves{}(_+_+_) << std::endl;
    std::cout << CountLeaves{}( _+1+_+_+5, 1, 1 ) << std::endl;

    //std::cout << decltype(eval_it_inc{}( _+_+(_+_) ,mpl::int_<1>{} ))::value << std::endl;
    std::cout << eval_it_inc{}( _+_+(_+_)+(_+_+_) , mpl::int_<1>{} ) << std::endl;
    std::cout << eval_it_inc{}( (1+_)+_ ) << std::endl;
}
