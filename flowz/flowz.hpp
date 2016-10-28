//   -----------------------------------------------------------------------------------------------
//    Copyright 2015 André Bergner. Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//      --------------------------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <array>

#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>

#include <boost/algorithm/string/erase.hpp>

#include "callable_decltype.hpp"
#include "tuple_tools.hpp"
#include "demangle.h"



namespace flowz
{



namespace mpl = boost::mpl;
namespace proto = boost::proto;



namespace building_blocks
{
   using namespace proto;

   //  ---------------------------------------------------------------------------------------------
   // copy_domain -- all flowz expressions should be hold by value to avoid dangling references
   //  ---------------------------------------------------------------------------------------------


   template< typename E >
   struct copy_expr;

   struct copy_generator : proto::pod_generator< copy_expr > {};

   struct copy_domain : proto::domain< copy_generator >
   {
      template < typename T >
      struct as_child : proto_base_domain::as_expr<T> {};
   };

   template< typename Expr >
   struct copy_expr
   {
      BOOST_PROTO_EXTENDS( Expr, copy_expr<Expr>, copy_domain )
   };


   //  ---------------------------------------------------------------------------------------------
   // definition of the building blocks of the language
   //  ---------------------------------------------------------------------------------------------

   template< typename X >
   auto make_terminal( X x )
   {
      return  make_expr<tag::terminal,copy_domain>(x);
   }


   template < typename I >  struct placeholder       { using arity = I; };
   template < typename T >  struct placeholder_arity { using type = typename T::arity; };

   template< int n >
   auto make_placeholder()
   {
      return  make_expr<tag::terminal, copy_domain>( placeholder< mpl::int_<n> >{} );
   }

   template < typename idx = _ >
   using delayed_placeholder = subscript< terminal<placeholder<idx>> , terminal<placeholder<_>> >;

   //  ---------------------------------------------------------------------------------------------
   // block composition operators -- TODO needs correct grammar as arguments

   using channel_operator  = comma              < _ , _ >;
   using parallel_operator = bitwise_or         < _ , _ >;
   using sequence_operator = bitwise_or_assign  < _ , _ >;
   using feedback_operator = complement         < _ >;

   // private nodes

   struct binary_feedback_tag {};
   using binary_feedback_operator = binary_expr< binary_feedback_tag , _ , _ >;

   template < typename LeftExpr , typename RightExpr >
   auto make_binary_feedback( LeftExpr const & l , RightExpr const & r )
   {  return make_expr<building_blocks::binary_feedback_tag>( l, r );  }
}

using building_blocks :: make_terminal;

using building_blocks :: placeholder;
using building_blocks :: placeholder_arity;
using building_blocks :: make_placeholder;
using building_blocks :: delayed_placeholder;

using building_blocks :: channel_operator;
using building_blocks :: sequence_operator;
using building_blocks :: parallel_operator;
using building_blocks :: feedback_operator;
using building_blocks :: binary_feedback_operator;
using building_blocks :: make_binary_feedback;


BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );
BOOST_PROTO_DEFINE_ENV_VAR( delayed_input_t, delayed_input );



//  ------------------------------------------------------------------------------------------------
// very simple delay_line operation on std::array --> TODO should be own type static_delay_line !

struct no_state {};

struct
{
    template< typename T , size_t N , typename Y >
    void operator()( std::array<T,N>& xs , Y y ) const
    {
        for ( size_t n = 1 ; n < xs.size() ; ++n )  xs[n-1] = xs[n];
        xs.back() = y;
    }

    template< typename T , typename Y >
    void operator()( std::array<T,0>& , Y ) const { }

