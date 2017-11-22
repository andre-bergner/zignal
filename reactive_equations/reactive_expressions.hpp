// TODO
// • check for cycles
// • remove state from parameter → move into own state-tuple
// • dependent values must be read-only!
// • rename get_value to something like get_state or get_memory ...
// • eval_assign_expr should check that rhs exists, error otherwise
// • set/get-wrapper to allow parameter wrap existing object's properties

// • win version: http://rextester.com/SYPKT89979



// ------------------------------------------------------------------------------------------------
// PARAMETER DAG -- build compile time description of dependencies using boost.proto 
// ------------------------------------------------------------------------------------------------

#define BOOST_NO_AUTO_PTR

#include "meta.h"
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/fold.hpp>
#include <iso646.h>


namespace building_blocks
{
   using namespace boost::proto;

   //  ---------------------------------------------------------------------------------------------
   // copy_domain -- all expressions should be hold by value to avoid dangling references
   //  ---------------------------------------------------------------------------------------------

   template< typename E >
   struct copy_expr;

   struct copy_generator : pod_generator< copy_expr > {};

   struct grammar;

   using allowed_unary_ops = or_
   <  unary_plus<grammar>
   ,  negate<grammar>
   ,  dereference<grammar>
   ,  complement<grammar>
   ,  address_of<grammar>
   ,  logical_not<grammar>
   >;

   using allowed_binary_ops = or_
   <  shift_left<_,_>
   ,  shift_right<_,_>
   ,  multiplies<_,_>
   ,  divides<_,_>
   ,  modulus<_,_>
   ,  plus<_,_>
   ,  minus<_,_>
   >;

   using allowed_comparison_ops = or_
   <  less<_,_>
   ,  greater<_,_>
   ,  less_equal<_,_>
   ,  greater_equal<_,_>
   ,  equal_to<_,_>
   ,  not_equal_to<_,_>
   >;

   using allowed_logical_ops = or_
   <  logical_or<_,_>
   ,  logical_and<_,_>
   ,  bitwise_and<_,_>
   ,  bitwise_or<_,_>
   ,  bitwise_xor<_,_>
   >;

   /* not supported
      mem_ptr<_,_>
      comma<_,_>
      shift_left_assign<_,_>
      shift_right_assign<_,_>
      multiplies_assign<_,_>
      divides_assign<_,_>
      modulus_assign<_,_>
      plus_assign<_,_>
      minus_assign<_,_>
      bitwise_and_assign<_,_>
      bitwise_or_assign<_,_>
      bitwise_xor_assign<_,_>
   */

   struct grammar : or_
   <  allowed_unary_ops
   ,  allowed_binary_ops
   ,  allowed_comparison_ops
   ,  allowed_logical_ops
   ,  assign<_,_>
   ,  subscript<_,_>
   ,  terminal<_>
   >
   {};

   struct copy_domain : domain< copy_generator, grammar >
   {
      template < typename T >
      struct as_child : proto_base_domain::as_expr<T> {};
   };

   template< typename Expr >
   struct copy_expr
   {
      BOOST_PROTO_EXTENDS( Expr, copy_expr<Expr>, copy_domain )
   };

   template< typename X >
   auto make_terminal( X x )
   {
      return  make_expr<tag::terminal,copy_domain>(x);
   }

   //  ---------------------------------------------------------------------------------------------
   // definition of the building blocks of the language
   //  ---------------------------------------------------------------------------------------------

   template <typename Tag, typename Value = float>
   struct parameter
   {
      Value value = {};

      // parameter(Value v) : value{v} { std::cout << "ctor" << std::endl; }
      // parameter() { std::cout << "ctor" << std::endl; }
      // parameter(parameter const& t) : value(t.value) { std::cout << "copy" << std::endl; }
      // parameter(parameter&&) { std::cout << "move" << std::endl; }
      // template <typename Any>
      // operator Value() const { return value; }
   };
   

   struct callable_decltype : callable
   {
      template< typename Signature >
      struct result;

      template< typename This , typename... Args >
      struct result< This( Args... ) >
      {
         using type = std::result_of_t<This(Args...)>;
      };
   };


   using just_parameter   = terminal<parameter<_,_>>;
   using assign_parameter = assign< terminal<parameter<_,_>> , _ >;


   // ---------------------------------------------------------------------------------------------

   struct make_set : callable_decltype
   {
      template <typename Value>
      auto operator()( Value&& ) const -> meta::type_set<std::decay_t<Value>>
      {
         return {};
      }
   };

   struct merge_set : callable_decltype
   {
      template <typename Set1, typename Set2>
      auto operator()( Set1, Set2 ) const
      {
         using namespace meta;
         return fold_t<insert_t, Set1, Set2>{};
      }

   };


