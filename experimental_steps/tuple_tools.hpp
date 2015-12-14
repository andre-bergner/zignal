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
   auto tuple_drop_impl( Tuple&& t , std::index_sequence<Ns...> )
   {
      using namespace std;
      return make_tuple( std::get<N+Ns>(t)... );
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


template < bool... bs >
using all_of_ = std::is_same< std::integer_sequence<bool, bs...>
                            , std::integer_sequence<bool, (bs,true)...>
                            >;


template< typename F, typename... Ts , typename... Tuples >
void tuple_for_each( F&& f , std::tuple<Ts...>& t , Tuples&&... tuples )
{
   static_assert( all_of_<(sizeof...(Ts) == std::tuple_size<Tuples>::value)...>::value
                , "all Tuples must have same size"
                );
   tuple_for_each_impl< sizeof...(Ts) >::apply( std::forward<F>(f), t, std::forward<Tuples>(tuples)... );
}


//  ------------------------------------------------------------------------------------------------
// tuple_cat_t -- (compile-time) concatination of tuples
//  ------------------------------------------------------------------------------------------------

template< typename... Tuples >
using tuple_cat_t = decltype( std::tuple_cat( std::declval<Tuples>()... ));



//  ------------------------------------------------------------------------------------------------
// tuple_drop -- (compile-time) drop for tuples
//  ------------------------------------------------------------------------------------------------


template < typename... Ts >
auto head( std::tuple<Ts...> const & t )
{
   return  std::get<0>(t);
}

template <  typename... Ts , std::size_t... Ns >
auto tail_impl( std::index_sequence<Ns...> , std::tuple<Ts...> t )
{
   return  std::forward_as_tuple( std::get<Ns+1u>(t)... );
}

template < typename... Ts >
auto tail( std::tuple<Ts...> const & t )
{
   return  tail_impl( std::make_index_sequence<sizeof...(Ts) - 1u>() , t );
}




//namespace { struct placeholder_arg {} _; }

//std::ostream& operator<<( std::ostream& o , placeholder_arg ) { o << '_'; return o; }

void print( std::tuple<> t )
{
   std::cout << std::endl;
}

template < typename T , typename... Ts >
void print( std::tuple<T , Ts...> t )
{
   std::cout << std::get<0>(t) << "  ";
   print( tail(t) );
}







