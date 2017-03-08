// compile time parameter compute DAGs
// Problem:
//  * given set of parameters that depend on each other
//  * changing one or more of the parameter the depedent ones need to be recomputed
//  * don't recompute all, usually done by hand, error prone
//  * unstable under refactoring
//  * hard to unit test → all possible set update combinations need to be tested

// Example:

struct Effect {
   
   float  a;
   float  b;
   float  a_1;    // depends on { a }
   float  c;      // depends on { a_1, b }
   float  d;      // depends on { b }

   void set_a(float a_)
   {
      a = a_;
      a_1 = 1.f / a;
      compute_c();
   }

   void set_b(float b_)
   {
      b = b_;
      compute_c();
      compute_d();
   }

   void compute_c()
   {
      c = (b + 1.f) * a_1;
   }

   void compute_d()
   {
      d = b*b;
   }

};


/*
// ideal version:
// not possible at compile time: no way for dependee to automatically dependers and trigger their recomputation
struct Effect2 {
   
   property<float>  a;
   property<float>  b;
   property<float>  a_1 = 1.f / a;
   property<float>  c = (b + 1.f) * a_1;
   property<float>  d = b*b;

};
*/










// ------------------------------------------------------------------------------------------------
// PARAMETER DAG -- build compile time description of dependencies using boost.proto 
// ------------------------------------------------------------------------------------------------


#include "demangle.h"
#include "meta.h"
#include <iostream>
#include <boost/proto/core.hpp>
#include <boost/proto/transform.hpp>
#include <boost/proto/debug.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/fold.hpp>

namespace building_blocks
{
   using namespace boost::proto;

   //  ---------------------------------------------------------------------------------------------
   // copy_domain -- all flowz expressions should be hold by value to avoid dangling references
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

      //parameter(Value v) : value{v} { std::cout << "ctor" << std::endl; }
      //parameter() { std::cout << "ctor" << std::endl; }
      //parameter(parameter const& t) : value(t.value) { std::cout << "copy" << std::endl; }
      //parameter(parameter&&) { std::cout << "move" << std::endl; }
      //template <typename Any>
      //operator Value() const { return value; }
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
   <  when< terminal<parameter<_,_>>, make_set(_value) >
   ,  when< terminal<_>, meta::type_set<>() >
   ,  when
      <  binary_expr<_,_,_>
      ,  merge_set( collect_dependendencies(_left), collect_dependendencies(_right) )
      >
   ,  when< _ , _default<collect_dependendencies> >
   >
   {};

   struct rhs_dependendencies : or_
   <  when< assign< terminal<parameter<_,_>>,_>, collect_dependendencies(_right) >
   ,  otherwise< meta::type_set<>() >
   >
   {};

   // ---------------------------------------------------------------------------------------------


   // for (D : DependencySet) AdjacencyMap[D].insert( Dependee )
   template <typename Dependee, typename DependencySet, typename AdjacencyMap = meta::type_map<> >
   struct collect_dependees
   {
      template <typename Map, typename Key>
      using inserter_dependee_t = meta::insert_t< meta::type_at_t<Map,Key,meta::type_set<>>, Dependee >;

      template <typename Map, typename Key>
      using inserter_t = meta::force_insert_t< Map, Key, inserter_dependee_t<Map,Key> >;

      using type = meta::fold_t< inserter_t, AdjacencyMap, DependencySet >;
   };

   template <typename Dependee, typename DependencySet, typename AdjacencyMap = meta::type_map<> >
   using collect_dependees_t = typename collect_dependees<Dependee, DependencySet, AdjacencyMap>::type;




   // ---------------------------------------------------------------------------------------------
   // expression  ( prop[n] = f(prop[..]), adj_map ) -> adj_map
   //

   struct insert_dependendencies : callable_decltype
   {
      template <typename Lhs, typename Rhs, typename AdjacencyMap>
      auto operator()( Lhs&&, Rhs&& rhs, AdjacencyMap&& ) const
      {
         collect_dependendencies  cd;
         return collect_dependees_t< std::decay_t<Lhs>, decltype(cd(rhs)), std::decay_t<AdjacencyMap> >{};
      }

   };

   struct insert_dependendencies_trafo : or_
   <  when< assign< terminal<parameter<_,_>>,_>, insert_dependendencies(_value(_left),_right,_state) >
   ,  otherwise< _state >
   >
   {};



   template <typename AdjacencyMap, typename Expression>
   struct insert_dependendencies_trafo_result_type
   {
      using type = decltype( std::declval<insert_dependendencies_trafo>()
                               ( std::declval<Expression>(), std::declval<AdjacencyMap>() )
                           );
   };

