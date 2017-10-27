#include "reactive_expressions.hpp"

#include <iostream>
#include <cmath>


int main()
{
   {
      auto sin = lazy_fun([](auto x){ return std::sin(x); });

      PARAMETER( float, s1, 12 );
      PARAMETER( float, s2, -3 );
      PARAMETER( float, d1, 0 );
      PARAMETER( float, d2, 0 );
      PARAMETER( float, d3, 0 );

      auto deps = make_mapping_dag
      (  s1  = 12
      ,  s2  = -3
      ,  d1  = 1.f / (1.f + s1*s1)
      ,  d2  = 1.f / (1.f + s2*s2)
      ,  d3  = sin(s1) * d1 / d2
      );

      deps.set(s1, 47);

      std::cout << deps.get(s1) << std::endl;
      std::cout << deps.get(s2) << std::endl;
      std::cout << deps.get(d1) << std::endl;
      std::cout << deps.get(d2) << std::endl;
      std::cout << deps.get(d3) << std::endl;
   }

   std::cout << "--------------------" << std::endl;

   {
      auto my_fun = lazy_fun([](auto x){
         if ( x > 0 )  return std::sqrt(x);
         else          return x*x;
      });

      auto my_fun3 = lazy_fun([](auto x, auto y, auto z){
         if ( x > y )  return std::sqrt(y*y);
         else          return x*y*z;
      });

      PARAMETER( float, a, 0 );
      PARAMETER( float, b, 0 );
      PARAMETER( float, c, 0 );

      auto deps = make_mapping_dag
      (  a = 1337.
      ,  b = -a*a
      ,  c = my_fun3(b, a*b , a*a)
      //,  c = my_fun(b)
      );

      std::cout << deps.get(b) << std::endl;
      std::cout << deps.get(c) << std::endl;

      deps.set(a, -3);

      std::cout << deps.get(b) << std::endl;
      std::cout << deps.get(c) << std::endl;
   }

}
