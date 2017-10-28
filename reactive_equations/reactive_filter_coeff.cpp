#include "reactive_expressions.hpp"
#include <iostream>
#include <cmath>


int main()
{
   const float two_pi = 8. * std::atan(1.);

   auto sin = lazy_fun([](auto x){ return std::sin(x); });
   auto cos = lazy_fun([](auto x){ return std::cos(x); });
   auto sqrt = lazy_fun([](auto x){ return std::sqrt(x); });

   auto to_string = lazy_fun([](auto x){ return std::to_string(x); });


   PARAMETER( float, sr, {} );
   PARAMETER( float, freq, {} );
   PARAMETER( float, Q, {} );
   PARAMETER( float, alpha, {} );
   PARAMETER( float, w0, {} );
   PARAMETER( float, cosw0, {} );
   PARAMETER( float, a0, {} );
   PARAMETER( float, a1, {} );
   PARAMETER( float, a2, {} );
   PARAMETER( float, b0, {} );
   PARAMETER( float, b1, {} );
   PARAMETER( float, b2, {} );
   PARAMETER( std::string, label_a0, {} );
   PARAMETER( std::string, label_a1, {} );
   PARAMETER( std::string, label_a2, {} );
   PARAMETER( std::string, label_b0, {} );
   PARAMETER( std::string, label_b1, {} );
   PARAMETER( std::string, label_b2, {} );
   PARAMETER( std::string, label_prefix, {} );

   //auto state = rex::make_reactive_expressions
   auto state = make_mapping_dag
   (  sr    = 44100.f
   ,  freq  = 440.
   ,  Q     = 1. / std::sqrt(2.)
   ,  w0    = two_pi * freq / sr
   ,  cosw0 = cos(w0)
   ,  alpha = sin(w0) / (2. * Q)
   ,  b0    =  (1. - cosw0) / 2.
   ,  b1    =   1. - cosw0
   ,  b2    =  (1. - cosw0) / 2.
   ,  a0    =   1. + alpha
   ,  a1    =  -2. * cosw0
   ,  a2    =   1. - alpha
   ,  label_prefix = "value of "
   ,  label_a0 = label_prefix + "a0 = " + to_string(a0)
   ,  label_a1 = label_prefix + "a1 = " + to_string(a1)
   ,  label_a2 = label_prefix + "a2 = " + to_string(a2)
   ,  label_b0 = label_prefix + "b0 = " + to_string(b0)
   ,  label_b1 = label_prefix + "b1 = " + to_string(b1)
   ,  label_b2 = label_prefix + "b2 = " + to_string(b2)
   );

   auto print_labels = [&]{
      std::cout << "----------------------------" << std::endl;
      std::cout << state.get(label_a0) << std::endl;
      std::cout << state.get(label_a1) << std::endl;
      std::cout << state.get(label_a2) << std::endl;
      std::cout << state.get(label_b0) << std::endl;
      std::cout << state.get(label_b1) << std::endl;
      std::cout << state.get(label_b2) << std::endl;
   };

   print_labels();

   state.set(Q, 10.);
   print_labels();

   state.set(label_prefix, "la valuer de ");
   print_labels();
}
