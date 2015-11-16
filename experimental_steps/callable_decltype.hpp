#pragma once


namespace boost { namespace proto
{
   // boost.proto (similar to functional from the STL) requires a result
   // type or trait to be declared within function objects. This helper
   // works around this design limitation of boost.proto by automatically
   // deriving the result type by using decltype on the object itself.

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
}}