   // ---------------------------------------------------------------------------------------------
   // input: an expression
   // output: meta-set of all parameter-types that are used in the expression

   struct collect_dependendencies : or_
   <  when< just_parameter, make_set(_value) >
   ,  when< terminal<_>, meta::type_set<>() >
   ,  when
      <  nary_expr<_, vararg<_>>
      ,  fold<_, meta::type_set<>(), merge_set(collect_dependendencies, _state) >
      >
   ,  when< _ , _default<collect_dependendencies> >
   >
   {};

   template <typename Expr>
   using collect_dependendencies_t
      = std::result_of_t<collect_dependendencies(Expr)>;


   // ---------------------------------------------------------------------------------------------
   // Given a depedent type (Dependee), a set of values that influence that one (Dependers),
   // and a AdjacencyMap (that maps ) insert_dependee will update the AdjacencyMap for all
   // influencing values with the given Dependee.
   // pseudo code:  for (D : Dependers) AdjacencyMap[D].insert( Dependee )
   // E.g.
   //     y = 3*x + z  // Dependee: y, Dependers: {x,z}
   //     updates: adj[x].insert(y)
   //              adj[z].insert(y)
   // insert_dependee returns a new AdjacencyMap

   template <typename Dependee, typename Dependers, typename AdjacencyMap = meta::type_map<> >
   struct insert_dependee
   {
      template <typename Map, typename Key>
      using inserter_dependee_t = meta::insert_t< meta::type_at_t<Map,Key,meta::type_set<>>, Dependee >;

      template <typename Map, typename Key>
      using inserter_t = meta::force_insert_t< Map, Key, inserter_dependee_t<Map,Key> >;

      using type = meta::fold_t< inserter_t, AdjacencyMap, Dependers >;
   };

   template <typename Dependee, typename Dependers, typename AdjacencyMap = meta::type_map<> >
   using insert_dependee_t = typename insert_dependee<Dependee, Dependers, AdjacencyMap>::type;




   // ---------------------------------------------------------------------------------------------
   // expression  ( prop[n] = f(prop[..]), adj_map ) -> adj_map
   //

   struct build_dependency_list : callable_decltype
   {
      template <typename Lhs, typename Rhs>
      auto operator()(Lhs&&, Rhs&&) const
      {
         using dependee_t = std::decay_t<Lhs>;
         using dependencies_t = std::decay_t<Rhs>;
         return meta::type_pair< dependee_t, collect_dependendencies_t<dependencies_t> >{};
      }

      template <typename Arg>
      auto operator()(Arg&&) const
      {
         using dependee_t = std::decay_t<Arg>;
         return meta::type_pair< dependee_t, meta::type_set<> >{};
      }
   };

   struct split_involved_parameters : or_
   <  when< assign_parameter, build_dependency_list(_value(_left), _right) >
   ,  when< just_parameter, build_dependency_list(_value) >
   >
   {};

   template <typename Expression>
   using split_involved_parameters_t = std::result_of_t<split_involved_parameters(Expression)>;





   template <typename AdjacencyMap, typename InvolvedParameters>
   struct insert_dependendencies
   {
      // using adjacency_map_t = meta::fold_t
      // <  insert_dependendencies, AdjacencyMap, InvolvedParameters > >;

      using type = insert_dependee_t
      <  typename InvolvedParameters::first_t
      ,  typename InvolvedParameters::second_t
      ,  AdjacencyMap
      >;
   };

   template <typename AdjacencyMap, typename Expression>
   using insert_dependendencies_t
      = typename insert_dependendencies<AdjacencyMap, split_involved_parameters_t<Expression>>::type;



   // this is used as a topological sort of nodes
   template <typename AdjacencyMap, typename Entry, typename FlatPath = meta::type_set<> >
   struct depth_first_search
   {
      template <typename List, typename D>
      using f = typename depth_first_search< AdjacencyMap, D, meta::force_insert_t<List,D> >::type;

      using type = meta::fold_t< f, FlatPath, meta::type_at_t<AdjacencyMap, Entry> >;
   };

   template <typename AdjacencyMap, typename T0, typename FlatPath = meta::type_set<> >
   using depth_first_search_t = typename depth_first_search<AdjacencyMap, T0, FlatPath>::type;


   // ---------------------------------------------------------------------------------------------


   struct is_valid_expression : or_
   <  when< assign_parameter, std::true_type() >
   ,  when< just_parameter, std::true_type() >
   ,  otherwise< std::false_type() >
   >
   {};

   template <typename Expr>
   constexpr bool is_valid_expression_v = std::result_of_t<is_valid_expression(Expr)>::value;




