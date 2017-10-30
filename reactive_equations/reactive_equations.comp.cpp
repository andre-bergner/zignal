#include <iostream>
#include "reactive_expressions.hpp"
#include <cmath>


int main()
{

   class HandCrafted
   {
   public:

      float s1 = 12;
      float s2 = -3;
      float d1;
      float d2;
      float d3;

      HandCrafted()
      {
         update1();
         update2();
      }

      void set_s1(float x)
      {
         s1 = x;
         update1();
      }

      void update1()
      {
         d1  = 1.f / (1.f + s1*s1);
         update2();
      }

      void update2()
      {
         d2  = 1.f / (1.f + s2*s2);
         update3();
      }

      void update3()
      {
         d3  = std::sin(s1) * d1 / d2;
      }
   };

   // Problems:
   //  • hard to refactor / error-prone
   //  • hard to understand 
   //  • not localized reasoning
   //  • a lot of noise

   volatile float new_s1 = 1337.f;

#if 0

   {
      HandCrafted h;
      h.set_s1(new_s1);

      volatile float s1_r = h.s1;
      volatile float s2_r = h.s2;
      volatile float d1_r = h.d1;
      volatile float d2_r = h.d2;
      volatile float d3_r = h.d3;

//      std::cout << d3_r << std::endl;
   }

#else

   auto sin = lazy_fun([](auto x){ return std::sin(x); });

   {
      PARAMETER( float, s1 );
      PARAMETER( float, s2 );
      PARAMETER( float, d1 );
      PARAMETER( float, d2 );
      PARAMETER( float, d3 );

      auto deps = make_mapping_dag
      (  s1  = 12
      ,  s2  = -3
      ,  d1  = 1.f / (1.f + s1*s1)
      ,  d2  = 1.f / (1.f + s2*s2)
      ,  d3  = sin(s1) * d1 / d2
      );


      deps.set(s1, new_s1);

      volatile float s1_r = deps.get(s1);
      volatile float s2_r = deps.get(s2);
      volatile float d1_r = deps.get(d1);
      volatile float d2_r = deps.get(d2);
      volatile float d3_r = deps.get(d3);

//      std::cout << d3_r << std::endl;
   }

#endif

}
