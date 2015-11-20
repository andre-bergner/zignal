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
// tuple_drop -- (compile-time) drop for tuples
//  ------------------------------------------------------------------------------------------------

namespace detail {

   template< size_t N , typename Tuple, std::size_t... Ns >
   auto tuple_drop( Tuple&& t , std::index_sequence<Ns...> )
   {
      using namespace std;
      return make_tuple( std::get<N+Ns>(t)... );
   }

}

template< size_t N , typename Tuple >
auto tuple_drop( Tuple&& t )
{
   using namespace std;
   using index_t = make_index_sequence< tuple_size<decay_t<Tuple>>::value - N >;
   static_assert( tuple_size<decay_t<Tuple>>::value >= N , "N is larger than size of tuple" );
   return detail::tuple_drop<N>( forward<Tuple>(t), index_t{} );
}