   struct assign_to_value;
   struct extract_value;
   struct extract_parameter_value;

   // TODO inject context with state (parameters + lookup function)

   struct eval_rhs_expr : or_
   <  when< just_parameter, extract_parameter_value(_value,_state) >
   ,  when< _ , _default<eval_rhs_expr> >
   >
   {};

   struct eval_assign_expr : when< assign<_,_> , eval_rhs_expr(_right,_state) >
   {};


   struct eval : or_
   <  when< just_parameter, extract_value(_value) >
   ,  when< assign<_,_> , assign_to_value(_left,_right) >
   ,  when< _ , _default<eval> >
   >
   {};



   struct extract_value : callable_decltype
   {
      template <typename T>
      decltype(auto) operator()( T const& p ) const
      {
         return p.value;
      }

      template <typename T>
      auto& operator()( T& p )
      {
         return p.value;
      }
   };

   struct extract_value_trafo : or_
   <  when< assign_parameter, extract_value_trafo(_left) >
   ,  when< just_parameter, extract_value(_value) >
   >
   {};

   constexpr auto get_value  = building_blocks::extract_value_trafo{};

   struct unlift_parameter : when< just_parameter, _value > {};

   constexpr auto get_parameter = building_blocks::unlift_parameter{};



   struct assign_to_value : callable_decltype
   {
      template <typename Lhs, typename Rhs>
      auto operator()( Lhs& lhs, Rhs const& rhs ) const
      {
         eval e;
         extract_value_trafo v;
         auto result = e(rhs);
         v(lhs) = result;
         //std::cout << "assign_to_value &: " << &v(lhs) << std::endl;
         return result;
      }
   };



   struct parameter_node : or_
   <  when< assign_parameter, parameter_node(_left) >
   ,  when< just_parameter, _value >
   ,  otherwise< meta::type_set<>() >    // TODO should be error
   >
   {};

   template <typename Expr>
   using parameter_node_t = std::result_of_t<parameter_node(Expr)>;




   template <typename Expressions>
   struct parameters;

   template <typename... Expressions>
   struct parameters<std::tuple<Expressions...>>
   {
      using type = meta::type_list<std::decay_t<parameter_node_t<Expressions>>...>;
   };

   template <typename Expressions>
   using parameters_t = typename parameters<Expressions>::type;



   struct extract_parameter_value : callable_decltype
   {
      template <typename Key, typename State>
      decltype(auto) operator()( Key const& key, State const& state ) const
      {
         using keys_t = parameters_t<State>;
         constexpr std::size_t n = meta::index_of_v<keys_t, Key>;

         auto const& expr = std::get<n>(state);
         return get_value(expr);
      }
   };






   // TODO
   // • create list of sources S
   // • for (s:S) deps.set(s, eval_rhs(s))

   struct rhs_dependendencies : or_
   <  when< assign_parameter, collect_dependendencies(_right) >
   ,  otherwise< meta::type_set<>() >
   >
   {};

   template <typename Expr>
   using rhs_dependendencies_t = std::result_of_t<rhs_dependendencies(Expr)>;


   template <typename Sources, typename Expression>
   struct insert_source
   {
      using type = std::conditional_t
      <  meta::size_v<rhs_dependendencies_t<Expression>> == 0
      ,  meta::insert_t<Sources, std::decay_t<parameter_node_t<Expression>>>
      ,  Sources
      >;
   };

   template <typename Sources, typename Expression>
   using insert_source_t = typename insert_source<Sources, Expression>::type;


/*
   struct is_source_parameter : or_
   <  when< assign_parameter, std::true_type() >
   ,  when< just_parameter, std::true_type() >
   ,  otherwise< std::false_type() >
   >
   {};

   template <typename Expr>
   constexpr bool is_source_parameter_v = std::result_of_t<is_source_parameter(Expr)>::value;
*/


   template <typename F, typename... Args>
   struct result_type_wrapper : F
   {
      using result_type = decltype( std::declval<F>()( std::declval<Args>()... ) );
      using F::operator();
      result_type_wrapper(F const& f) : F(f) {}
   };

   template <typename Function>
   auto lazy_fun(Function f)
   {
      return [f=std::move(f)](auto&&... args)
      {
         using namespace boost::proto;
         using wrapped_func_t = result_type_wrapper<decltype(f), decltype(eval{}(args))...>;
         return make_expr<tag::function>( wrapped_func_t{f}, std::forward<decltype(args)>(args)... );
      };
   }

}



