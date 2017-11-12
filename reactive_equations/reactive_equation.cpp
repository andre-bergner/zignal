#include <iostream>
#include "../flowz/demangle.h"

#include "reactive_expressions.hpp"


int main()
{
   PARAMETER( float, a );
   PARAMETER( float, b );
   PARAMETER( float, c );
   PARAMETER( float, d );
   PARAMETER( float, a_1 );
   PARAMETER( std::string, name );

   std::cout << "created params." << std::endl;
   std::cout << "----------------------------------" << std::endl;
   std::cout << get_value(a) << std::endl;
   std::cout << get_value(b) << std::endl;
   std::cout << get_value(a_1) << std::endl;
   std::cout << get_value(c) << std::endl;
   std::cout << get_value(d) << std::endl;
   std::cout << get_value(name) << std::endl;
   std::cout << "----------------------------------" << std::endl;


   std::cout << "created dependency DAG." << std::endl;

   auto deps = make_reactive_expressions
   (  a    = 1337
   ,  b    = 47
   ,  c    = (b + 1.f) * a_1 + a
   ,  a_1  = 1.f / a
   ,  d    = b*b
   ,  name = name + " world: " + std::to_string(get_value(a))     // recursive --> should be error
   );

   auto print_params = [&]{
      std::cout << "----------------------------------" << std::endl;
      std::cout << "a:   " << deps.get(a) << std::endl;
      std::cout << "b:   " << deps.get(b) << std::endl;
      std::cout << "a_1: " << deps.get(a_1) << std::endl;
      std::cout << "c:   " << deps.get(c) << std::endl;
      std::cout << "d:   " << deps.get(d) << std::endl;
      std::cout << "name:" << deps.get(name) << std::endl;
      std::cout << "----------------------------------" << std::endl;
   };

   print_params();

   std::cout << "set a=10." << std::endl;
   deps.set(a, 10);
   print_params();

   std::cout << "set a=-23, b=1337." << std::endl;
   deps.set(a, -23);
   deps.set(b, 1337);
   print_params();



   std::cout << "----------------------------------" << std::endl;

   //using deep_dependees_t = depth_first_search_t<decltype(deps)::map_t, building_blocks::parameter<a_t, float>>;
   //std::cout << type_name<deep_dependees_t>() << std::endl;
   //std::cout << type_name<meta::type_at_t<decltype(deps)::map_t, building_blocks::parameter<a_t, float>>>() << std::endl;
   //std::cout << type_name<decltype(deps)::map_t>() << std::endl;
}

