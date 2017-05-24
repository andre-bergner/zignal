// TODO
// • check for cycles
// • detect source parameters
// • allow set only for sources
// • recursively update all dependees
//   --> requires topological sort of nodes



// ------------------------------------------------------------------------------------------------
// PARAMETER DAG -- build compile time description of dependencies using boost.proto 
// ------------------------------------------------------------------------------------------------

#include "demangle.h"
#include "meta.h"
#include <iostream>
#include <array>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/fold.hpp>

namespace building_blocks
{
   using namespace boost::proto;

   //  ---------------------------------------------------------------------------------------------
   // copy_domain -- all expressions should be hold by value to avoid dangling references
   //  ---------------------------------------------------------------------------------------------

   template< typename E >
   struct copy_expr;

   struct copy_generator : pod_generator< copy_expr > {};

   struct copy_domain : domain< copy_generator >
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
         using type = decltype( std::declval<This>()( std::declval<Args>()... ));
      };
   };


   using just_parameter   = terminal<parameter<_,_>>;
   using assign_parameter = assign< terminal<parameter<_,_>> , _ >;



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


   struct collect_dependendencies : or_
   <  when< just_parameter, make_set(_value) >
   ,  when< terminal<_>, meta::type_set<>() >
   ,  when
      <  binary_expr<_,_,_>
      ,  merge_set( collect_dependendencies(_left), collect_dependendencies(_right) )
      >
   ,  when< _ , _default<collect_dependendencies> >
   >
   {};

   template <typename Expr>
   using collect_dependendencies_t
      = meta::invoke_result_t<collect_dependendencies, Expr>;


   struct rhs_dependendencies : or_
   <  when< assign_parameter, collect_dependendencies(_right) >
   ,  otherwise< meta::type_set<>() >
   >
   {};


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

   struct insert_dependendencies_impl : callable_decltype
   {
      template <typename Lhs, typename Rhs, typename AdjacencyMap>
      auto operator()( Lhs&&, Rhs&&, AdjacencyMap&& ) const
      {
         return insert_dependee_t< std::decay_t<Lhs>, collect_dependendencies_t<Rhs>
                                 , std::decay_t<AdjacencyMap> >{};
      }

   };

   struct insert_dependendencies : or_
   <  when< assign< just_parameter , _ >
          , insert_dependendencies_impl(_value(_left), _right, _state)
          >
   ,  otherwise< _state >
   >
   {};

   template <typename AdjacencyMap, typename Expression>
   using insert_dependendencies_t
      = meta::invoke_result_t<insert_dependendencies, Expression, AdjacencyMap>;





   struct insert_expression_impl : callable_decltype
   {
      template <typename Lhs, typename Rhs, typename ExpressionsMap>
      auto operator()( Lhs&&, Rhs&& rhs, ExpressionsMap&& ) const
      {
         using key_t = std::decay_t<Lhs>;
         using expr_t = std::decay_t<Rhs>;
         return meta::insert_t< std::decay_t<ExpressionsMap>, key_t, expr_t >{};
      }
   };


   // TODO: almast same structure as insert_dependendencies, options:
   //  * return tuple (left,right)  or empty
   //  * make called transform a template parameter
   struct build_expression_map : or_
   <  when< assign< just_parameter , _ >
          , insert_expression_impl(_value(_left), _right, _state)
          >
   ,  otherwise< _state >
   >
   {};


   template <typename Map, typename Expression>
   using build_expression_map_t
      = meta::invoke_result_t<build_expression_map, Expression, Map>;



   // ---------------------------------------------------------------------------------------------


   struct is_valid_expression : or_
   <  when< assign_parameter, std::true_type() >
   ,  when< just_parameter, std::true_type() >
   ,  otherwise< std::false_type() >
   >
   {};

   template <typename Expr>
   constexpr bool is_valid_expression_v = meta::invoke_result_t<is_valid_expression, Expr>::value;



   struct assign_to_value;
   struct extract_value;


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

   struct extract_parameter_trafo : when< just_parameter, _value > {};

   constexpr auto get_parameter = building_blocks::extract_parameter_trafo{};



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


   struct dependee : or_
   <  when< assign_parameter, extract_parameter_trafo(_left) >
   ,  otherwise< meta::type_set<>() >
   >
   {};

   template <typename Expr>
   using dependee_t = meta::invoke_result_t<dependee, Expr>;



   struct parameter_node : or_
   <  when< assign_parameter, parameter_node(_left) >
   ,  when< just_parameter, _value >
   ,  otherwise< meta::type_set<>() >    // TODO should be error
   >
   {};

   template <typename Expr>
   using parameter_node_t = meta::invoke_result_t<parameter_node, Expr>;

}



