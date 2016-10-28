//   -----------------------------------------------------------------------------------------------
//    Copyright 2015 André Bergner. Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//      --------------------------------------------------------------------------------------------

#pragma once

#include <tuple>
#include <type_traits>

//  ------------------------------------------------------------------------------------------------
// flatten_tuple -- flattens nested tuples out to on tuple containing all non tuple values (leafs)
//  ------------------------------------------------------------------------------------------------

namespace detail {

   template< typename Tuple >
   using index_t = std::make_index_sequence< std::tuple_size<std::decay_t<Tuple>>::value >;

   struct put_in_tuple {};
   struct try_tuple : put_in_tuple {};

   template< typename T >
   auto unpack( put_in_tuple, T&& t )
   {
      return std::forward_as_tuple(std::forward<T>(t));
   }

   template< typename T, std::size_t I = std::tuple_size<std::decay_t<T>>{} >
   auto unpack( try_tuple, T&& t );

   template< typename T, std::size_t... Is >
   auto unpack(T&& t, std::index_sequence<Is...>)
   {
      return std::tuple_cat( unpack( try_tuple{}, std::get<Is>(std::forward<T>(t)) )... );
   }

   template< typename T, std::size_t I >
   auto unpack( try_tuple, T&& t )
   {
      return unpack( std::forward<T>(t), std::make_index_sequence<I>{} );
   }

   template< typename Tuple, std::size_t... Is >
   auto decay_tuple( Tuple&& t, std::index_sequence<Is...> )
   {
      return std::make_tuple( std::get<Is>(std::forward<Tuple>(t))... );
   }

   template< typename Tuple >
   auto decay_tuple( Tuple&& t )
   {
      return decay_tuple( std::forward<Tuple>(t), index_t<Tuple>{} );
   }

   template< typename Tuple, std::size_t... Is >
   auto flatten_tuple( Tuple&& t, std::index_sequence<Is...> )
   {
      using namespace std;
      return decay_tuple( tuple_cat( unpack( try_tuple{}, get<Is>(forward<Tuple>(t)) )... ));
   }

}

template< typename Tuple >
auto flatten_tuple( Tuple&& t )
{
   using namespace std;
   return detail::flatten_tuple( forward<Tuple>(t), detail::index_t<Tuple>{} );
}



//  ------------------------------------------------------------------------------------------------
// tuple_take -- (compile-time) take for tuples
//  ------------------------------------------------------------------------------------------------

namespace detail {

   template< typename Tuple, std::size_t... Ns >
   auto tuple_take_impl( Tuple&& t , std::index_sequence<Ns...> )
   {
      return std::make_tuple( std::get<Ns>(t)... );
   }

   template< typename Tuple, std::size_t... Ns >
   auto tuple_take_impl( Tuple& t , std::index_sequence<Ns...> )
   {
      return std::tie( std::get<Ns>(t)... );
   }

   template< size_t N , typename Tuple , typename
           = std::enable_if_t<std::tuple_size<std::decay_t<Tuple>>::value >= N> >
   auto tuple_take( Tuple&& t , int )
   {
      return tuple_take_impl( std::forward<Tuple>(t), std::make_index_sequence<N>{} );
   }

   template< size_t N , typename Tuple >
   auto tuple_take( Tuple&& t, char )
   {
      return std::forward<Tuple>(t);
   }

}


template< size_t N , typename Tuple >
auto tuple_take( Tuple&& t )
{
   return detail::tuple_take<N>( std::forward<Tuple>(t) , 0 );
}


//  ------------------------------------------------------------------------------------------------
// tuple_drop -- (compile-time) drop for tuples
//  ------------------------------------------------------------------------------------------------

namespace detail {

   template< size_t N , typename Tuple, std::size_t... Ns >
   auto tuple_drop_impl( Tuple&& t , std::index_sequence<Ns...> )
   {
      using namespace std;
      return std::make_tuple( std::get<N+Ns>(t)... );
   }

   template< size_t N , typename Tuple, std::size_t... Ns >
   auto tuple_drop_impl( Tuple& t , std::index_sequence<Ns...> )
   {
      using namespace std;
      return std::tie( std::get<N+Ns>(t)... );
   }

   template< size_t N , typename Tuple , typename
           = std::enable_if_t<std::tuple_size<std::decay_t<Tuple>>::value >= N> >
   auto tuple_drop( Tuple&& t , int )
   {
      using namespace std;
      using index_t = make_index_sequence< tuple_size<decay_t<Tuple>>::value - N >;
      return tuple_drop_impl<N>( forward<Tuple>(t), index_t{} );
   }

   template< size_t N , typename Tuple >
   auto tuple_drop( Tuple&& , char )
   {
      return std::tuple<>{};
   }

}


