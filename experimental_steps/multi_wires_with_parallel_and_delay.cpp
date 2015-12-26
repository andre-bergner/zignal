//   -----------------------------------------------------------------------------------------------
//    Copyright 2015 André Bergner. Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//      --------------------------------------------------------------------------------------------

#include <iostream>
#include <array>

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

   template < typename I >  struct placeholder       { using arity = I; };
   template < typename T >  struct placeholder_arity { using type = typename T::arity; };

   template < typename idx = _ >
   using delayed_placeholder = subscript< terminal<placeholder<idx>> , terminal<placeholder<_>> >;

   //  ---------------------------------------------------------------------------------------------
   // block composition operators -- TODO needs correct grammar as arguments

   using channel_operator  = comma              < _ , _ >;
   using parallel_operator = bitwise_or         < _ , _ >;
   using sequence_operator = bitwise_or_assign  < _ , _ >;
}

using building_blocks :: placeholder;
using building_blocks :: placeholder_arity;
using building_blocks :: delayed_placeholder;

using building_blocks :: channel_operator;
using building_blocks :: sequence_operator;
using building_blocks :: parallel_operator;


BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );
BOOST_PROTO_DEFINE_ENV_VAR( delayed_input_t, delayed_input );



//  ------------------------------------------------------------------------------------------------
// very simple delay_line operation on std::array --> TODO should be own type static_delay_line !

struct
{
    template< typename T , size_t N , typename Y >
    void operator()( std::array<T,N>& xs , Y y ) const
    {
        for ( size_t n = 1 ; n < xs.size() ; ++n )  xs[n-1] = xs[n];
        xs.back() = y;
    }

    template< typename T , typename Y >
    void operator()( std::array<T,0>& xs , Y y ) const { }