   template <typename AdjacencyMap, typename Expression>
   using insert_dependendencies_trafo_t
      = typename insert_dependendencies_trafo_result_type<AdjacencyMap,Expression>::type;








   struct insert_expression : callable_decltype
   {
      template <typename Lhs, typename Rhs, typename ExpressionsMap>
      auto operator()( Lhs&&, Rhs&& rhs, ExpressionsMap&& ) const
      {
         using key_t = std::decay_t<Lhs>;
         using expr_t = std::decay_t<Rhs>;
         return meta::insert_t< std::decay_t<ExpressionsMap>, key_t, expr_t >{};
      }

   };


   // TODO: almast same structure as insert_dependendencies_trafo, options:
   //  * return tuple (left,right)  or empty
   //  * make called transform a template parameter
   struct build_expression_map_trafo : or_
   <  when< assign< terminal<parameter<_,_>>,_>, insert_expression(_value(_left),_right,_state) >
   ,  otherwise< _state >
   >
   {};



   // TODO: almast same structure as insert_dependendencies_trafo_result_type
   //       --> make this a lifter (lift into declval)
   template <typename Map, typename Expression>
   struct build_expression_map_trafo_result_type
   {
      using type = decltype( std::declval<build_expression_map_trafo>()
                               ( std::declval<Expression>(), std::declval<Map>() )
                           );
   };

   template <typename Map, typename Expression>
   using build_expression_map_trafo_t
      = typename build_expression_map_trafo_result_type<Map,Expression>::type;



   // ---------------------------------------------------------------------------------------------

   struct assign_to_value;
   struct extract_value;


   struct eval : or_
   <  when< terminal<parameter<_,_>>, extract_value(_value) >
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

   struct extract_value_trafo : when< terminal<parameter<_,_>>, extract_value(_value) > {};

   constexpr auto get_value  = building_blocks::extract_value_trafo{};

   struct extract_parameter_trafo : when< terminal<parameter<_,_>>, _value > {};

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



}

using building_blocks::parameter;
//using building_blocks::term_type;
using building_blocks::make_terminal;
using building_blocks::get_value;
using building_blocks::get_parameter;
using building_blocks::eval;
using building_blocks::collect_dependendencies;
using building_blocks::rhs_dependendencies;
using building_blocks::collect_dependees_t;
using building_blocks::insert_dependendencies_trafo_t;
using building_blocks::build_expression_map_trafo_t;



