#include <iostream>
#include <algorithm>
#include <array>
#include <vector>
#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/ostream.hpp>
#include <boost/typeof/std/iostream.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>

#include "demangle.h"



namespace mpl = boost::mpl;
namespace proto = boost::proto;

//      Recursion Operator
//
//      a <= b    pre-conditions: a,b are callable,
//                                #input(a) >= #outputs(b)
//                                #input(b) >= #outputs(a)
//
//                    state                  input
//       _1           0                      1
//      (_1 <= _1)    1 (recursion delay)    0 (placeholders called mutually inside recursion)
//      (_2 <= _1)    1 (recursion delay)    2 (one is swallowed by recursion)
//      (sin <= cos)  
//      (sin(_1) <= cos(_1))    identical to prev.
//      (_1 + sin <= cos)       should not work
//
//      (_1 <= _1 <= _1)  equal to  ((_1 <= _1) <= _1)
//                    does not work: _1 <= _1 is a generator (no input)

//      _1/2 | _1[_4],_1[_2] | _1 + _2  ==  ( x[n-4] + x[n-2] ) / 2
//
//
//




namespace boost { namespace proto
{
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
}}


//  ------------------------------------------------------------------------------------------------
// main part
//  ------------------------------------------------------------------------------------------------




namespace building_blocks
{
   using namespace proto;

   //  ---------------------------------------------------------------------------------------------
   // definition of the building blocks of the language
   //  ---------------------------------------------------------------------------------------------

   template< typename I >  struct placeholder       { using arity = I; };
   template< typename T >  struct placeholder_arity { using type = typename T::arity; };


   template< typename idx = _ >
   using delayed_placeholder = subscript< terminal<placeholder<idx>> , terminal<placeholder<_>> >;


   //  ---------------------------------------------------------------------------------------------
   // block composition operators

   using parallel_operator  = comma< _ , _ >;          // TODO needs correct grammar as arguments
   using sequence_operator  = shift_right< _ , _ >;    // TODO needs correct grammar as arguments
   using recursion_operator = shift_left< _ , _ >;     // TODO needs correct grammar as arguments
//   using sequence_operator  = bitwise_or< _ , _ >;     // TODO needs correct grammar as arguments
//   using recursion_operator = less_equal< _ , _ >;     // TODO needs correct grammar as arguments

}

using building_blocks :: placeholder;
using building_blocks :: placeholder_arity;
using building_blocks :: delayed_placeholder;

using building_blocks :: parallel_operator;
using building_blocks :: sequence_operator;
using building_blocks :: recursion_operator;


BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );
BOOST_PROTO_DEFINE_ENV_VAR( delayed_input_t, delayed_input );



namespace transforms
{
   using namespace proto;

   //  ---------------------------------------------------------------------------------------------
   // static analysis transforms  --  helpers that inspect structure of expression
   //  ---------------------------------------------------------------------------------------------

   struct output_arity;

   struct input_arity : or_
   <  when
      <  delayed_placeholder<>
      ,  placeholder_arity<_value(_left)>()
      >
   ,  when
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

    //   #out_r + max( 0 , #out_l - in_r )
    //
    //   •——→•——→      •——→•——→       •—→•—→        •—→•—→
    //   •——→•         •——→•——→       •—→•—→        •—→•
    //   •——————→      •——————→           —→           •
    //

   struct output_arity : or_
   <  when
      <  comma< output_arity, output_arity >
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
   //,  when
   //   <  sink
   //   ,  mpl::int_<0>
   //   >
   ,  when
      <  _
      ,  mpl::int_<1>()
      >
   >
   {};




   template< typename input_channel >
   struct static_max_delay_impl   // excapsulates template parameter, proto gets confused elsewise
   {
      struct apply : or_
      <  when
         <  delayed_placeholder<input_channel>
         ,  placeholder_arity< _value(_right) >()
         >
      ,  when
         <  terminal<_>
         ,  mpl::int_<0>()
         >
      ,  when
         <  nary_expr<_, vararg<_>>
         ,  fold<_, mpl::int_<0>(), mpl::max<apply, _state>()>
         >
      >
      {};
   };

