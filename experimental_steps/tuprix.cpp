
#include <type_traits>
#include <iostream>
#include <vector>

//#include "tuple_tools.hpp"


namespace ni {

   //------------------------------------------------------------------------------------------------------------------
   //  zero, one, two -- types and instances
   //------------------------------------------------------------------------------------------------------------------

   struct Zero { template< typename Number > operator Number() { return Number{0}; } };
   struct One  { template< typename Number > operator Number() { return Number{1}; } };

   namespace {
      Zero  zero;
      One   one;
   }

   std::ostream& operator<<( std::ostream& o , Zero ) { o << "𝟎"; return o; }
   std::ostream& operator<<( std::ostream& o , One  ) { o << "𝟏"; return o; }

   //------------------------------------------------------------------------------------------------------------------
   //  type traits
   //------------------------------------------------------------------------------------------------------------------

   template< typename T >
   using is_zero = std::is_same< std::decay_t<T> , Zero >;



   //------------------------------------------------------------------------------------------------------------------
   //  Algebra for Zero
   //------------------------------------------------------------------------------------------------------------------

   //------------------------------------------------------------------------------------------------------------------
   // addition

   template< typename X >
   auto operator+ ( X const & x , Zero ) -> X const &   { return  x; }

   template< typename X >
   auto operator+ ( Zero , X const & x ) -> X const &   { return  x; }

   auto operator+ ( Zero , Zero ) -> Zero               { return  {}; }


   //------------------------------------------------------------------------------------------------------------------
   // subtraction

   template< typename X >
   auto operator- ( X const & x , Zero ) -> X const &   { return  x; }

   template< typename X >
   auto operator- ( Zero , X const & x ) -> X           { return  -x;  }

   auto operator- ( Zero , Zero ) -> Zero               { return  {}; }

   auto operator- ( Zero ) -> Zero                      { return  {}; }


   //------------------------------------------------------------------------------------------------------------------
   // multiplication

   template< typename X , typename Y ,
     typename = std::enable_if_t< is_zero<X>::value or is_zero<Y>::value > >
   auto operator* ( X const & , Y const & ) -> Zero   { return {}; }




   //------------------------------------------------------------------------------------------------------------------
   //  Algebra for One
   //------------------------------------------------------------------------------------------------------------------

   //------------------------------------------------------------------------------------------------------------------
   // addition

   template< typename X >
   auto operator+ ( X const & x , One ) -> X    { return  x + X{1}; }

   template< typename X >
   auto operator+ ( One , X const & x ) -> X    { return  X{1} + x; }

   auto operator+ ( Zero , One  ) -> One        { return  {}; }
   auto operator+ ( One  , Zero ) -> One        { return  {}; }


   //------------------------------------------------------------------------------------------------------------------
   // subtraction

   //auto operator- ( One ) -> Minus_one        { return  {}; }

   template< typename X >
   auto operator- ( X const & x , One ) -> X    { return  x - X{1}; }

   template< typename X >
   auto operator- ( One , X const & x ) -> X    { return  X{1} - x; }

   auto operator- ( One  , One  ) -> Zero       { return  {}; }
   auto operator- ( Zero , One  ) -> int        { return  -1; }
   auto operator- ( One  , Zero ) -> One        { return  {}; }


   //------------------------------------------------------------------------------------------------------------------
   // multiplication

   template< typename X >
   auto operator* ( One , X const & x ) -> X const &   { return  x; }

   template< typename X >
   auto operator* ( X const & x , One ) -> X const &   { return  x; }

   auto operator* ( One , One )  -> One    { return {}; }
   auto operator* ( Zero , One ) -> Zero   { return {}; }
   auto operator* ( One , Zero ) -> Zero   { return {}; }


}


#include <tuple>







template < typename T , typename... Ts >
auto head( std::tuple<T,Ts...> t )
{
   return  std::get<0>(t);
}

template <  typename... Ts , std::size_t... Ns >
auto tail_impl( std::index_sequence<Ns...> , std::tuple<Ts...> const & t )
{
   return  std::tie( std::get<Ns+1u>(t)... );
   //return  std::make_tuple( std::get<Ns+1u>(t)... );
}

template < typename T, typename... Ts >
auto tail( std::tuple<T,Ts...> const & t )
{
   return  tail_impl( std::make_index_sequence<sizeof...(Ts)>() , t );
}




namespace tuple_ostream_detail {


   template < typename T >
   void print( std::ostream& o, std::tuple<T> const & t )
   {
      o << std::get<0>(t) << " )";
   }

   template < typename T , typename T1, typename... Ts >
   void print( std::ostream& o, std::tuple<T,T1,Ts...> const & t )
   {
      o << std::get<0>(t) << ", ";
      print( o, tail(t) );
   }

}