    template< typename T , typename Y >
    void operator()( no_state , Y ) const { }

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
      <  feedback_operator
      ,  mpl::max
         <  mpl::int_<0>
         ,  mpl::minus< input_arity(_child) , output_arity(_child) >
         >()
      >
   ,  when
      <  binary_feedback_operator
      ,  mpl::plus
         <  mpl::max
            <  mpl::int_<0>
            ,  mpl::minus< input_arity(_left) , output_arity(_right) >
            >
         ,  mpl::max
            <  mpl::int_<0>
            ,  mpl::minus< input_arity(_right) , output_arity(_left) >
            >
         >()
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
      <  feedback_operator
      ,  output_arity(_child)
      >
   ,  when
      <  binary_feedback_operator
      ,  output_arity(_right)
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


   template < typename Expr >
   using input_arity_t = typename boost::result_of<input_arity(Expr)>::type;

   template < typename Expr >
   using output_arity_t = typename boost::result_of<output_arity(Expr)>::type;


   // -------------------------------------------------------------------------------------------
   // front-panel for the expressions -- collects all loose wires
   // -------------------------------------------------------------------------------------------


   template < std::size_t N >
   inline auto make_front()
   {
      return make_front<N-1>() | make_placeholder<1>();
   }

   template <>
   inline auto make_front<1>()
   {
      return make_placeholder<1>();
   }

   template < typename Expr >
   auto add_front_panel( Expr const & x )
   {
      return  make_front<input_arity_t<Expr>::value>() |= x;
   }




   // -------------------------------------------------------------------------------------------
   // state and wire related tuple tools
   // -------------------------------------------------------------------------------------------

   template < typename arity , typename delay , typename base_other >
   struct make_arity_impl
   {
      using type = tuple_cat_t< repeat_t< arity::value-1ul, base_other >
                              , std::tuple<delay>
                              >;
   };

   template < typename delay , typename base_other >
   struct make_arity_impl<mpl::int_<0>, delay, base_other>
   {
      using type = std::tuple<>;
   };

   template < typename arity , typename delay = mpl::int_<0>, typename base_other = mpl::int_<0> >
   using make_arity = typename make_arity_impl<arity,delay,base_other>::type;


   struct delay_per_wire : callable_decltype
   {
      using default_t = placeholder<mpl::int_<0>>;

      template < typename Placeholder = default_t , typename Delay = default_t>
      auto operator()( Placeholder const & = default_t{}, Delay const & = default_t{} ) const
      {
         return make_arity<typename Placeholder::arity, typename Delay::arity>{};
      }
   };


   struct delay_per_wire_2 : callable_decltype
   {
      using default_t = placeholder<mpl::int_<0>>;