using building_blocks::parameter;
using building_blocks::make_terminal;
using building_blocks::get_value;
using building_blocks::get_parameter;
using building_blocks::eval;
using building_blocks::collect_dependendencies;
using building_blocks::rhs_dependendencies;
using building_blocks::insert_dependee_t;
using building_blocks::insert_dependendencies_t;
using building_blocks::build_expression_map_t;
using building_blocks::dependee_t;
using building_blocks::parameter_node_t;
using building_blocks::is_valid_expression_v;



#define PARAMETER(TYPE,NAME,VALUE)  \
   struct NAME##_t {};  \
   decltype(make_terminal(parameter<NAME##_t,TYPE>{}))  NAME = make_terminal(parameter<NAME##_t,TYPE>{VALUE});



template <typename Expressions>
struct dependees;

template <typename Expressions>
using dependees_t = typename dependees<Expressions>::type;

template <typename... Expressions>
struct dependees<std::tuple<Expressions...>>
{
   using type = meta::type_list<std::decay_t<dependee_t<Expressions>>...>;
};


template <typename Expressions>
struct parameters;

template <typename Expressions>
using parameters_t = typename parameters<Expressions>::type;

template <typename... Expressions>
struct parameters<std::tuple<Expressions...>>
{
   using type = meta::type_list<std::decay_t<parameter_node_t<Expressions>>...>;
};


// TODO
// sources
// all_params = soures && dependees



template <typename AdjacencyMap, typename Expressions>
struct dependency_manager
{
   using map_t = AdjacencyMap;
   using exprs_t = Expressions;

   exprs_t  exprs;

   template <typename Parameter, typename Value>
   void set(Parameter&& parameter, Value x)
   {
      //static_assert( is_source_parameter_v<Parameter>, "The given parameter is not a source.");
      using key_t = std::decay_t<decltype(get_parameter(parameter))>;
      using keys_t = parameters_t<exprs_t>;   // TODO use only sources_t
      constexpr size_t n = meta::index_of_v<keys_t, key_t>;

      eval e;
      meta::type_at_t<map_t, key_t> ex;
      // set value at n
      //    assign_to_value
      // update all dependees recursively

      auto const& expr = std::get<n>(exprs);
      //std::cout << "result: " << e( expr ) << std::endl;
   }


   template <typename Parameter>
   auto get(Parameter&& parameter)
   {
      using key_t = std::decay_t<decltype(get_parameter(parameter))>;
      using keys_t = parameters_t<exprs_t>;
      constexpr size_t n = meta::index_of_v<keys_t, key_t>;

      auto const& expr = std::get<n>(exprs);
      return get_value(expr);
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
auto make_mapping_dag( Expressions&&... es )
{
   static_assert( meta::fold_and_v< is_valid_expression_v<Expressions>...>
                , "At least one expression malformed. "
                  "Well formed expressions are either `param = expr` or just `param`."
                );
   using expr_list_t = meta::type_list<Expressions...>;
   using dependency_map_t = meta::fold_t< insert_dependendencies_t, meta::type_map<>, expr_list_t >;

   eval  e;
   [](...){}( (e(es),0)... );

   using expression_t = std::tuple<Expressions... >;
   return dependency_manager<dependency_map_t, expression_t>{ std::forward_as_tuple(es...) };
}



int main()
{
   PARAMETER( float, a, 1337 );
   PARAMETER( float, b, 47 );
   PARAMETER( float, c, 0 );
   PARAMETER( float, d, 0 );
   PARAMETER( float, a_1, 0 );
   PARAMETER( std::string, name, "hello" );

   std::cout << "----------------------------------" << std::endl;

   std::cout << get_value(a) << std::endl;
   std::cout << get_value(b) << std::endl;
   std::cout << get_value(a_1) << std::endl;
   std::cout << get_value(a_1) << std::endl;
   std::cout << get_value(c) << std::endl;
   std::cout << get_value(d) << std::endl;
   std::cout << get_value(name) << std::endl;

   auto deps = make_mapping_dag
   (  a    //= 1337
   ,  b    //= 47
   ,  a_1  = 1.f / a
   ,  c    = (b + 1.f) * a_1
   ,  d    = b*b
   ,  name = name + " world: " + std::to_string(get_value(a))     // recursive --> should be error
   );

   std::cout << "----------------------------------" << std::endl;

   deps.set(a_1, 3);

   std::cout << deps.get(a) << std::endl;
   std::cout << deps.get(b) << std::endl;
   std::cout << deps.get(a_1) << std::endl;
   std::cout << deps.get(c) << std::endl;
   std::cout << deps.get(d) << std::endl;
   std::cout << deps.get(name) << std::endl;
}