   template< typename input_channel = _ >
   using static_max_delay = typename static_max_delay_impl<input_channel>::apply;



   struct count_recursion : or_
   <  when
      <  recursion_operator
      ,  mpl::int_<1>()
      >
   ,  when
      <  terminal<_>
      ,  mpl::int_<0>()
      >
   ,  when
      <  nary_expr<_, vararg<_>>
      ,  fold<_, mpl::int_<0>(), mpl::plus<count_recursion, _state>()>
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


   struct place_delay : callable_decltype
   {
      template< typename I , typename J , typename S , typename Delayed_input >
      auto operator()( placeholder<I> const & , placeholder<J> const & , S , Delayed_input const & del_in ) const
      {
         //std::cout << "s: " << S::value << std::endl;
         auto const & s = std::get<I::value-1>(del_in);
         return s[s.size()-J::value];
      }
   };




   struct sequence;


   struct plain_eval : or_ < when<_, _default<plain_eval> > > {};

   struct binary_eval : callable_decltype
   {
      template< typename Tag ,  typename Left , typename Right >
      auto operator()( Tag , Left const & l , Right const & r ) const
      {
         return plain_eval{}(make_expr<Tag>( l , r ));
      }
   };

   struct eval_it : or_
   <  when
      <  delayed_placeholder<>
      ,  place_delay( _value(_left) , _value(_right) , _state , _env_var<delayed_input_t> )
      >
   ,  when
      <  terminal< placeholder<_> >
      ,  place_the_holder( _value , _state , _env_var<current_input_t> )
      >
   ,  when
      <  sequence_operator
      ,  sequence( _left , _right , _env_var<current_input_t> )
      >
   ,  when
      <  binary_expr< _ , _ , _ >
      ,  binary_eval( tag_of<_expr>()
                    , eval_it( _left, _state )
                    , eval_it( _right, mpl::plus<_state,count_recursion(_left)>() )
                    )
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
using  transforms :: output_arity;

using  transforms :: static_max_delay;
using  transforms :: eval_it;



template< typename idx , typename Expr >
using static_max_delay_t = typename boost::result_of<static_max_delay<idx>(Expr)>::type;

template< typename Expr >
using input_arity_t = typename boost::result_of<input_arity(Expr)>::type;




template< size_t arity, typename Expr >
struct delay_for_placeholder
{
   using type = decltype( std::tuple_cat( std::declval<typename delay_for_placeholder<arity-1,Expr>::type>()
                                        , std::declval<std::tuple< static_max_delay_t<mpl::int_<arity>,Expr>>>() ));
};

template< typename Expr >
struct delay_for_placeholder<0,Expr>
{
   using type = std::tuple<>;
};




//  ------------------------------------------------------------------------------------------------
// supporting tools  --  needed for managing state types
//  ------------------------------------------------------------------------------------------------



//  ------------------------------------------------------------------------------------------------
// lift_into_tuple  --  lifts a meta-function into a tuple,
//                      i.e. applies it on each type in tuple, returns tuple of new types

template< typename F , typename Tuple >
struct lift_into_tuple;

template< typename F , typename... Ts >
struct lift_into_tuple< F , std::tuple<Ts...> >
{
   using type = std::tuple< typename F::template apply_t<Ts>... >;
};

template< typename F , typename Tuple >
using lift_into_tuple_t = typename lift_into_tuple<F,Tuple>::type;



//  ------------------------------------------------------------------------------------------------
// to_array  --  meta-function that maps mpl::int_<N> to array<T,N>

template< typename T >
struct to_array
{
   template< typename Int >
   struct apply
   {
      using type = std::array< T , Int::value >;
   };