template < typename... Ts >
std::ostream& operator<<( std::ostream& o , std::tuple<Ts...> const & t )
{
   o << "( ";
   tuple_ostream_detail::print( o , t );
   return o;
}




template< std::size_t... Ns , typename... Xs, typename... Ys , typename F >
auto tuple_transform_impl( std::index_sequence<Ns...> , std::tuple<Xs...> const & x , std::tuple<Ys...> const & y , F&& f )
{
    return std::make_tuple( f(std::get<Ns>(x) , std::get<Ns>(y))... );
}

template< typename... Xs, typename... Ys , typename F >
auto tuple_transform( std::tuple<Xs...> const & x , std::tuple<Ys...> const & y , F&& f )
{
   return tuple_transform_impl( std::make_index_sequence<sizeof...(Xs)>() , x , y , std::forward<F>(f) );
}



template< std::size_t... Ns , typename... Xs, typename... Ys , typename F >
auto tuple_transform_impl( std::index_sequence<Ns...> , std::tuple<Xs...> const & x , F&& f )
{
    return std::make_tuple( f(std::get<Ns>(x))... );
}

template< typename... Xs, typename... Ys , typename F >
auto tuple_transform( std::tuple<Xs...> const & x , F&& f )
{
   return tuple_transform_impl( std::make_index_sequence<sizeof...(Xs)>() , x , std::forward<F>(f) );
}





template< typename... Xs, typename... Ys >
auto operator+( std::tuple<Xs...> const & x , std::tuple<Ys...> const & y )
{
   return tuple_transform( x, y, [](auto l, auto r){ return l+r; } );
}

template< typename... Xs, typename... Ys >
auto mul( std::tuple<Xs...> const & x , std::tuple<Ys...> const & y )
{
   return tuple_transform( x, y, [](auto l, auto r){ return l*r; } );
}



template< typename X >
auto sum( std::tuple<X> const & x )
{
   return std::get<0>(x);
}

template< typename X, typename X1, typename... Xs >
auto sum( std::tuple<X,X1,Xs...> const & xs )
{
   return std::get<0>(xs) + sum(tail(xs));
}


template< typename... Xs, typename... Ys >
auto dot( std::tuple<Xs...> const & x , std::tuple<Ys...> const & y )
{
   return sum( mul(x,y) );
}


template< bool... > struct bool_seq;

template< bool... bs >
struct and_ : std::integral_constant< bool ,
   std::is_same<
      bool_seq<  bs... >,
      bool_seq< (bs, true)... >
   >::value
>{};


template< typename > struct is_tuple : std::false_type {};
template< typename... Xs > struct is_tuple< std::tuple<Xs...> > : std::true_type {};


template< typename > struct is_tuprix : std::false_type {};
template< typename... Tuples > struct is_tuprix< std::tuple<Tuples...> > : and_< is_tuple<Tuples>::value... > {};



template< typename X, typename Y , typename = std::enable_if_t<is_tuprix<X>::value> >
auto operator*( X const & x , Y const & y )
{
   return tuple_transform( x , [&y](auto x){ return dot(x,y); } );
}


double foo()
{
   static auto x = 0.123;
   x *= 4.0*(1.-x);
   return x;
}

int main()
{/*
   auto t1 = std::make_tuple( ni::one, 3.14,     3.f  );
   auto t2 = std::make_tuple( 2      , ni::zero, 7u   );

   std::cout << t1 << std::endl;
   std::cout << t2 << std::endl;
   std::cout << t1+t2 << std::endl;
   std::cout << sum(t1) << std::endl;
   std::cout << sum(t2) << std::endl;
   std::cout << dot(t1,t2) << std::endl;
   std::cout << dot(t1,t1) << std::endl;

   auto m1 = std::make_tuple( t1, t2, t1 );
*/

   std::vector<double>  xs(1000);
   for ( auto& x : xs ) x = foo();

   auto m = std::make_tuple( std::make_tuple( ni::one, 0.6 )
                           , std::make_tuple( ni::zero, 0.3 ) );
   auto v = std::make_tuple( 1.0 , 1.0 );
   
   for ( auto const & x : xs ) v = m*v + std::tie( x , ni::zero );
   std::cout << m*v << std::endl;
   printf( "%f %f\n", std::get<0>(v), std::get<1>(v) );
/*
   std::cout << std::boolalpha << "---------------------------" << std::endl;
   std::cout << is_tuple<decltype(t1)>::value << std::endl;
   std::cout << is_tuple<decltype(t2)>::value << std::endl;
   std::cout << is_tuple<decltype(m1)>::value << std::endl;
   std::cout << is_tuprix<decltype(m1)>::value << std::endl;
*/
}



