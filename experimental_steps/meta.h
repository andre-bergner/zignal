#pragma once

#include <type_traits>



namespace meta {

   template <bool...>
   struct bools {};

}


namespace meta {

   namespace detail {

      template <bool b>
      struct as_true { static constexpr bool value = true; };

   }

   template <bool... bs>
   struct fold_and : std::integral_constant
   <  bool
   ,  std::is_same
      <  bools< detail::as_true<bs>::value... >
      ,  bools< bs... >
      >::value
   >
   {};

   template <bool... bs>
   constexpr bool fold_and_v = fold_and<bs...>::value;



   template <bool... bs>
   struct fold_or : std::integral_constant
   <   bool
   ,   not fold_and< (not bs)... >::value
   >
   {};

   template <bool... bs>
   constexpr bool fold_or_v = fold_or<bs...>::value;

}


namespace meta {


   template <typename...> struct type_list {};
   template <typename...> struct type_set  {};
   template <typename...> struct type_map  {};


   // ----------------------------------------------------------------------------------------------
   // Declarations
   // ----------------------------------------------------------------------------------------------

   // insert adds an element T (or key value pair {T,V}) to a type list, set, or map.
   // If T exists already in the set or the map nothing will happen.
   template <typename TypeContainer, typename T, typename V = void>
   struct insert;

   template <typename TypeContainer, typename T, typename V = void>
   using insert_t = typename insert<TypeContainer, T, V>::type;


   template <typename TypeContainer, typename T, typename V = void>
   struct force_insert;

   template <typename TypeContainer, typename T, typename V = void>
   using force_insert_t = typename force_insert<TypeContainer, T, V>::type;


   // remove removes element T (or key value pair {T,V}) from type list, set, or map if present.
   // For a list it only removes the first occurence.
   template <typename TypeContainer, typename T>
   struct remove;

   template <typename TypeContainer, typename T>
   using remove_t = typename remove<TypeContainer, T>::type;


   // replace works as insert except that it overwrite the value in the map.
   template <typename TypeContainer, typename T, typename V = void>
   struct replace;

   template <typename TypeContainer, typename T, typename V = void>
   using replace_t = typename replace<TypeContainer, T, V>::type;


   template <typename TypeContainer, typename Key, typename Default = void>
   struct type_at;

   template <typename TypeContainer, typename Key, typename Default = void>
   using type_at_t = typename type_at<TypeContainer, Key, Default>::type;


   template <typename TypeContainer, typename Key>
   struct index_of;

   template <typename TypeContainer, typename Key>
   constexpr auto index_of_v = index_of<TypeContainer, Key>::value;


   template <template <typename...> class MetaFunction, typename Init, typename TypeContainer>
   struct fold;

   template <template <typename...> class MetaFunction, typename Init, typename TypeContainer>
   using fold_t = typename fold<MetaFunction, Init, TypeContainer>::type;



   // ----------------------------------------------------------------------------------------------
   // Helper
   // ----------------------------------------------------------------------------------------------

   template <typename, typename> struct type_pair {};


   namespace detail {
      template <typename T, typename U>
      struct as_type { using type = T; };

      template <typename T, typename U>
      using as_type_t = typename as_type<T,U>::type;
   }


   template <typename T, typename TypeContainer>
   struct contains;

   template <typename T, typename TypeContainer>
   constexpr bool contains_v = contains<T,TypeContainer>::value;

   template <typename T, typename... Us>
   struct contains< T, type_list<Us...> > : fold_or
   <  std::is_same
      <  type_list< detail::as_type_t<T,Us> >
      ,  type_list< Us >
      >::value...
   >
   {};



   // ----------------------------------------------------------------------------------------------
   // List Implementations
   // ----------------------------------------------------------------------------------------------


   template <typename T, typename... Us>
   struct insert< type_list<Us...>, T >
   {
      using type = type_list<Us...,T>;
   };

   template <typename T, typename... Us>
   struct insert< type_set<Us...>, T > : std::conditional
   <  contains_v<T, type_list<Us...>>
   ,  type_set<Us...>
   ,  type_set<Us...,T>
   >
   {};


   template <typename Value>
   struct index_of<type_list<>, Value>
   {
      using fail = std::integral_constant<bool, !std::is_same<Value,Value>::value>;
      static_assert(fail::value, "Value not found." );
   };

   template <typename Value, typename... Values>
   struct index_of<type_list<Value, Values...>, Value>
     : std::integral_constant< size_t, 0 >{};

   template <typename Value, typename Value0, typename... Values>
   struct index_of<type_list<Value0, Values...>, Value>
     : std::integral_constant< size_t, 1 + index_of_v<type_list<Values...>, Value> > {};


   // ----------------------------------------------------------------------------------------------
   // Map Implementations
   // ----------------------------------------------------------------------------------------------


   template <typename MapEntry>
   struct get_key;

   template <typename MapEntry>
   using get_key_t = typename get_key<MapEntry>::type;

   template <typename MapEntry>
   struct get_value;

