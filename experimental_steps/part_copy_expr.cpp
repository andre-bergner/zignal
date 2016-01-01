//   -----------------------------------------------------------------------------------------------
//    Copyright 2015 André Bergner. Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//      --------------------------------------------------------------------------------------------

#include <iostream>
#include <array>
#include <boost/mpl/int.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>

#include "tuple_tools.hpp"
#include "callable_decltype.hpp"
#include "demangle.h"

namespace proto = boost::proto;
namespace mpl   = boost::mpl;

template < typename I >  struct placeholder       { using arity = I; };
template < typename T >  struct placeholder_arity { using type = typename T::arity; };

template < typename T >
auto operator<<( std::ostream& os , placeholder<T> const & ) -> std::ostream&
{
   return os << T{};
}






template < typename >
struct is_placeholder : std::false_type {};



template< typename E >
struct copy_expr;

struct copy_generator : proto::pod_generator< copy_expr > {};

struct copy_domain : proto::domain< copy_generator >
{
   template < typename T >
   struct as_child : std::conditional_t
                     <  true//proto::is_expr<std::decay_t<T>>::value && !is_placeholder<std::decay_t<T>>::value
                     ,  proto_base_domain::as_expr<T>
                     ,  proto_base_domain::as_child<T>
                     > {/*
                        as_child() {
                           std::cout << "• as child: " << type_name<T>() << std::endl;
                           std::cout << "• place :   " << is_placeholder<std::decay_t<T>>::value << std::endl;
                        }*/
                     };

   template < typename T >
   struct as_expr : proto_base_domain::as_expr<T> 
                    { as_expr() { std::cout << "• as expr: " << type_name<T>() << std::endl; } };
};

template< typename Expr >
struct copy_expr
{
   BOOST_PROTO_EXTENDS( Expr, copy_expr<Expr>, copy_domain )
};



const proto::literal< placeholder< mpl::int_<1> >, copy_domain >   _1  = {};
const proto::literal< placeholder< mpl::int_<2> >, copy_domain >   _2  = {};
const proto::literal< placeholder< mpl::int_<3> >, copy_domain >   _3  = {};
const proto::literal< placeholder< mpl::int_<4> >, copy_domain >   _4  = {};
const proto::literal< placeholder< mpl::int_<5> >, copy_domain >   _5  = {};
const proto::literal< placeholder< mpl::int_<6> >, copy_domain >   _6  = {};

template < typename Int >
struct is_placeholder< proto::literal< placeholder<Int>, copy_domain > > : std::true_type {};



const proto::terminal< placeholder< mpl::int_<1> >>::type   _1_  = {{}};
const proto::terminal< placeholder< mpl::int_<2> >>::type   _2_  = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3_  = {{}};
const proto::terminal< placeholder< mpl::int_<4> >>::type   _4_  = {{}};
const proto::terminal< placeholder< mpl::int_<5> >>::type   _5_  = {{}};
const proto::terminal< placeholder< mpl::int_<6> >>::type   _6_  = {{}};


auto&& ref_holder = std::tuple
<  const proto::literal< placeholder< mpl::int_<1> >>
,  const proto::literal< placeholder< mpl::int_<2> >>
,  const proto::literal< placeholder< mpl::int_<3> >>
,  const proto::literal< placeholder< mpl::int_<4> >>
,  const proto::literal< placeholder< mpl::int_<5> >>
,  const proto::literal< placeholder< mpl::int_<6> >>
>{};








BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );

namespace transforms
{
   using namespace proto;

   struct make_expr_ : callable_decltype
   {
      template< typename Tag , typename... Args >
      auto operator()( Tag , Args&&... args ) const
      {
         //auto t = std::make_tuple(args...);
         //std::cout << "• make…expr: " << t << std::endl;
         std::cout << "• make…expr" << std::endl;
         return make_expr<Tag>( std::forward<Args>(args)... );
      }
   };

   struct as_child_ : callable_decltype
   {
      template < typename T >
      auto operator()( T&& t ) const
      {
         std::cout << "• terminal: " << t << std::endl;
         return std::forward<T>(t);
         //return t;
      }

      template < typename Int >
      auto operator()( placeholder<Int> p) const
      {
         std::cout << "• terminal _x" << std::endl;
         return std::get<Int::value-1>(ref_holder);
      }
   };


   struct rebuild : or_
   <  when
      <  terminal<_>
      //,  as_child_(_value)
      ,  _value
      >
   ,  when
      <  _
      ,  make_expr_( tag_of<_expr>(), rebuild(proto::pack(_))... )
      >
   >
   {};


   template < typename T >
   using in_t = T const &;

   struct place_the_holder : callable_decltype
   {
      template < typename I , typename Tuple >
      auto operator()( placeholder<I> const & , Tuple const & args ) const
      {
         return std::get<I::value-1>( args );
      }
   };


   struct eval_it : or_
   <  when
      <  terminal< placeholder<_> >
      ,  place_the_holder( _value , _env_var<current_input_t> )
      >
   ,  when
      <  _
      ,  _default< eval_it >
      >
   >
   {};

}

using  transforms :: rebuild;
using  transforms :: eval_it;



auto compile = []( auto const & expr_ )        // TODO need to define value_type for state
{
   auto expr = proto::deep_copy(expr_);
   using expr_t = decltype(expr);

   return [expr]( auto&&... xs )
   {
      return eval_it{}( expr, 0, ( current_input = std::forward_as_tuple(xs...) ) );
   };
};







inline void escape(void* p) { asm volatile( "" :: "g"(p) : "memory" ); }

int main()
{
   using namespace std;
   using namespace proto;
   namespace t = proto::tag;

   rebuild r;

   //auto x = _1 + 1337 + _2 + 10 + _2*_1 - 100*_2;
   //auto x = std::get<0>(ref_holder) + 1337 + std::get<0>(ref_holder);
   auto x = _1 + 1337 + _2 + 42 + _1*_2 - 17*_1*_2;
   auto x2 = r(x);
   //auto x3 = make_expr<t::minus>( make_expr<t::plus>( make_expr<t::plus>(_1_,1337) , _2_), 42);

   std::cout << "sizeof(x):  " << sizeof(x) << std::endl;
   std::cout << "sizeof(x2): " << sizeof(x2) << std::endl;
   //std::cout << "sizeof(x3): " << sizeof(x3) << std::endl;

   //std::cout << is_terminal<decltype(_1)>::value << std::endl;
   //std::cout << is_terminal<decltype(_1+2)>::value << std::endl;

   auto f = compile(x);
   std::cout << f(11,-1337) << std::endl;
   auto g = compile(x2);
   std::cout << g(11,-1337) << std::endl;

   //std::cout << "-----" << std::endl;
   //std::cout << type_name(x) << std::endl;
   //std::cout << "-----" << std::endl;
   //std::cout << type_name(x2) << std::endl;
   //std::cout << "-----" << std::endl;
   //std::cout << type_name(x3) << std::endl;

   //display_expr( x );
   //proto::display_expr( x2 );
}



