#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>

#include "callable_decltype.hpp"
#include "demangle.h"

//  This step implements wires between lambda expressions, mono wires only.
//  I.e. it's possible to have many inputs into the first expression, but one
//  output per expression only. Thus, all successive expression may have one
//  input argument only.


namespace mpl = boost::mpl;
namespace proto = boost::proto;


namespace building_blocks
{
   using namespace proto;

   //  ---------------------------------------------------------------------------------------------
   // definition of the building blocks of the language
   //  ---------------------------------------------------------------------------------------------

   template< typename I >  struct placeholder       { using arity = I; };
   template< typename T >  struct placeholder_arity { using type = typename T::arity; };

   //  ---------------------------------------------------------------------------------------------
   // block composition operators

   using sequence_operator  = shift_right< _ , _ >;    // TODO needs correct grammar as arguments

}

using building_blocks :: placeholder;
using building_blocks :: placeholder_arity;
using building_blocks :: sequence_operator;


BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );



namespace transforms
{
   using namespace proto;

   //  ---------------------------------------------------------------------------------------------
   // static analysis transforms  --  helpers that inspect structure of expression
   //  ---------------------------------------------------------------------------------------------

   struct input_arity : or_
   <  when
      <  terminal< placeholder<_> >
      ,  placeholder_arity<_value>()
      >
   ,  when
      <  terminal<_>
      ,  mpl::int_<0>()
      >
   ,  when
      <  nary_expr<_, vararg<_>>
      ,  fold<_, mpl::int_<0>(), mpl::max<input_arity, _state>()>
      >
   >
   {};

   struct place_the_holder : callable_decltype
   {
      template< typename I , typename S , typename Tuple >
      auto operator()( placeholder<I> const & , S , Tuple const & args ) const
      {
         //std::cout << "s: " << S::value << std::endl;
         return std::get<I::value-1>( args );
      }
   };



   struct sequence;

   struct eval_it : or_
   <  when
      <  terminal< placeholder<_> >
      ,  place_the_holder( _value , _state , _env_var<current_input_t> )
      >
   ,  when
      <  sequence_operator
      ,  sequence( _left , _right , _env_var<current_input_t> )
      >
   ,  when
      <  _
      ,  _default< eval_it >
      >
   >
   {};


   struct sequence : callable_decltype
   {
      template< typename LeftExpr , typename RightExpr , typename Tuple >
      auto operator()( LeftExpr const & l , RightExpr const & r , Tuple const & args ) const
      {
         auto e = eval_it{};
         auto left_result = e( l, mpl::int_<0>{}, ( current_input = args ) );
         return e( r, mpl::int_<0>{}, ( current_input = std::make_tuple(left_result) ) );
      }
   };

}

using  transforms :: input_arity;
using  transforms :: eval_it;


template< typename Expr >
using input_arity_t = typename boost::result_of<input_arity(Expr)>::type;


//  ------------------------------------------------------------------------------------------------
// compile  --  main function of framework
//              • takes an expressions
//              • returns clojure
//  ------------------------------------------------------------------------------------------------

template
<  typename Expression
,  size_t   arity
>
struct stateful_lambda
{
private:

   Expression  expr_;

   template< typename... Args >
   auto call_impl( mpl::int_<0> , Args const &... args ) -> decltype(auto)
   {
      return eval_it{}( expr_, proto::_state{}, ( current_input = std::make_tuple(args...) ) );
   }

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

   stateful_lambda( Expression expr ) : expr_( std::move(expr)) {}

   template< typename... Args , typename = std::enable_if_t< sizeof...(Args) <= arity > >
   auto operator()( Args const &... args ) -> decltype(auto)
   {
      return call_impl( mpl::int_< arity - sizeof...(Args) >{} , args... );
   }
};


auto compile = []( auto expr )        // TODO need to define value_type for state
{
   using expr_t = decltype(expr);
   using arity_t = input_arity_t<expr_t>;

   return stateful_lambda< expr_t, arity_t::value>{ expr };
};


const proto::terminal< placeholder< mpl::int_<1> >>::type   _1  = {{}};
const proto::terminal< placeholder< mpl::int_<2> >>::type   _2  = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3  = {{}};
const proto::terminal< placeholder< mpl::int_<4> >>::type   _4  = {{}};
const proto::terminal< placeholder< mpl::int_<5> >>::type   _5  = {{}};



int main()
{
   auto seq_expr1 = ( (_1 + _2 - 37) >> _1*2 >> _1 - 604 );    // left associative
   auto seq_expr2 = ( (_1 + _2 - 37) >> (_1*2 >> _1 - 604) );  // right associative
   auto seq1 = compile( seq_expr1 );
   auto seq2 = compile( seq_expr2 );
   std::cout << seq1(1337,2) << std::endl;
   std::cout << seq2(1337,2) << std::endl;

   /*{
      auto seq = compile( _1 - 37 >> _1*2 );    // produces:
      __asm__ __volatile__("nop":::"memory");   // movl  $1337, %esi
      auto x = seq(1337);                       // subl  -108(%rbp), %esi
      __asm__ __volatile__("nop":::"memory");   // imull -112(%rbp), %esi
      std::cout << x << std::endl;
   }*/
}