   template <typename MapEntry>
   using get_value_t = typename get_value<MapEntry>::type;

   template <typename Key, typename Value>
   struct get_key< type_pair<Key,Value> >
   {
      using type = Key;
   };

   template <typename Key, typename Value>
   struct get_value< type_pair<Key,Value> >
   {
      using type = Value;
   };



   template <typename Key, typename Default>
   struct type_at< type_map<>, Key, Default >
   {
      using type = Default;
   };

   template <typename Key, typename Entry, typename... Entries, typename Default>
   struct type_at< type_map<Entry, Entries...>, Key, Default> : std::conditional
   <  std::is_same< Key, get_key_t<Entry> >::value
   ,  get_value_t<Entry>
   ,  type_at_t< type_map<Entries...>, Key, Default>
   >
   {};


   template <typename Key>
   struct index_of<type_map<>, Key>
   {
      using fail = std::integral_constant<bool, !std::is_same<Key,Key>::value>;
      static_assert(fail::value, "Key not found." );
   };

   template <typename Key, typename Value, typename... Entries>
   struct index_of<type_map<type_pair<Key,Value>, Entries...>, Key>
     : std::integral_constant< size_t, 0 >{};

   template <typename Key, typename Entry, typename... Entries>
   struct index_of<type_map<Entry, Entries...>, Key>
     : std::integral_constant< size_t, 1 + index_of_v<type_map<Entries...>, Key> > {};

/*
   template <typename Key>
   struct index_of< type_map<>, Key > : std::integral_constant<size_t, 0>
   {
      // Error
   };

   template <typename Key, typename Entry, typename... Entries>
   struct index_of< type_map<Entry, Entries...>, Key> : std::conditional
   <  std::is_same< Key, get_key_t<Entry> >::value
   ,  std::integral_constant<size_t, 0>
   ,  std::integral_constant<size_t, 1 + index_of_v< type_map<Entries...>, Key>>
   >
   {};
*/



   template <typename K, typename V, typename... Entries>
   struct insert< type_map<Entries...>, K, V > : std::conditional
   <  contains_v<K, type_list<typename get_key<Entries>::type...>>
   ,  type_map<Entries...>
   ,  type_map<Entries..., type_pair<K,V> >
   >
   {};




   template <typename Key, typename... Entries>
   struct remove< type_map<Entries...>, Key >
   {
      template <typename NewMap, typename Map>
      struct impl;

      template <typename NewMap, typename Map>
      using impl_t = typename impl<NewMap,Map>::type;

      template <typename NewMap>
      struct impl< NewMap, type_map<> >
      {
         using type = NewMap;
      };

      template <typename NewMap, typename E, typename... Es>
      struct impl< NewMap, type_map<E, Es...> > : std::conditional
      <  std::is_same< Key, get_key_t<E> >::value
      ,  impl_t< NewMap, type_map<Es...> >
      ,  impl_t< insert_t<NewMap, get_key_t<E>, get_value_t<E>>, type_map<Es...> >
      >
      {};

      using type = impl_t< type_map<>, type_map<Entries...> >;
   };


   template <typename K, typename V, typename... Entries>
   struct force_insert< type_map<Entries...>, K, V >
   {
      using type = insert_t< remove_t< type_map<Entries...>, K >, K, V>;
   };


   template <typename Key, typename Value, typename... Entries>
   struct replace< type_map<Entries...>, Key, Value >
   {
      template <typename NewMap, typename Entry>
      using replacer_t = std::conditional_t
      <  std::is_same< Key, get_key_t<Entry> >::value
      ,  insert_t< NewMap, Key, Value >
      ,  insert_t< NewMap, get_key_t<Entry>, get_value_t<Entry> >
      >;
      using type = fold_t< replacer_t, type_map<>, type_map<Entries...> >;
   };



   // ----------------------------------------------------------------------------------------------
   // Algorithms
   // ----------------------------------------------------------------------------------------------


   template <template <typename...> class MetaFunction, typename Init, typename TypeContainer>
   struct fold;

   template <template <typename...> class MetaFunction, typename Init, typename TypeContainer>
   using fold_t = typename fold<MetaFunction, Init, TypeContainer>::type;


   template < template <typename...> class MetaFunction, typename Init
            , template <typename...> class TypeContainer>
   struct fold< MetaFunction, Init, TypeContainer<> >
   {
      using type = Init;
   };

   template < template <typename...> class MetaFunction, typename Init
            , template <typename...> class TypeContainer, typename T, typename... Ts>
   struct fold< MetaFunction, Init, TypeContainer<T,Ts...> >
   {
      using type = typename fold<MetaFunction, MetaFunction<Init,T>, TypeContainer<Ts...> >::type;
   };






   // ----------------------------------------------------------------------------------------------
   // Invoke Return Type
   // ----------------------------------------------------------------------------------------------

   template <typename Function, typename... Args>
   using invoke_result_t = decltype( std::declval<Function>()(std::declval<Args>()...) );

}