      template < typename Placeholder = default_t , typename Delay = default_t>
      auto operator()( Placeholder const & = default_t{}, Delay const & = default_t{} ) const
      {
         return make_arity<typename Placeholder::arity, typename Delay::arity, mpl::int_<-1>>{};
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




   template < int n , int m >
   using map_min = mpl::int_< n == -1 ? m : m == -1 ? n : (n<m)?n:m >;
   // -1 ,  m  ->  m
   //  n , -1  ->  n
   //  n ,  m  ->  min(n,m)

   template < typename Tuple1, typename Tuple2, std::size_t... Ns >
   auto zip_with_min( Tuple1&& t1, Tuple2&& t2, std::index_sequence<Ns...> )
   {
      using namespace std;
      return tuple< map_min< decay_t<decltype(get<Ns>(t1))>::value , decay_t<decltype(get<Ns>(t2))>::value >... >{};
   }

   template < typename Tuple1, typename Tuple2 >
   auto zip_with_min( Tuple1&&, Tuple2&&, std::index_sequence<> )
   {
      return std::tuple<>{};
   }

   struct min_delay_of_wires : callable_decltype
   {
      template < typename Tuple_l , typename Tuple_r >
      auto operator()( Tuple_l&& tl, Tuple_r&& tr ) const
      {
         using tuple_tl = std::decay_t<Tuple_l>;
         using tuple_tr = std::decay_t<Tuple_r>;
         using min_size = typename mpl::min< mpl::int_<std::tuple_size<tuple_tl>::value>
                                           , mpl::int_<std::tuple_size<tuple_tr>::value>
                                           >::type;
         return  std::tuple_cat(  zip_with_min(tl,tr,std::make_index_sequence<min_size::value>{})
                               ,  tuple_drop<min_size::value>( std::forward<Tuple_l>(tl) )
                               ,  tuple_drop<min_size::value>( std::forward<Tuple_r>(tr) )
                               );
      }
   };






   struct tuple_take_ : callable_decltype
   {
      template< typename Drop , typename Tuple >
      decltype(auto) operator()( Drop , Tuple const & t ) const
      {
         return tuple_take<Drop::value>(t);
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

   template < typename Generator , typename Combiner >
   struct input_delays
   {
      struct apply : or_
      <  when
         <  delayed_placeholder<>
         ,  Generator( _value(_left) , _value(_right) )
         >
      ,  when
         <  terminal< placeholder<_> >
         ,  Generator( _value() )
         >
      ,  when
         <  terminal<_>
         ,  std::tuple<>()
         >
      ,  when
         <  feedback_operator
         ,  tuple_drop_
            (  output_arity(_child)
            ,  apply(_child)
            )
         >
      ,  when
         <  binary_feedback_operator
         ,  tuple_cat_fn
            (  tuple_drop_
               (  output_arity(_right)
               ,  apply(_left)
               )
            ,  tuple_drop_
               (  output_arity(_left)
               ,  apply(_right)
               )
            )
         >
      ,  when
         <  parallel_operator
         ,  tuple_cat_fn( apply(_left) , apply(_right) )
         >
      ,  when
         <  sequence_operator
         ,  tuple_cat_fn
            (  apply(_left)
            ,  tuple_drop_
               (  output_arity(_left)
               ,  apply(_right)
               )
            )
         >
      ,  when
         <  nary_expr<_, vararg<_>>
         ,  fold<_, std::tuple<>(), Combiner(apply, _state) >
         >
      >
      {};
   };


   using max_input_delays = typename input_delays< delay_per_wire , max_delay_of_wires >::apply;
   using min_input_delays = typename input_delays< delay_per_wire_2 , min_delay_of_wires >::apply;

   template < typename Expr >
   using min_input_delays_t = decltype( min_input_delays{}( std::declval<Expr>() ) );



   template < bool... bs >
   struct bools {};

   template < typename Tuple , typename T >
   struct any_of_;

   template < typename... Ts , typename T >
   struct any_of_< std::tuple<Ts...> , T > : std::integral_constant
   <  bool
   ,  !std::is_same
      <  bools< std::is_same<T , Ts>::value... >
      ,  bools< !std::is_same<Ts, Ts>::value... >
      >::value
   >
   {};


   template < typename Expr >
   using needs_direct_input = any_of_< min_input_delays_t<Expr> , mpl::int_<0> >;

   template < typename Expr , std::size_t N >
   using needs_num_direct_input = any_of_< tuple_take_t<N,min_input_delays_t<Expr>> , mpl::int_<0> >;




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
         <  feedback_operator
         ,  make_tuple_fn
            (  StateCtor(max_input_delays( _child ))
            ,  apply( _child )
            )
         >
      ,  when
         <  binary_feedback_operator
         ,  make_tuple_fn
            (  StateCtor( tuple_take_( output_arity(_left) , max_input_delays(_right) ))
            ,  apply( _left  )
            ,  apply( _right )
            )
         >
      ,  when
         <  sequence_operator
         ,  make_tuple_fn
            (  StateCtor( tuple_take_( output_arity(_left) , max_input_delays(_right) ))
            ,  apply( _left  )
            ,  apply( _right )
            )
         >
      ,  when
         <  parallel_operator
         ,  make_tuple_fn
            (  apply( _left  )
            ,  apply( _right )
            )
         >
      ,  otherwise< std::tuple<>() >
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
   struct feedback;
   struct binary_feedback;
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
      <  feedback_operator
      ,  feedback( _child , _env_var<current_input_t> , _env_var<delayed_input_t> )
      >
   ,  when
      <  binary_feedback_operator
      ,  binary_feedback( _left , _right, _env_var<current_input_t> , _env_var<delayed_input_t> )
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

   //  ---------------------------------------------------------------------------------------------
   // expression rebuilder -- transforms unary feedback nodes into binary ones
   //  ---------------------------------------------------------------------------------------------

   // example: a*_1 |= _1 + _2 |= _1[_1]   -->  future_expr: a*_1 |= _1 + _2

   //  ~( _1 |= ~(a*_1[_1]+_2[_1]) )

   struct split_future_subexpr;

   struct make_canonical : or_
   <  when
      <  terminal<_>
      ,  _
      >
   ,  when
      <  feedback_operator
      ,  split_future_subexpr( _child )
      >
   ,  otherwise< _default<make_canonical> >
   >
   {};


   struct split_in_sequence_combinator;
   struct lifted_default;

   struct unary_to_binary_feedback : or_
   <  when
      <  terminal<_>
      ,  make_tuple_fn(_)
      >
   ,  when
      <  sequence_operator
      ,  split_in_sequence_combinator( _left , _right , _state )
      >
   ,  when
      <  _
      ,  lifted_default( _ , _state )
      >
   >
   {};

   struct lifted_default : callable_decltype
   {
      template< typename Expr , typename State , std::size_t... Ns >
      auto apply( std::index_sequence<Ns...> , Expr const & x , State const & s ) const
      {
         using  tag = typename tag_of<Expr>::type;
         unary_to_binary_feedback   e;
         auto res = std::make_tuple( e(child_c<Ns>(x),s)... );
         return std::tuple_cat( std::make_tuple( make_expr<tag>( std::get<0>(std::get<Ns>(res))... ) )
                              , tail(std::get<0>(res))
                              );
      }

      template< typename Expr, typename State >
      auto operator()( Expr const & e , State const & s ) const
      {
         using idx_seq = std::make_index_sequence< proto::arity_of<Expr>::value >;
         return apply( idx_seq{}, e , s );
      }
   };



   // TODO
   // • in compile: transform expression into new expression w/ binary feedback node
   // • handle expr w/o future part, e.g. _1[_1] |= _1[_1] -->  (expr,())
   // • needs_direct_input must be parametrized prev. output arity
   // • handle parallel combiner, e.g. (_1 |= _1[_1]) | (_1 |= _1[_1]) should work (diff. split point)
   // • handle expr w/o current part, e.g. _1 |= _1 -->  ((),expr) compile error
   // • nested recursion:
   //   - idea: trafo1 -> go down and rebuild expr
   //                   , when in first (i.e. deepest) una_feedback call rebuild-splitter
   //                   , return this expr
   //                   , what to do with bin_feeback?

   struct split_future_subexpr : callable_decltype
   {
      template < typename Expr >
      auto operator()( in_t<Expr> x ) const
      {
         make_canonical  t;
         unary_to_binary_feedback  e;
         auto split_pred = [](auto const & l, auto const & r)
         {
            using l_expr_t = decltype(l);
            using r_expr_t = decltype(r);
            return needs_num_direct_input<r_expr_t,output_arity_t<l_expr_t>::value>{};
         };
         //return impl( e(t(add_front_panel(x)), split_pred) );
         return impl( e(t(make_front<output_arity_t<Expr>::value>() |= x), split_pred) );
      }

      template < typename LiftedExpr >
      auto impl( in_t<LiftedExpr> x ) const
      {
         return make_binary_feedback( std::get<0>(x), std::get<1>(x) );
      }
   };


   struct split_in_sequence_combinator : callable_decltype
   {
      template < typename LeftExpr , typename RightExpr , typename State >
      auto operator()( in_t<LeftExpr> l , in_t<RightExpr> r , State const & s ) const
      {
         unary_to_binary_feedback  e;
         return impl( e(l,s), r, s );
      }

   private:

      template < typename LL , typename LR , typename R , typename Pred >
      auto impl( in_t<std::tuple<LL,LR>> l , in_t<R> r , in_t<Pred> p ) const
      {
         return std::make_tuple( std::get<0>(l), std::get<1>(l) |= r );
      }

      template < typename L , typename R , typename Pred >
      auto impl( in_t<std::tuple<L>> l , in_t<R> r , in_t<Pred> p ) const
      {
         return impl2( p(std::get<0>(l),r), l, r, p );
      }

      template < typename L , typename R , typename P >
      auto impl2( std::true_type , in_t<L> l , in_t<R> r , in_t<P> p ) const
      {
         unary_to_binary_feedback  e;
         return impl3( l , e(r,p) );
      }

      template < typename LeftExpr , typename RL , typename RR >
      auto impl3( in_t<LeftExpr> l , in_t<std::tuple<RL,RR>> r ) const
      {
         return std::make_tuple( std::get<0>(l) |= std::get<0>(r) , std::get<1>(r) );
      }

      template < typename LeftExpr , typename R >
      auto impl3( in_t<LeftExpr> l , in_t<std::tuple<R>> r ) const
      {
         return std::make_tuple( std::get<0>(l) |= std::get<0>(r) );
      }

      template < typename L , typename RightExpr , typename P >
      auto impl2( std::false_type , in_t<L> l , in_t<RightExpr> r , in_t<P> ) const
      {
         return std::make_tuple( std::get<0>(l) , r );
      }

   };


   //-----------------------------------------------------------------------------------------------


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
         auto const & s = std::get<I::value-1>(std::get<0>(del_in));
         return s[s.size()-J::value];
      }
   };

   struct sequence : callable_decltype
   {
      template < typename LeftExpr , typename RightExpr , typename Input , typename State >
      auto operator()( in_t<LeftExpr> l , in_t<RightExpr> r , in_t<Input> input , State state ) const
      {
         eval_it  e;

         auto in_state    = deep_tie(std::get<0>(state));
         auto node_state  = deep_tie(std::get<0>(std::get<1>(state)));
         auto left_state  = deep_tie(std::get<1>(std::get<1>(state)));
         auto right_state = deep_tie(std::get<2>(std::get<1>(state)));
/*
         std::cout << "--- state triple ---" << std::endl;
         print_state(std::get<1>(state));
         std::cout << "--- node_state ---" << std::endl;
         print_state(node_state);
         std::cout << "--- left_state ---" << std::endl;
         print_state(left_state);
         std::cout << "--- right_state ---" << std::endl;
         print_state(right_state);
*/
         //static_assert( std::tuple_size<Input>::value == std::tuple_size<decltype(in_state)>::value );

         auto left_delayed_input = tuple_take<input_arity_t<LeftExpr>::value>(in_state);

         auto left_result =
            flatten_tuple( std::make_tuple(
               e( l, 0, ( current_input = tuple_take<input_arity_t<LeftExpr>::value>(input)
                        , delayed_input = std::tie( left_delayed_input, left_state )
                        ))
            ));

         using left_size = std::tuple_size<decltype(left_result)>;
         auto right_input = tuple_cat( left_result, tuple_drop<input_arity_t<LeftExpr>::value>(input) );
         auto right_delayed_input = std::tuple_cat( node_state , tuple_drop<input_arity_t<LeftExpr>::value>(in_state) );

         auto right_result =
            flatten_tuple( std::make_tuple(
               e( r, 0, ( current_input = right_input
                        , delayed_input = std::tie( right_delayed_input, right_state )
                        ))
            ));

         tuple_for_each( rotate_push_back, node_state, left_result );

         return tuple_cat( right_result
                         , tuple_drop< input_arity_t<RightExpr>::value >( std::move(left_result) )
                         , tuple_drop< input_arity_t<LeftExpr>::value + left_size::value >( input )
                         );
      }
   };


   struct bottom_type {};

   struct feedback : callable_decltype
   {
      template < typename Expr , typename Input , typename State >
      auto operator()( in_t<Expr> x , in_t<Input> input , State state ) const
      {
         eval_it  e;

         auto in_state    = deep_tie(std::get<0>(state));
         auto node_state  = deep_tie(std::get<0>(std::get<1>(state)));
         auto child_state = deep_tie(std::get<1>(std::get<1>(state)));
         auto next_state  = std::tuple_cat( node_state, in_state );

         auto aligned_input = std::tuple_cat( repeat_t<output_arity_t<Expr>::value, bottom_type>{} , input );
         auto result =
            flatten_tuple( std::make_tuple(
               e( x, 0, ( current_input = aligned_input , delayed_input = std::tie(next_state,child_state) ))
            ));


         tuple_for_each( rotate_push_back, node_state, std::tuple_cat( result, input) );
         using size = std::tuple_size<decltype(result)>;
         return tuple_cat( result, tuple_drop<size::value>(input) );
      }
   };

   struct binary_feedback : callable_decltype
   {
      template < typename LeftExpr , typename RightExpr , typename Input , typename State >
      auto operator()( in_t<LeftExpr> l , in_t<RightExpr> r , in_t<Input> input , State state ) const
      {
         eval_it  e;

         auto in_state    = deep_tie(std::get<0>(state));
         auto node_state  = deep_tie(std::get<0>(std::get<1>(state)));
         auto left_state  = deep_tie(std::get<1>(std::get<1>(state)));
         auto right_state = deep_tie(std::get<2>(std::get<1>(state)));
/*
         std::cout << "--- bf state triple ---" << std::endl;
         print_state(std::get<1>(state));
         std::cout << "--- bf node_state ---" << std::endl;
         print_state(node_state);
         std::cout << "--- bf left_state ---" << std::endl;
         print_state(left_state);
         std::cout << "--- bf right_state ---" << std::endl;
         print_state(right_state);
*/
         auto future_input = std::tuple_cat
                           ( repeat_t<output_arity_t<LeftExpr>::value, bottom_type>{}
                           //, tuple_drop< input_arity_t<LeftExpr>::value-output_arity_t<RightExpr>::value >(input) );
                           , tuple_drop< std::min(0,input_arity_t<LeftExpr>::value-output_arity_t<RightExpr>::value) >(input) );
                           // TODO min( 0 , drop_value )
         auto left_delayed_input = std::tuple_cat
               //( node_state , tuple_drop< input_arity_t<LeftExpr>::value-output_arity_t<RightExpr>::value >(in_state) );
               ( node_state , tuple_drop< std::min(0,input_arity_t<LeftExpr>::value-output_arity_t<RightExpr>::value) >(in_state) );

         auto result =
            flatten_tuple( std::make_tuple(
               e( r, 0, ( current_input = future_input , delayed_input = std::tie(left_delayed_input,right_state) ))
            ));

         auto promise_input = std::tuple_cat( result, tuple_take< input_arity_t<LeftExpr>::value-output_arity_t<RightExpr>::value >(input) );
         auto right_delayed_input = std::tuple_cat
               ( repeat_t<output_arity_t<LeftExpr>::value, no_state>{}
               , tuple_take< input_arity_t<LeftExpr>::value-output_arity_t<RightExpr>::value >(in_state) );

         auto promise_result =
            flatten_tuple( std::make_tuple(
               e( l, 0, ( current_input = promise_input , delayed_input = std::tie( right_delayed_input ,left_state) ))
            ));

         tuple_for_each( rotate_push_back, node_state, promise_result );

         return tuple_cat( result
                         //, tuple_drop< input_arity_t<RightExpr>::value >( std::move(left_result) )
                         //, tuple_drop< input_arity_t<LeftExpr>::value + left_size::value >( input )
                         );
      }
   };

   struct parallel : callable_decltype
   {
      template < typename LeftExpr , typename RightExpr , typename Input , typename State >
      auto operator()( in_t<LeftExpr> l , in_t<RightExpr> r , in_t<Input> input , State state ) const
      {
         eval_it  e;

         auto in_state    = deep_tie(std::get<0>(state));
         auto left_state  = deep_tie(std::get<0>(std::get<1>(state)));
         auto right_state = deep_tie(std::get<1>(std::get<1>(state)));

         auto in_state_l = tuple_take<input_arity_t<LeftExpr>::value>(in_state);
         auto left_state_  = ( current_input = tuple_take<input_arity_t<LeftExpr>::value>(input)
                             , delayed_input = std::tie(in_state_l,left_state)
                             );

         auto in_state_r = tuple_drop<input_arity_t<LeftExpr>::value>(in_state);
         auto right_state_ = ( current_input = tuple_drop<input_arity_t<LeftExpr>::value>(input)
                             , delayed_input = std::tie(in_state_r,right_state)
                             );
         return std::make_tuple
                (  e( l, 0, left_state_ )
                ,  e( r, 0, right_state_ )
                );
      }
   };

}


using  transforms :: input_arity;
using  transforms :: max_input_delays;
using  transforms :: build_state;
using  transforms :: input_arity_t;
using  transforms :: output_arity;
using  transforms :: eval_it;
using  transforms :: add_front_panel;


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

template< typename T , typename Int >
struct to_array_impl
{
   using type = std::array< T , std::decay_t<Int>::value >;
};

template< typename T >
struct to_array_impl< T, mpl::int_<0> >
{
   using type = no_state;     // array<T,N> prevents empty base class optimization to kick in.
};


template< typename T >
struct to_array
{
   template< typename Int >
   using apply_t = typename to_array_impl<T,std::decay_t<Int>>::type;
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
//  • takes an expression
//  • returns closure holding the expression and an associated context with state & ceofficients
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
      std::tuple<> in_state;
      auto in_tuple = std::tie(in_state,state_);
      auto result = eval_it{}( expr_, 0, ( current_input = std::make_tuple(args...)
                                         , delayed_input = in_tuple ) );
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

