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
   (  s1  = 12
   ,  s2  = -3
   ,  d1  = 1.f / (s1*s1 + s2*s2)
   ,  d2  = s1 * d1
   ,  d3  = s1 * d2
   );

   assert( deps.get(s1) == 12.f );
   assert( deps.get(s2) == -3.f );
   assert( deps.get(d1) == 1.f / 153.f );
   assert( deps.get(d2) == (1.f / 153.f) * 12.f );
   assert( deps.get(d3) == (1.f / 153.f) * 12.f * 12.f );

   deps.set(s2, 0);
   assert( deps.get(s1) == 12.f );
   assert( deps.get(s2) == 0.f );
   assert( deps.get(d1) == 1.f / 144.f );
   assert( deps.get(d2) == (1.f / 144.f) * 12.f );
   assert( deps.get(d3) == 1.f );
}