#define PARAMETER(TYPE,NAME,VALUE)  \
   struct NAME##_t {};  \
   decltype(make_terminal(parameter<NAME##_t,TYPE>{}))  NAME = make_terminal(parameter<NAME##_t,TYPE>{VALUE});


// expressions must have form
// v1 = assign-expr1
// ...
// vn = assign-exprn
//
// 1. left side is exactyl one leaf node of type parameter<_>
// 2. each parameter appears exacly once on left side
//
//
/*
template <typename T=float>
struct mapping_dag
{
   template <typename... Expressions>
   mapping_dag( Expressions... es )
   {
      // Input constraints:
      // • parameter
      // • parameter = expr( w/o =, other parameter)

      eval  e;
      collect_dependendencies  c;
      //rhs_dependendencies  rc;

      [](...){}( ( std::cout << e(es) << std::endl
                 , std::cout << "dependencies: " << type_name(c(es)) << std::endl
                 //, std::cout << "dependencies: " << type_name(rc(es)) << std::endl
                 //, boost::proto::display_expr(es)
                 , std::cout << "\n--------------------------" << std::endl
                 , 0
                 )...
               );

      using dependency_map = meta::fold_t< insert_dependendencies_trafo_t, meta::type_map<>, meta::type_list<Expressions...> >;

      std::cout << type_name<dependency_map>() << std::endl;

   }

   template <typename Tag>
   void set( term_type<Tag,T> p, T value )
   {
      //params[&p](value);  // sets the value by triggering call --> cascade of potential recomputations
   }

};
*/





template <typename AdjacencyMap, typename Expressions>
struct dependency_manager
{
   using map_t = AdjacencyMap;
   using exprs_t = Expressions;

   template <typename Terminal, typename Value>
   void set(Terminal term, Value x)
   {
      eval e;
      using tag_t = std::decay_t<decltype(get_parameter(term))>;
      meta::type_at_t<exprs_t, tag_t> ex;
      std::cout << "result: " << e( ex ) << std::endl;
   }

   exprs_t  exprs;
   map_t    map;
};




template <typename... Expressions>
auto make_mapping_dag( Expressions&&... es )
{
   using expr_list_t = meta::type_list<Expressions...>;
   using dependency_map_t = meta::fold_t< insert_dependendencies_trafo_t, meta::type_map<>, expr_list_t >;
   using expression_map_t = meta::fold_t< build_expression_map_trafo_t, meta::type_map<>, expr_list_t >;

   eval  e;
   [](...){}( (e(es),0)... );


   return dependency_manager<dependency_map_t, expression_map_t>{/*es...*/};
}




/*

struct Effect3 {

   //struct a_t {}; parameter<a_t,float> a = 1337;
   
   PARAMETER( float, a, 1337 );
   PARAMETER( float, b, 47 );
   PARAMETER( float, c, 0 );
   PARAMETER( float, d, 0 );
   PARAMETER( float, a_1, 0 );
   PARAMETER( std::string, name, "hello" );

   mapping_dag<float> dag
   {  a
   ,  b
   ,  a_1  = 1.f / a
   ,  c    = (b + 1.f) * a_1
   ,  d    = b*b
   ,  name = name + " world: " + std::to_string(get_value(a))
   };

   //void set_a(float a_) { dep.set(a,a_); assert( new value of dependent values) }
   //void get_c() { dep.get(c); }

   using t = collect_dependees_t< c_t, meta::type_set<a_t,b_t> >;

};
*/

template <typename... Terminals>
void test_set_terminal( Terminals&&... ts )
{
   [](...){}( (get_value(ts) = 13 ,0)... );
}

int main()
{
   /*
   Effect3 ef3;
   std::cout << get_value(ef3.a_1) << std::endl;
   std::cout << get_value(ef3.c) << std::endl;
   std::cout << get_value(ef3.d) << std::endl;
   std::cout << get_value(ef3.name) << std::endl;

   std::cout << type_name<Effect3::t>() << std::endl;
   std::cout << std::boolalpha;
   std::cout << meta::contains_v< int,   meta::type_list<int,char> > << std::endl;
   std::cout << meta::contains_v< float, meta::type_list<int,char> > << std::endl;
   */



   PARAMETER( float, a, 1337 );
   PARAMETER( float, b, 47 );
   PARAMETER( float, c, 0 );
   PARAMETER( float, d, 0 );
   PARAMETER( float, a_1, 0 );
   PARAMETER( std::string, name, "hello" );

   //test_set_terminal(a,b);
/*
   std::cout << type_name(a) << std::endl;
*/
   std::cout << "----------------------------------" << std::endl;

   //std::cout << "extract_value &: " << &get_value(a)   << "     value: " << get_value(a)   << std::endl;
   //std::cout << "extract_value &: " << &get_value(b)   << "     value: " << get_value(b)   << std::endl;
   //std::cout << "extract_value &: " << &get_value(a_1) << "     value: " << get_value(a_1) << std::endl;
/*
   std::cout << get_value(a) << std::endl;
   std::cout << get_value(b) << std::endl;
   std::cout << get_value(a_1) << std::endl;
   std::cout << get_value(a_1) << std::endl;
   std::cout << get_value(c) << std::endl;
   std::cout << get_value(d) << std::endl;
   std::cout << get_value(name) << std::endl;
*/
   auto deps = make_mapping_dag
   (  a    //= 1337
   ,  b    //= 47
   ,  a_1  = 1.f / a
   ,  c    = (b + 1.f) * a_1
   ,  d    = b*b
   ,  name = name + " world: " + std::to_string(get_value(a))     // recursive --> should be error
   );

   std::cout << "----------------------------------" << std::endl;

   //std::cout << "extract_value &: " << &get_value(a)   << "     value: " << get_value(a)   << std::endl;
   //std::cout << "extract_value &: " << &get_value(b)   << "     value: " << get_value(b)   << std::endl;
   //std::cout << "extract_value &: " << &get_value(a_1) << "     value: " << get_value(a_1) << std::endl;

/*
   deps.set(a_1, 3);

   std::cout << "----------------------------------" << std::endl;
   std::cout << get_value(a) << std::endl;
   std::cout << get_value(b) << std::endl;
   std::cout << get_value(a_1) << std::endl;
   std::cout << get_value(c) << std::endl;
   std::cout << get_value(d) << std::endl;
   std::cout << get_value(name) << std::endl;

   //std::cout << "----------------------------------" << std::endl;
   //std::cout << type_name(deps.map) << std::endl;
   //std::cout << "----------------------------------" << std::endl;
   //std::cout << type_name(deps.exprs) << std::endl;
*/

}

