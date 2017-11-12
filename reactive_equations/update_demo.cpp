#include "reactive_expressions.hpp"

#include <iostream>
#include <cmath>


int main()
{
   using namespace std;

   PARAMETER( float, s1 );
   PARAMETER( float, s2 );
   PARAMETER( float, i1 );
   PARAMETER( float, o1 );

   auto deps = make_reactive_expressions
   (  s1  = 12
   ,  o1  = lazy_fun([](auto x, auto y, auto z){ cout << "updateing o1.\n"; return x*y+z; })(s1, s2, i1)
   ,  i1  = lazy_fun([](auto x){ cout << "updateing i1.\n"; return x+x; })(s1)
   ,  s2  = -3
   );

   cout << "------------" << endl;
   deps.set(s1, 47);

   cout << "------------" << endl;
   cout << deps.get(s1) << endl;
   cout << deps.get(s2) << endl;
   cout << deps.get(i1) << endl;
   cout << deps.get(o1) << endl;
}