   stateful_lambda( Expression expr ) : expr_( expr )
   {
      //build_state<> b;
      //std::cout << "• size expr:  " << sizeof(expr_)  << std::endl;
      //std::cout << "• size state: " << sizeof(state_) << std::endl;
      //std::cout << "• state: ";     print_state(state_);
      ////print_state(b(expr));
   }

   template < typename... Args , typename = std::enable_if_t< sizeof...(Args) <= arity > >
   auto operator()( Args const &... args ) -> decltype(auto)
   {
      return call_impl( mpl::int_< arity - sizeof...(Args) >{} , args... );
   }
};


auto compile = []( auto expr_ )
{
   // static_assert( is_valid_expr<decltype(expr_)>::value , "This expression is not valid." );
   // TODO must not contain private expressions, yet.

   using arity_t = input_arity_t<decltype(expr_)>;

   transforms :: make_canonical  canonize;

   //auto expr = expr_;
   //auto expr = add_front_panel(expr_);
   //auto expr = canonize( expr_ );
   auto expr = canonize( add_front_panel(expr_) );
   using expr_t = decltype(expr);

   auto builder = build_state< to_array_tuple<float>::apply >{};
   using state_t = decltype( builder(expr) );
   //proto::display_expr(expr);
   //print_state(state_t{});

   return stateful_lambda< expr_t, state_t, arity_t::value>{ expr };
};


const auto _1 = make_placeholder<1>();
const auto _2 = make_placeholder<2>();
const auto _3 = make_placeholder<3>();
const auto _4 = make_placeholder<4>();
const auto _5 = make_placeholder<5>();
const auto _6 = make_placeholder<6>();


}  // flowz

