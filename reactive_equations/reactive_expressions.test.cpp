#include <iostream>
#include "../flowz/demangle.h"

#include "reactive_expressions.hpp"


int main()
{
   PARAMETER( float, s1, 12 );
   PARAMETER( float, s2, -3 );
   PARAMETER( float, d1, 0 );
   PARAMETER( float, d2, 0 );
   PARAMETER( float, d3, 0 );

   auto deps = make_mapping_dag
   (  s1   = 12
   ,  s2   = -3
   ,  d1   = 1.f / (s1*s1 + s2*s2)
   ,  d2   = s1 * d1
   ,  d3   = s1 * d2
   );

   auto print_params = [&]{
      std::cout << "----------------------------------" << std::endl;
      std::cout << "s1:  " << deps.get(s1) << std::endl;
      std::cout << "s2:  " << deps.get(s2) << std::endl;
      std::cout << "d1:  " << deps.get(d1) << std::endl;
      std::cout << "d2:  " << deps.get(d2) << std::endl;
      std::cout << "d3:  " << deps.get(d3) << std::endl;
      std::cout << "----------------------------------" << std::endl;
   };

   print_params();

   deps.set(s2, 0);
   print_params();
/*
   std::cout << "set a=10." << std::endl;
   print_params();

   std::cout << "set a=-23, b=1337." << std::endl;
   deps.set(a, -23);
   deps.set(b, 1337);
   print_params();
*/
}