using building_blocks::parameter;
using building_blocks::make_terminal;
using building_blocks::lazy_fun;
using building_blocks::get_value;
using building_blocks::get_parameter;
using building_blocks::eval;
using building_blocks::eval_assign_expr;
using building_blocks::insert_dependendencies_t;
using building_blocks::insert_source_t;
using building_blocks::parameters_t;
using building_blocks::extract_parameter_value;
using building_blocks::is_valid_expression_v;
using building_blocks::depth_first_search_t;


#define PARAMETER(TYPE, NAME)  \
   decltype(make_terminal(parameter<struct NAME##_t, TYPE>{}))  \
   NAME = make_terminal(parameter<struct NAME##_t, TYPE>{});



/*
BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );
BOOST_PROTO_DEFINE_ENV_VAR( delayed_input_t, delayed_input );

,  when
   <  terminal< placeholder<_> >
   ,  place_the_holder( _value , _env_var<current_input_t> )
   >

e( l, 0, ( current_input = tuple_take<input_arity_t<LeftExpr>::value>(input)
         , delayed_input = std::tie( left_delayed_input, left_state )
         ))

*/

template <typename AdjacencyMap, typename Expressions>
struct dependency_manager
{
   using map_t = AdjacencyMap;
   using exprs_t = Expressions;
   using sources_t = meta::fold_t< insert_source_t, meta::type_set<>, exprs_t >;

   exprs_t  exprs;

   dependency_manager(exprs_t&& es) : exprs{ std::forward<exprs_t>(es) }
   {
      //std::cout << type_name<sources_t>() << std::endl;

      meta::for_each(sources_t{}, [this](auto&& s){
         using key_t = std::decay_t<decltype(s)>;
         using keys_t = parameters_t<exprs_t>;
         constexpr size_t n = meta::index_of_v<keys_t, key_t>;
         set<key_t>( eval_assign_expr{}(std::get<n>(exprs),exprs) );
      });
   }

   template <size_t N>
   void update_parameter()
   {
      get_value(std::get<N>(exprs)) = eval_assign_expr{}(std::get<N>(exprs),exprs);
   }

   template <typename keys_t>
   void update_parameters(meta::type_set<>&&)
   {}

   template <typename keys_t, typename P, typename... Ps>
   void update_parameters(meta::type_set<P,Ps...>&&)
   {
       // TODO C++17 use fold expression
       update_parameter<meta::index_of<keys_t, P>::value>();
       update_parameters<keys_t>(meta::type_set<Ps...>{});
   }

   /*
   // #ifdef C++17
   template <typename Parameter, typename Value>
   struct ParamValuePair {
      Parameter&& p;
      Value&& v;
   };

   template <typename... Parameter, typename... Value>
   void set(ParamValuePair<Parameter,Value>... pv)
   {

   }
   // #endif
   */


   template <typename Parameter, typename Value>
   void set(Parameter&& parameter, Value&& x)
   {
      set<decltype(get_parameter(parameter))>( std::forward<Value>(x) );
   }

   template <typename Parameter, typename Value>
   void set(Value&& x)
   {
      using key_t = std::decay_t<Parameter>;
      static_assert( meta::contains_v<key_t, sources_t>, "The given parameter is not a source.");

      using keys_t = parameters_t<exprs_t>;
      constexpr size_t n = meta::index_of_v<keys_t, key_t>;

      auto& expr = std::get<n>(exprs);
      get_value(expr) = x;

      using deep_dependees_t = depth_first_search_t<map_t, key_t>;
      //std::cout << type_name<deep_dependees_t>() << std::endl;

      update_parameters<keys_t>(deep_dependees_t{});
   }


   template <typename Parameter>
   auto get(Parameter&& parameter)
   {
      using key_t = std::decay_t<decltype(get_parameter(parameter))>;

      return extract_parameter_value{}(key_t{}, exprs);
   }
};


// expressions must have form
// v1 = assign-expr1
// ...
// vn = assign-exprn
//
// 1. left side is exactyl one leaf node of type parameter<_>
// 2. each parameter appears exacly once on left side
//
// Input constraints:
// • parameter
// • parameter = expr( w/o =, other parameter)
//


template <typename... Expressions>
auto make_reactive_expressions( Expressions&&... es )
{
   static_assert( meta::fold_and_v< is_valid_expression_v<Expressions>...>
                , "At least one expression malformed. "
                  "Well formed expressions are either `param = expr` or just `param`."
                );
   using expr_list_t = meta::type_list<Expressions...>;
   using dependency_map_t = meta::fold_t< insert_dependendencies_t, meta::type_map<>, expr_list_t >;

   using expression_t = std::tuple<Expressions... >;
   return dependency_manager<dependency_map_t, expression_t>{ std::forward_as_tuple(es...) };
}