template< size_t N , typename Tuple >
auto tuple_drop( Tuple&& t )
{
   return detail::tuple_drop<N>( std::forward<Tuple>(t) , 0 );
}


//  ------------------------------------------------------------------------------------------------
// tuple_cat_t -- (compile-time) concatination of tuples
//  ------------------------------------------------------------------------------------------------

template< typename... Tuples >
using tuple_cat_t = decltype( std::tuple_cat( std::declval<Tuples>()... ));



//  ------------------------------------------------------------------------------------------------
// repeat<N,T>  --  creates a tuple of type T repeated N times.
//  ------------------------------------------------------------------------------------------------

template < std::size_t N , typename T >
struct repeat;

template < std::size_t N , typename T >
using  repeat_t = typename repeat<N,T>::type;


template < std::size_t N , typename T >
struct repeat
{
   using type = tuple_cat_t< repeat_t<N-1,T> , std::tuple<T> >;
};

template < typename T >
struct repeat<0,T> { using type = std::tuple<>; };

//  ------------------------------------------------------------------------------------------------
// tuple_for_each  --  apply n-ary function on each value in list of n tuples of same size
//  ------------------------------------------------------------------------------------------------

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


template< typename F, typename... Tuples >
void tuple_for_each( F&& f , Tuples&&... tuples )
{
   using namespace std;
   using min_size = std::integral_constant< size_t, min({ tuple_size<decay_t<Tuples>>::value... }) >;
   tuple_for_each_impl< min_size::value >::apply( forward<F>(f), forward<Tuples>(tuples)... );
}


//  ------------------------------------------------------------------------------------------------
// tuple_drop -- (compile-time) drop for tuples
//  ------------------------------------------------------------------------------------------------


template < typename... Ts >
auto head( std::tuple<Ts...> const & t )
{
   return  std::get<0>(t);
}

template <  typename... Ts , std::size_t... Ns >
auto tail_impl( std::index_sequence<Ns...> , std::tuple<Ts...> const & t )
{
   return  std::forward_as_tuple( std::get<Ns+1u>(t)... );
}

template < typename... Ts >
auto tail( std::tuple<Ts...> const & t )
{
   return  tail_impl( std::make_index_sequence<sizeof...(Ts) - 1u>() , t );
}





//  ------------------------------------------------------------------------------------------------
// scan -- (compile-time) scan (partial fold) for tuples
//  ------------------------------------------------------------------------------------------------

template < typename F, typename X >
auto scan( std::tuple<> const & t , X x , F&& f )
{
   return std::make_tuple();
}

template < typename F, typename X, typename... Ts >
auto scan( std::tuple<Ts...> const & t , X x , F&& f )
{
   auto y = f(x,std::get<0>(t));
   return tuple_cat( std::make_tuple(y) , scan( tuple_drop<1>(t), y, std::forward<F>(f) ));
}




//  ------------------------------------------------------------------------------------------------
// deep_tie -- 
//  ------------------------------------------------------------------------------------------------


template < std::size_t... Ns, typename... Ts >
auto deep_tie_impl( std::index_sequence<Ns...>, std::tuple<Ts...>& t )
{
   return  std::tie( std::get<Ns>(t)... );
}

template < typename... Ts >
auto deep_tie( std::tuple<Ts...>& t )
{
   return deep_tie_impl( std::make_index_sequence< sizeof...(Ts) >() , t );
}

template < typename T >
auto deep_tie( T& t ) { return t; }

//  ------------------------------------------------------------------------------------------------
// print(tuple) -- debug
//  ------------------------------------------------------------------------------------------------


template < typename... Ts >
auto operator<<( std::ostream& os , std::tuple<Ts...> const & t ) -> std::ostream&;

template < typename T , std::size_t N >
auto operator<<( std::ostream& os , std::array<T,N> const & t ) -> std::ostream&;

template < std::size_t... Ns, typename Tuple >
void print_tuple( std::index_sequence<Ns...> , std::ostream& os, Tuple const & t )
{
   std::tie( (os << std::get<Ns>(t) << ((Ns+1<std::tuple_size<Tuple>::value)? " ":"") )... );
}

template < typename... Ts >
auto operator<<( std::ostream& os , std::tuple<Ts...> const & t ) -> std::ostream&
{
   os << "(";
   print_tuple( std::make_index_sequence< sizeof...(Ts) >(), os, t );
   os << ")";
   return os;
}

template < typename... Ts >
void print( std::tuple<Ts...> t ) { std::cout << t << std::endl; }



template < typename T , std::size_t N >
auto operator<<( std::ostream& os , std::array<T,N> const & t ) -> std::ostream&
{
   os << "(";
   print_tuple( std::make_index_sequence<N>(), os, t );
   os << ")";
   return os;
}


