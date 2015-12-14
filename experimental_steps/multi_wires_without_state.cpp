#include <iostream>
#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>

#include "callable_decltype.hpp"
#include "tuple_tools.hpp"
#include "demangle.h"


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
   // block composition operators -- TODO needs correct grammar as arguments

   using channel_operator  = comma< _ , _ >;
   using sequence_operator = bitwise_or_assign< _ , _ >;

}

using building_blocks :: placeholder;
using building_blocks :: placeholder_arity;

using building_blocks :: channel_operator;
using building_blocks :: sequence_operator;


BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );



namespace transforms
{
   using namespace proto;

   //  ---------------------------------------------------------------------------------------------
   // static analysis transforms  --  helpers that inspect structure of expression
   //  ---------------------------------------------------------------------------------------------

   //   output arity of expression
   //
   //   #out_r + max( 0 , #out_l - in_r )
   //
   //   •——→•——→      •——→•——→       •—→•—→        •—→•—→
   //   •——→•         •——→•——→       •—→•—→        •—→•
   //   •——————→      •——————→           —→           •
   //

   struct output_arity;

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
      <  sequence_operator
      ,  mpl::plus
         <  input_arity(_left)
         ,  mpl::max
            <  mpl::int_<0>
            ,  mpl::minus< input_arity(_right) , output_arity(_left) >
            >
         >()
      >
   ,  when
      <  nary_expr<_, vararg<_>>
      ,  fold<_, mpl::int_<0>(), mpl::max<input_arity, _state>()>
      >
   >
   {};

   struct output_arity : or_
   <  when
      <  channel_operator
      ,  mpl::plus< output_arity(_left) , output_arity(_right) >()
      >
   ,  when
      <  sequence_operator
      ,  mpl::plus
         <  output_arity(_right)
         ,  mpl::max
            <  mpl::int_<0>
            ,  mpl::minus< output_arity(_left) , input_arity(_right) >
            >
         >()
      >
   ,  when
      <  _
      ,  mpl::int_<1>()
      >
   >
   {};


   template< typename Expr >
   using input_arity_t = typename boost::result_of<input_arity(Expr)>::type;


   //  ---------------------------------------------------------------------------------------------

   struct place_the_holder;
   struct sequence;
   struct collect_wires;


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
      <  channel_operator    //  grammar rule: only allowed after sequence or recursion at top of sub-tree
      ,  collect_wires( eval_it(_left), eval_it(_right) )
      >
   ,  when
      <  _
      ,  _default< eval_it >
      >
   >
   {};


   //-----------------------------------------------------------------------------------------------

   struct place_the_holder : callable_decltype
   {
      template< typename I , typename S , typename Tuple >
      auto operator()( placeholder<I> const & , S , Tuple const & args ) const
      {
         return std::get<I::value-1>( args );
      }
   };

   struct sequence : callable_decltype
   {
      template< typename LeftExpr , typename RightExpr , typename Input >
      auto operator()( LeftExpr const & l , RightExpr const & r , Input const & input ) const
      {
         auto e = eval_it{};
         auto left_result = flatten_tuple( std::make_tuple(e( l, mpl::int_<0>{}, ( current_input = input ))));
         return e( r, mpl::int_<0>{}, ( current_input = left_result ) );
      }
   };

   struct collect_wires : callable_decltype
   {
      template< typename Left , typename Right >
      auto operator()( Left const & l , Right const & r ) const
      {
         return std::make_tuple( l, r );
      }      
   };

}


using  transforms :: input_arity;
using  transforms :: input_arity_t;
using  transforms :: output_arity;
using  transforms :: eval_it;


//  ------------------------------------------------------------------------------------------------
// compile  --  main function of framework
//              • takes an expression
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
   auto call_impl( mpl::int_<0> , Args const &... args ) const -> decltype(auto)
   {
      auto result = eval_it{}( expr_, mpl::int_<0>{}, ( current_input = std::make_tuple(args...) ) );
      return flatten_tuple( std::make_tuple( result ));
   }

   template< int arg_diff , typename... Args >
   auto call_impl( mpl::int_<arg_diff> , Args const &... args ) const
   {
      return [ args...
             , expr = *this ]           // TODO should not be a lambda, but a type that is
      ( auto const &... missing_args )  //      aware of the # of args (enable_if them)
      {
         return expr( args..., missing_args... );
      };
   }

public:

   stateful_lambda( Expression expr ) : expr_( std::move(expr)) {}

   template< typename... Args , typename = std::enable_if_t< sizeof...(Args) <= arity > >
   auto operator()( Args const &... args ) const -> decltype(auto)
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
   auto print_ins_and_outs = []( auto const & expr )
   {
       std::cout << "--------------------------" << std::endl;
       //proto::display_expr( expr );
       std::cout << "#ins:  " << input_arity{} (expr) << std::endl;
       std::cout << "#outs: " << output_arity{}(expr) << std::endl;
       std::cout << std::endl;
   };


    
   auto seq_expr =    ( _2 + _1 , _2 ) 
                   |= ( _1*_2 - 100 )
                   ;

   print_ins_and_outs( seq_expr );
   auto seq = compile( seq_expr );

   auto t = compile( _1 |= _1 );
   t(1);

   std::cout << std::get<0>(seq(1337,3)) << std::endl;
}
