#include <iostream>
#include "../flowz/demangle.h"

#include "reactive_expressions.hpp"

void basic_update_mechanism()
{
   PARAMETER( float, s1 );
   PARAMETER( float, s2 );
   PARAMETER( float, d1 );
   PARAMETER( float, d2 );
   PARAMETER( float, d3 );

   auto deps = make_reactive_expressions
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


void correct_update_order()
{
   PARAMETER( std::string, a );
   PARAMETER( std::string, b );
   PARAMETER( std::string, c );
   PARAMETER( std::string, d );

   auto order1 = make_reactive_expressions
   (  a = "a"
   ,  b = a + "b"
   ,  c = b + "c"
   ,  d = c + b
   );

   auto order2 = make_reactive_expressions
   (  a = "a"
   ,  c = b + "c"
   ,  d = c + b
   ,  b = a + "b"
   );

   auto order3 = make_reactive_expressions
   (  d = c + b
   ,  c = b + "c"
   ,  b = a + "b"
   ,  a = "a"
   );

   assert( order1.get(d) == "abcab" );
   assert( order2.get(d) == "abcab" );
   assert( order3.get(d) == "abcab" );
}



void correct_update_order2()
{
   PARAMETER( std::string, a );
   PARAMETER( std::string, b );
   PARAMETER( std::string, c );
   PARAMETER( std::string, d );

   std::string path;
   auto lb = lazy_fun([&](auto)      { path += "b"; return "b"; });
   auto lc = lazy_fun([&](auto)      { path += "c"; return "c"; });
   auto ld = lazy_fun([&](auto,auto) { path += "d"; return "d"; });

   auto order1 = make_reactive_expressions
   (  a = 0
   ,  d = ld(b,c)
   ,  b = lb(a)
   ,  c = lc(b)
   );

   assert( path == "bcd" );
}


int main()
{
   basic_update_mechanism();
   correct_update_order();
   correct_update_order2();
}