   template< typename Int >
   using apply_t = typename apply<Int>::type;
};



//  ------------------------------------------------------------------------------------------------
// tuple_for_each  --  apply n-ary function on each value in list of n tuples of same size

template< size_t N , size_t n = 0 >
struct tuple_for_each_impl
{
   template< typename F, typename... Tuples >
   static void apply( F&& f, Tuples&&... tuples )
   {
      f( std::get<n>(tuples)... );
      tuple_for_each_impl<N,n+1>::apply( std::forward<F>(f), std::forward<Tuples>(tuples)... );
   }
};

template< size_t N >
struct tuple_for_each_impl<N,N>
{
   template< typename F, typename... Tuples >
   static void apply( F&& , Tuples&&... ) { }
};


template< typename F, typename... Ts , typename... Tuples >
// requires (sizeof...(Ts) == tuple_size(Tuples))...
void tuple_for_each( F&& f , std::tuple<Ts...>& t , Tuples&&... tuples )
{
   tuple_for_each_impl< sizeof...(Ts) >::apply( std::forward<F>(f), t, std::forward<Tuples>(tuples)... );
}


//  ------------------------------------------------------------------------------------------------
// very simple delay_line operation on std::array --> TODO should be own type static_delay_line !

struct {
   template< typename T , size_t N , typename Y >
   void operator()( std::array<T,N>& xs , Y y )
   {
      for ( size_t n = 1 ; n < xs.size() ; ++n )  xs[n-1] = xs[n];
      xs.back() = y;
   }
} rotate_push_back;




//  ------------------------------------------------------------------------------------------------
// compile  --  main function of framework
//              • takes a zignal/dsp expressions
//              • generate state
//              • returns clojure
//  ------------------------------------------------------------------------------------------------

template
<  typename Expression
,  typename State
,  size_t   arity
>
struct stateful_lambda
{
private:

   Expression  expr_;
   State       state_;

   template< typename... Args >
   auto call_impl( mpl::int_<0> , Args const &... args ) -> decltype(auto)
   {
      auto result = eval_it{}( expr_, mpl::int_<0>{}, ( current_input = std::make_tuple(args...) , delayed_input = state_ ) );
      tuple_for_each( rotate_push_back, state_, std::make_tuple(args...) );
      return result;
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
   using tuple_t = typename delay_for_placeholder< arity_t::value, expr_t >::type;
   using state_t = lift_into_tuple_t< to_array<float> , tuple_t >;

   return stateful_lambda< expr_t, state_t, arity_t::value>{ expr };
};







const proto::terminal< placeholder< mpl::int_<1> >>::type   _1  = {{}};
const proto::terminal< placeholder< mpl::int_<2> >>::type   _2  = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3  = {{}};
const proto::terminal< placeholder< mpl::int_<4> >>::type   _4  = {{}};
const proto::terminal< placeholder< mpl::int_<5> >>::type   _5  = {{}};



int main()
{
   std::cout << "arities -----------------" << std::endl;

   auto print_ins_and_outs = []( auto const & expr )
   {
       std::cout << "--------------------------" << std::endl;
       //proto::display_expr( expr );
       std::cout << input_arity{} (expr) << std::endl;
       std::cout << output_arity{}(expr) << std::endl;
   };


   auto expr1 =  ( _1 + _2
                 , _1 * _1
                 , _2 / _1
                 );

   print_ins_and_outs( expr1 );

   print_ins_and_outs(   (_1,_2,_3) >> (_1,_2) >> _4 );
   print_ins_and_outs(   (_1,_2) >> (_1,_2,_2) >> _4 );

    
   //auto seq_expr = _1 - 37 >> _1*2 ;
   auto seq = compile(( (_1 + _2 - 37, 2) >> _1*2 ));
   std::cout << seq(1337,2) << std::endl;

   /*{
      auto seq = compile( _1 - 37 >> _1*2 );
      __asm__ __volatile__("nop":::"memory");   // movl  $1337, %esi
      auto x = seq(1337);                       // subl  -108(%rbp), %esi
      __asm__ __volatile__("nop":::"memory");   // imull -112(%rbp), %esi
      std::cout << x << std::endl;
   }*/
}