    template< typename T , typename Y >   // if input is not an array, e.g. for not fully
    void operator()( T& , Y ) const { }   // unpacked state while traversing the tree (tuple o'tuples)
}
rotate_push_back;



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
      <  parallel_operator
      ,  mpl::plus< input_arity(_left) , input_arity(_right) >()
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
      <  parallel_operator
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
   ,  otherwise< mpl::int_<1>() >
   >
   {};



   template < typename arity , typename delay = mpl::int_<0> >
   struct make_arity_impl
   {
      using type = tuple_cat_t< repeat_t< arity::value-1ul, mpl::int_<0> >
                              , std::tuple<delay>
                              >;
   };

   template < typename delay >
   struct make_arity_impl<mpl::int_<0>, delay>
   {
      using type = std::tuple<>;
   };

   template < typename arity , typename delay = mpl::int_<0> >
   using make_arity = typename make_arity_impl<arity,delay>::type;


   struct delay_per_wire : callable_decltype
   {
      using default_t = placeholder<mpl::int_<0>>;

      template < typename Placeholder = default_t , typename Delay = default_t>
      auto operator()( Placeholder const & = default_t{}, Delay const & = default_t{} ) const
      {
         return make_arity<typename Placeholder::arity, typename Delay::arity>{};
      }
   };

   struct tuple_cat_fn : callable_decltype
   {
      template < typename... Tuples >
      auto operator()( Tuples&&... ts ) const
      {
         return std::tuple_cat( std::forward<Tuples>(ts)... );
      }
   };

   struct make_tuple_fn : callable_decltype
   {
      template < typename... Ts >
      auto operator()( Ts&&... ts ) const
      {
         return std::make_tuple( std::forward<Ts>(ts)... );
      }
   };

   // -------------------------------------------------------------------------------------------
   // max-zip tuple
   // -------------------------------------------------------------------------------------------

   template < typename Tuple1, typename Tuple2, std::size_t... Ns >
   auto zip_with_max( Tuple1&& t1, Tuple2&& t2, std::index_sequence<Ns...> )
   {
      using namespace std;
      return tuple< typename mpl::max< decay_t<decltype(get<Ns>(t1))> , decay_t<decltype(get<Ns>(t2))> >::type... >{};
   }

   template < typename Tuple1, typename Tuple2 >
   auto zip_with_max( Tuple1&&, Tuple2&&, std::index_sequence<> )
   {
      return std::tuple<>{};
   }

   struct max_delay_of_wires : callable_decltype
   {
      template < typename Tuple_l , typename Tuple_r >
      auto operator()( Tuple_l&& tl, Tuple_r&& tr ) const
      {
         using tuple_tl = std::decay_t<Tuple_l>;
         using tuple_tr = std::decay_t<Tuple_r>;
         using min_size = typename mpl::min< mpl::int_<std::tuple_size<tuple_tl>::value>
                                           , mpl::int_<std::tuple_size<tuple_tr>::value>
                                           >::type;
         return  std::tuple_cat(  zip_with_max(tl,tr,std::make_index_sequence<min_size::value>{})
                               ,  tuple_drop<min_size::value>( std::forward<Tuple_l>(tl) )
                               ,  tuple_drop<min_size::value>( std::forward<Tuple_r>(tr) )
                               );
      }
   };

   struct tuple_drop_ : callable_decltype
   {
      template< typename Drop , typename Tuple >
      decltype(auto) operator()( Drop , Tuple const & t ) const
      {
         return tuple_drop<Drop::value>(t);
      }
   };

   struct input_delays : or_
   <  when
      <  delayed_placeholder<>
      ,  delay_per_wire( _value(_left),_value(_right))
      >
   ,  when
      <  terminal< placeholder<_> >
      ,  delay_per_wire( _value() )
      >
   ,  when
      <  terminal<_>
      ,  std::tuple<>()//delay_per_wire()
      >
   ,  when
      <  parallel_operator
      ,  tuple_cat_fn( input_delays(_left) , input_delays(_right) )
      >
   ,  when
      <  sequence_operator
      ,  tuple_cat_fn
         (  input_delays(_left)
         ,  tuple_drop_ // <-- problem here????
            (  output_arity(_left)
            ,  input_delays(_right)
            )
         )
      >
   ,  when
      <  nary_expr<_, vararg<_>>
      ,  fold<_, std::tuple<>(), max_delay_of_wires(input_delays, _state) >
      >
   >
   {};



   template < typename Expr >
   using input_arity_t = typename boost::result_of<input_arity(Expr)>::type;


   struct identity : callable_decltype
   {
      template < typename X >
      auto operator()( X&& x ) { return std::forward<X>(x); }
   };

   template < typename StateCtor >
   struct build_state_impl
   {
      struct apply : or_
      <  when
         <  sequence_operator
         ,  make_tuple_fn
            (  apply( _left  , StateCtor(input_delays( _left  )) )
            ,  apply( _right , StateCtor(input_delays( _right )) )
            )
         >
      ,  otherwise< _state >
      >
      {};
   };

   template < typename StateCtor = identity >
   using build_state = typename build_state_impl<StateCtor>::apply;


   //  ---------------------------------------------------------------------------------------------
   // Evaluators -- transforms that work together for the evaluation of flowz-expressions
   //  ---------------------------------------------------------------------------------------------

   struct place_the_holder;
   struct place_delay;
   struct sequence;
   struct parallel;


   struct eval_it : or_
   <  when
      <  delayed_placeholder<>
      ,  place_delay( _value(_left) , _value(_right) , _env_var<delayed_input_t> )
      >
   ,  when
      <  terminal< placeholder<_> >
      ,  place_the_holder( _value , _env_var<current_input_t> )
      >
   ,  when
      <  sequence_operator
      ,  sequence( _left , _right , _env_var<current_input_t> , _env_var<delayed_input_t> )
      >
   ,  when
      <  parallel_operator
      ,  parallel( _left , _right , _env_var<current_input_t> , _env_var<delayed_input_t> )
      >
   ,  when
      <  channel_operator    //  grammar rule: only allowed after sequence or recursion at top of sub-tree
      ,  make_tuple_fn( eval_it(_left), eval_it(_right) )
      >
   ,  when
      <  _
      ,  _default< eval_it >
      >
   >
   {};


   //-----------------------------------------------------------------------------------------------


   template < typename T >
   using in_t = T const &;
   //using in_t = T;

   struct place_the_holder : callable_decltype
   {
      template < typename I , typename Tuple >
      auto operator()( placeholder<I> const & , Tuple const & args ) const
      {
         return std::get<I::value-1>( args );
      }
   };

   struct place_delay : callable_decltype
   {
      template < typename I , typename J , typename Delayed_input >
      auto operator()( placeholder<I> , placeholder<J> , in_t<Delayed_input> del_in ) const
      {
         auto const & s = std::get<I::value-1>(del_in);
         return s[s.size()-J::value];
      }
   };

   struct sequence : callable_decltype
   {
      template < typename LeftExpr , typename RightExpr , typename Input , typename Delay >
      auto operator()( in_t<LeftExpr> l , in_t<RightExpr> r , in_t<Input> input , Delay& delay ) const
      {
         auto e = eval_it{};

         auto& ls = std::get<0>(delay);
         auto& rs = std::get<1>(delay);

         auto left_result =
            flatten_tuple( std::make_tuple(
               e( l, 0, ( current_input = input , delayed_input = boost::ref(ls) ))
            ));
         tuple_for_each( rotate_push_back, ls, input );

         auto res = e( r, 0, ( current_input = left_result , delayed_input = boost::ref(rs) ) );
         tuple_for_each( rotate_push_back, rs, left_result );

         std::cout << "rs: " << rs << "     ";

         return res;
      }
   };

   struct parallel : callable_decltype
   {
      template < typename LeftExpr , typename RightExpr , typename Input , typename Delay >
      auto operator()( in_t<LeftExpr> l , in_t<RightExpr> r , in_t<Input> input , Delay& del ) const
      {
         auto e = eval_it{};
         auto left_state  = ( current_input = input , delayed_input = del );
         auto right_state = ( current_input = tuple_drop<input_arity_t<LeftExpr>::value>(input)
                            , delayed_input = tuple_drop<input_arity_t<LeftExpr>::value>(del)
                            );
         return std::make_tuple
                (  e( l, 0, left_state )
                ,  e( r, 0, right_state )
                );
      }
   };

}


