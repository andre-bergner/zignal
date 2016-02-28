
#include <type_traits>
#include <iostream>

#include <flowz/tuple_tools.hpp>
#include <flowz/dirac.hpp>
#include <flowz/demangle.h>

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

template< std::size_t N , typename Tuprix , typename Tuple >
auto tuprix_impl( std::integral_constant<size_t,1> , Tuprix t , Tuple xs )
{
   return  std::make_tuple(xs);
}

template< std::size_t N , std::size_t M , typename Tuprix , typename Tuple >
auto tuprix_impl( std::integral_constant<size_t,M> , Tuprix t , Tuple xs )
{
   return std::tuple_cat( std::make_tuple( tuple_take<N>(xs) ) , tuprix_impl<N>(std::integral_constant<size_t,M-1>{},t,tuple_drop<N>(xs)) );
}


template< std::size_t N , std::size_t M , typename... Xs >
auto tuprix( Xs&&... xs )
{
   static_assert( sizeof...(xs) == M*N , "Number of elements do not match specified size constraints.");
   return tuprix_impl<N>( std::integral_constant<size_t,M>{} , std::make_tuple() , std::forward_as_tuple(xs...) );
}


template< typename... Xs >
auto tup( Xs&&... xs ) { return std::make_tuple( xs... ); }




int main()
{
   auto a = tuprix<2,2>( 0.6  , -0.3
                       , one , zero );

   auto b = tuprix<2,2>( 0.5  , 0.5
                       , zero , zero );

   auto v = tup( 0.0 , 0.0 );

   for ( auto const & x : dirac(10) )
   {
      std::cout << std::get<0>(v) << std::endl;
      v = a*v + b*std::tie( x , zero );
   }
}



