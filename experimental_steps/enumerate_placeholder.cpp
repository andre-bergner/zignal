//   -----------------------------------------------------------------------------------------------
//    Copyright 2015 André Bergner. Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//      --------------------------------------------------------------------------------------------

#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>

#include "callable_decltype.hpp"
#include "tuple_tools.hpp"

namespace mpl = boost::mpl;
namespace proto = boost::proto;


//  This demo shows how haskell like self-enumerating placeholders (i.e. having a
//  generic placeholder _ that increments the input argument index on each usage)
//  can be implemented in boost.proto by passing along different transform states
//  on each branch of a binary operator.


struct placeholder {};


namespace transforms
{
   using namespace proto;


   template < typename Op >
   struct count_node_impl
   {
      struct apply : or_
      <  when
         <  Op
         ,  fold<_, mpl::int_<1>(), mpl::plus<apply, _state>()>
         >
      ,  when
         <  nary_expr<_, vararg<_>>
         ,  fold<_, mpl::int_<0>(), mpl::plus<apply, _state>()>
         >
      ,  otherwise< mpl::int_<0>() >
      >
      {};
   };

   template < typename Node >
   struct count_node_impl< terminal<Node> >
   {
      struct apply : or_
      <  when
         <  terminal< placeholder >
         ,  mpl::int_<1>()
         >
      ,  when
         <  nary_expr<_, vararg<_>>
         ,  fold<_, mpl::int_<0>(), mpl::plus<apply, _state>()>
         >
      ,  otherwise< mpl::int_<0>() >
      >
      {};
   };

   template < typename Node >
   using count_node = typename count_node_impl<Node>::apply;


   struct place_the_holder : callable_decltype
   {
      template< typename State >
      auto operator()( placeholder const & , State const & ) const
      {
         return  State::value;
      }
   };

   struct nary_eval;

   struct evaluate : or_
   <  when
      <  terminal< placeholder >
      ,  place_the_holder( _value , _state )
      >
   ,  when
      <  nary_expr<_, vararg<_>>
      ,  nary_eval( _expr, _state )
      >
   ,  when
      <  _
      ,  _default< evaluate >
      >
   >
   {};

   struct nary_eval : callable_decltype
   {
      template< typename Expr , typename State , std::size_t... Ns >
      auto apply( std::index_sequence<Ns...> , Expr const & x , State const & s ) const
      {
         using  tag = typename tag_of<Expr>::type;
         using  node_t = terminal<placeholder>;

         auto       n = std::make_tuple( count_node<node_t>{}(child_c<Ns>(x))... );
         evaluate   e;
         _default<> f;

         auto add = [](auto s, auto x) { return typename mpl::plus<decltype(s),decltype(x)>::type{} ; };
         auto m = std::tuple_cat( std::make_tuple(s) , scan( n, s, add ));

         return f(make_expr<tag>( e( child_c<Ns>(x) , std::get<Ns>(m) )... ));
      }

      template< typename Expr, typename State >
      auto operator()( Expr const & e , State const & s ) const
      {
         using idx_seq = std::make_index_sequence< proto::arity_of<Expr>::value >;
         return apply( idx_seq{}, e , s );
      }
   };

}

using  transforms :: evaluate;

const proto::terminal< placeholder >::type   _  = {{}};


int main()
{
   using namespace std;

   auto e = [](auto expr) { return evaluate{}( expr , mpl::int_<0>{} ); };


   std::cout << transforms:: count_node< proto::multiplies<proto::_,proto::_> >{}( 0 +_* _+_ *_*_[0] ) << std::endl;
   std::cout << transforms:: count_node< proto::terminal<placeholder> >{}( 0 +_* _+_ *_*_[0] ) << std::endl;

   std::cout << "--- expecing 0 ---" << std::endl;
   cout << e( (1*_ + 0*_) + 0*_ ) << endl;
   cout << e( 1*_ + (0*_ + 0*_) ) << endl;

   std::cout << "--- expecing 1 ---" << std::endl;
   cout << e( (0*_ + 1*_) + 0*_ ) << endl;
   cout << e( 0*_ + (1*_ + 0*_) ) << endl;

   std::cout << "--- expecing 2 ---" << std::endl;
   cout << e( (0*_ + 0*_) + 1*_ ) << endl;
   cout << e( 0*_ + (0*_ + 1*_) ) << endl;

   std::cout << "--- expecing 3 ---" << std::endl;
   cout << e( (1*_ + 0*_) + (0*_ + 1*_) ) << endl;
   cout << e( (1*_ + 0*_ + 0*_) + 1*_ ) << endl;
   cout << e( 1*_ + (0*_ + 0*_ + 1*_) ) << endl;
}

