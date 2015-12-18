#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>

#include "callable_decltype.hpp"
#include "demangle.h"
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

   struct count_placeholders : or_
   <  when
      <  terminal< placeholder >
      ,  mpl::int_<1>()
      >
   ,  when
      <  terminal<_>
      ,  mpl::int_<0>()
      >
   ,  when
      <  nary_expr<_, vararg<_>>
      ,  fold<_, mpl::int_<0>(), mpl::plus<count_placeholders, _state>()>
      >
   >
   {};

   struct place_the_holder : callable_decltype
   {
      template< typename State >
      auto operator()( placeholder const & , State const & ) const
      {
         return  State::value;
      }
   };

   struct plain_eval : when<_, _default<plain_eval> > {};

   struct binary_eval : callable_decltype
   {
      template< typename Tag ,  typename Left , typename Right >
      auto operator()( Tag , Left const & l , Right const & r ) const
      {
         return plain_eval{}(make_expr<Tag>( l , r ));
      }
   };

   struct binary_eval2;
   struct nary_eval;

   struct add : callable_decltype
   {
      template< typename L , typename R >
      auto operator()( L , R ) const
      {
         return  mpl::plus<L,R>{};
      }
   };

   struct evaluate : or_
   <  when
      <  terminal< placeholder >
      ,  place_the_holder( _value , _state )
      >
   //,  when
   //   <  terminal<_>
   //   ,  _value
   //   >
   //,  when
   //   <  binary_expr< _ , _ , _ >          // this should acually be an n-ary eval...
   //   ,  binary_eval( tag_of<_expr>()
   //                 , evaluate( _left, _state )
   //                 , evaluate( _right, mpl::plus< _state, count_placeholders(_left)>() )
   //                 )
   //   >
   //,  when
   //   <  binary_expr< _ , _ , _ >          // this should acually be an n-ary eval...
   //   ,  binary_eval2( _expr, _state )
   //   >
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
         using      t = typename tag_of<Expr>::type;
         auto       n = std::make_tuple( count_placeholders{}(child_c<Ns>(x))...);
         evaluate   e;
         plain_eval f;

         auto add = [](auto s, auto x) { return typename mpl::plus<decltype(s),decltype(x)>::type{} ; };
         auto m = std::tuple_cat( std::make_tuple(s) , scan( n, s, add ));

         return f(make_expr<t>( e( child_c<Ns>(x) , std::get<Ns>(m) )... ));
      }

      template< typename Expr, typename State >
      auto operator()( Expr const & e , State const & s ) const
      {
         using idx_seq = std::make_index_sequence< proto::arity_of<Expr>::value >;
         return apply( idx_seq{}, e , s );
      }
   };


   struct binary_eval2 : callable_decltype
   {
      template< typename Expr, typename State >
      auto operator()( Expr const & e , State const & s ) const
      {
         using tag = typename tag_of<Expr>::type;
         evaluate ev;
         using rs = mpl::plus<State, decltype(count_placeholders{}(_left{}(e))) >;
         auto l = ev(_left{}(e),s);
         auto r = ev(_right{}(e),rs{});
         return plain_eval{}(make_expr<tag>(l,r));
      }
   };


}

using  transforms :: evaluate;

const proto::terminal< placeholder >::type   _  = {{}};


int main()
{
   using namespace std;

   auto e = [](auto expr) { return evaluate{}( expr , mpl::int_<0>{} ); };

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

