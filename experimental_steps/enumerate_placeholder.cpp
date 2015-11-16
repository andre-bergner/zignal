#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>

#include "callable_decltype.hpp"

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

   struct plain_eval : or_ < when<_, _default<plain_eval> > > {};

   struct binary_eval : callable_decltype
   {
      template< typename Tag ,  typename Left , typename Right >
      auto operator()( Tag , Left const & l , Right const & r ) const
      {
         return plain_eval{}(make_expr<Tag>( l , r ));
      }
   };

   struct evaluate : or_
   <  when
      <  terminal< placeholder >
      ,  place_the_holder( _value , _state )
      >
   ,  when
      <  binary_expr< _ , _ , _ >          // this should acually be an n-ary eval...
      ,  binary_eval( tag_of<_expr>()
                    , evaluate( _left, _state )
                    , evaluate( _right, mpl::plus< _state, count_placeholders(_left)>() )
                    )
      >
   ,  when
      <  _
      ,  _default< evaluate >
      >
   >
   {};


}

using  transforms :: evaluate;

const proto::terminal< placeholder >::type   _  = {{}};


int main()
{
   using namespace std;

   auto e = [](auto expr) { return evaluate{}( expr , mpl::int_<0>{} ); };

   cout << e( (1*_ + 0*_) + 0*_ ) << endl;
   cout << e( 1*_ + (0*_ + 0*_) ) << endl;
   cout << e( (0*_ + 1*_) + 0*_ ) << endl;
   cout << e( 0*_ + (1*_ + 0*_) ) << endl;
   cout << e( (0*_ + 0*_) + 1*_ ) << endl;
   cout << e( 0*_ + (0*_ + 1*_) ) << endl;

   cout << e( (1*_ + 0*_) + (0*_ + 1*_) ) << endl;
   cout << e( (1*_ + 0*_ + 0*_) + 1*_ ) << endl;
   cout << e( 1*_ + (0*_ + 0*_ + 1*_) ) << endl;
}

