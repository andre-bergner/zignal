///////////////////////////////////////////////////////////////////////////////
// Copyright 2015 André Bergner. adapted from:
// Copyright 2008 Eric Niebler. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This example builds a simple but functional lambda library using Proto.

#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>

//  TODO 
//  √ use callable transform instead of context
//  √ implement currying (partially, see TODO)
//  √ operator() -> enable if # of args


namespace mpl = boost::mpl;
namespace proto = boost::proto;



template<typename T>
struct lambda;

struct lambda_domain
  : proto::domain<proto::pod_generator<lambda> >
{};




template< typename I >  struct placeholder       { using arity = I; };
template< typename T >  struct placeholder_arity { using type = typename T::arity; };


namespace transforms
{
    using namespace proto;

    struct lambda_arity : or_
    <   when
        <   terminal< placeholder<_> >
        ,   mpl::next<placeholder_arity<_value>>()
        >
    ,   when
        <   terminal<_>
        ,   mpl::int_<0>()
        >
    ,   when
        <   nary_expr<_, vararg<_>>
        ,   fold<_, mpl::int_<0>(), mpl::max<lambda_arity, _state>()>
        >
    >
    {};


    // boost.proto (similar to functional from the STL) requires a result
    // type or trait to be declared within function objects. This helper
    // works around this design limitation of boost.proto by automatically
    // deriving the result type by using decltype on the object itself.

    struct callable_decltype : callable
    {
        template< typename Signature >
        struct result;

        template< typename This , typename... Args >
        struct result< This( Args... ) > 
        {
            using type = decltype( std::declval<This>()( std::declval<Args>()... ));
        };
    };



    struct place_the_holder : callable_decltype
    {
        template< typename I , typename Tuple >
        auto operator()( placeholder<I> const & , Tuple const & args ) const
        {
            return std::get<I::value>( args );
        }
    };

    struct eval_it : or_
    <   when
        <   terminal< placeholder<_> >
        ,   place_the_holder( _value , _state )
        >
    ,   when
        <   _
        ,   _default< eval_it >
        >
    >
    {};

}

using  transforms :: lambda_arity;
using  transforms :: eval_it;



template<typename T>
struct lambda
{
    BOOST_PROTO_BASIC_EXTENDS(T, lambda<T>, lambda_domain)
    BOOST_PROTO_EXTENDS_ASSIGN()
    BOOST_PROTO_EXTENDS_SUBSCRIPT()

    static int const arity = boost::result_of<lambda_arity(T)>::type::value;

private:

    template< typename... Args , typename = std::enable_if_t< arity == sizeof...(Args) > >
    auto call_impl( mpl::int_<0> , Args const &... args ) const -> decltype(auto)
    {
        std::tuple<Args const & ...>  args_tuple( args... );
        return eval_it{}( *this, args_tuple );
    }

    // Define our operator () that evaluates the lambda expression.
    template< int arg_diff , typename... Args >
    auto call_impl( mpl::int_<arg_diff> , Args const &... args ) const
    {
        return [ &args...                 // TODO should copy args into lambda
               , expr = *this ]           // TODO should not be a lambda, but a type that is
        ( auto const &... missing_args )  //      aware of the # of args (enable_if them)
        {
            return expr( args..., missing_args... );
        };
    }

public:

    // Define our operator () that evaluates the lambda expression.
    template< typename... Args , typename = std::enable_if_t< sizeof...(Args) <= arity > >
    auto operator()( Args const &... args ) const -> decltype(auto)
    {
        return call_impl( mpl::int_< arity - sizeof...(Args) >{} , args... );
    }
};

// Define some lambda placeholders
lambda<proto::terminal< placeholder<mpl::int_<0>> >::type> const _1 = {{}};
lambda<proto::terminal< placeholder<mpl::int_<1>> >::type> const _2 = {{}};
lambda<proto::terminal< placeholder<mpl::int_<2>> >::type> const _3 = {{}};
lambda<proto::terminal< placeholder<mpl::int_<3>> >::type> const _4 = {{}};
lambda<proto::terminal< placeholder<mpl::int_<4>> >::type> const _5 = {{}};




template<typename T>
lambda<typename proto::terminal<T>::type> const val(T const &t) { return {{t}}; }

template<typename T>
lambda<typename proto::terminal<T &>::type> const var(T &t) { return {{t}}; }


int main()
{
    int i = ( (_1 + 2) / 4 )(42);
    std::cout << i << std::endl; // prints 11

    int j = ( (-(_1 + 2)) / 4 )(42);
    std::cout << j << std::endl; // prints -11

    double d = ( (4 - _2) * 3 )(42, 3.14);
    std::cout << d << std::endl; // prints 2.58

    auto d2 = ( (4 - _2) * 3 )(42)()(3.14);
    std::cout << d2 << std::endl; // prints 2.58

    // check non-const ref terminals
    (std::cout << _1 << " -- " << _2 << '\n')(42, "Life, the Universe and Everything!");
    // prints "42 -- Life, the Universe and Everything!"

    int k = (val(1) + val(2))();
    std::cout << k << std::endl; // prints 3

    // check array indexing for kicks
    int integers[5] = {0};
    (var(integers)[2] = 2)();
    (var(integers)[_1] = _1)(3);
    std::cout << integers[2] << std::endl; // prints 2
    std::cout << integers[3] << std::endl; // prints 3

}