using  transforms :: input_arity;
using  transforms :: input_delays;
using  transforms :: build_state;
using  transforms :: input_arity_t;
using  transforms :: output_arity;
using  transforms :: eval_it;


//  ------------------------------------------------------------------------------------------------
// supporting state tools
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
      using type = std::array< T , std::decay_t<Int>::value >;
   };

   template< typename Int >
   using apply_t = typename apply<Int>::type;

};

template < typename T >
struct to_array_tuple
{
   struct apply : proto::callable_decltype
   {
       template< typename Tuple >
       auto operator()( Tuple ) { return lift_into_tuple_t< to_array<T>, Tuple > {}; }
   };
};



//  ------------------------------------------------------------------------------------------------
// compile  --  main function of framework
//              • takes an expression
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

   template < typename... Args >
   auto call_impl( mpl::int_<0> , Args const &... args ) -> decltype(auto)
   {
      auto ref_state = std::tie( std::get<0>(state_) , std::get<1>(state_) );
      auto result = eval_it{}( expr_, 0, ( current_input = std::make_tuple(args...) , delayed_input = boost::ref(ref_state) ) );

      //tuple_for_each( rotate_push_back, state_, std::make_tuple(args...) );
      return flatten_tuple( std::make_tuple( result ));
   }

   template < int arg_diff , typename... Args >
   auto call_impl( mpl::int_<arg_diff> , Args const &... args )
   {
      return [ args...
             , expr = *this ]           // TODO should not be a lambda, but a type that is
      ( auto const &... missing_args ) mutable  //      aware of the # of args (enable_if them)
      {
         return expr( args..., missing_args... );
      };
   }

public:

   stateful_lambda( Expression expr ) : expr_( std::move(expr)) {}

   template < typename... Args , typename = std::enable_if_t< sizeof...(Args) <= arity > >
   auto operator()( Args const &... args ) -> decltype(auto)
   {
      return call_impl( mpl::int_< arity - sizeof...(Args) >{} , args... );
   }
};


// TODO template input type determinse state type (float)
auto compile = []( auto expr )        // TODO need to define value_type for state
{
   using expr_t = decltype(expr);
   using arity_t = input_arity_t<expr_t>;

   auto builder = build_state< to_array_tuple<float>::apply >{};
   using state_t = decltype( builder(expr) );

   return stateful_lambda< expr_t, state_t, arity_t::value>{ expr };
};







const proto::terminal< placeholder< mpl::int_<1> >>::type   _1  = {{}};
const proto::terminal< placeholder< mpl::int_<2> >>::type   _2  = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3  = {{}};
const proto::terminal< placeholder< mpl::int_<4> >>::type   _4  = {{}};
const proto::terminal< placeholder< mpl::int_<5> >>::type   _5  = {{}};
const proto::terminal< placeholder< mpl::int_<6> >>::type   _6  = {{}};



int main()
{
   /*
   std::cout << "-------------------------" << std::endl;
   print( input_delays{}(_5[_2]) );
   print( input_delays{}(_5+1) );
   print( input_delays{}(_5 | _2[_4]) );
   print( input_delays{}((_5,_1,_1,_1) |= _6[_2]) );
   print( input_delays{}((_5-_1[_3]) |= _2[_2]) );
   std::cout << "-------------------------" << std::endl;
   */

   auto print_ins_and_outs = []( auto const & expr )
   {
       std::cout << "-------------------------" << std::endl;
       //proto::display_expr( expr );
       std::cout << "#ins:  " << input_arity{} (expr) << std::endl;
       std::cout << "#outs: " << output_arity{}(expr) << std::endl;
       std::cout << std::endl;
   };

   auto e1 = ( _2 + _1 , _2 );
   auto e2 = ( _1*_2 - 100 );
   auto seq_expr =(      e1 | (_1+_2)
                      |= e2 | (_1*_1)
                  );

   print( input_delays{}(seq_expr));

   print_ins_and_outs( seq_expr );
   auto seq = compile( seq_expr );

   std::cout << std::get<1>(seq(1337,3,4)(3)) << std::endl;


   std::cout << "-------------------------" << std::endl;
   build_state<> b;
   build_state< to_array_tuple<float>::apply > d;
   std::cout << b( (_2[_1] |= _1[_2]) |= (_1[_3] |= _3[_4]) ) << std::endl;
   std::cout << b(  _2[_1] |= (_1[_2] |=  _1[_3] |= _3[_4]) ) << std::endl;
   std::cout << b( (_2[_1] |=  _1[_2] |=  _1[_3]) |= _3[_4] ) << std::endl;
   std::cout << b( _1, std::tuple<>{} ) << std::endl;
   std::cout << type_name( d( (_2[_1] |= _1[_2]) |= (_1[_3] |= _3[_4]) )) << std::endl;
   std::cout << "-------------------------" << std::endl;

   //auto z = compile( (_1[_1] , _1) |= (_2*_2-_1) );
   //auto z = compile( (_1 , _1) |= (_2-_1[_1]) );
   auto z = compile( ((_1|_1) |= (_1[_2] - _2[_3])) |= _1[_1] );

   std::cout << std::get<0>(z(1,1)) << std::endl;
   std::cout << std::get<0>(z(1,2)) << std::endl;
   std::cout << std::get<0>(z(1,3)) << std::endl;
   std::cout << std::get<0>(z(1,4)) << std::endl;
   std::cout << std::get<0>(z(1,5)) << std::endl;
   std::cout << std::get<0>(z(1,6)) << std::endl;
   std::cout << std::get<0>(z(3,10)) << std::endl;

}
