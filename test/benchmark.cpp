//   -----------------------------------------------------------------------------------------------
//    Copyright 2015 André Bergner. Distributed under the Boost Software License, Version 1.0.
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//      --------------------------------------------------------------------------------------------

#include <benchmark/benchmark.h>

#include <flowz/flowz.hpp>





namespace biquad
{
   using namespace flowz;

   const float b0 =  0.2
             , b1 = -0.3
             , b2 =  1.1
             , a1 = -0.2
             , a2 =  0.8
             ;

   auto fwd  = proto::deep_copy(  (b0 * _1  +  b1 * _1[_1]  +  b2 * _1[_2] ) );
   auto bwd  = proto::deep_copy( ~(     _2  +  a1 * _1[_1]  +  a2 * _1[_2] ) );

   namespace direct_form_1
   {
      auto make_flow = []
      {
         return compile( fwd |= bwd );
      };

      auto make_custom = []
      {
         return [ & , x1 = 0.f , x2 = 0.f , y1 = 0.f , y2 = 0.f ](float x0) mutable
         {
            auto 
            y0  =  b0 * x0  +  b1 * x1  +  b2 * x2  +  a1 * y1  +  a2 * y2;
            x2  =  x1;
            x1  =  x0;
            y2  =  y1;
            y1  =  y0;
            return std::make_tuple(y0);
         };
      };

      auto make_custom2 = []
      {
         return [ f1 = make_custom() , f2 = make_custom() ]( float x ) mutable
         {
            return f2(std::get<0>(f1(x)));
         };
      };
   }

   namespace direct_form_2
   {
      auto make_flow = []
      {
         return compile( bwd |= fwd );
      };

      auto make_custom = []
      {
         return [ & , u1 = 0.f , u2 = 0.f ](float x0) mutable
         {
            auto
            u0  =       x0  +  a1 * u1  +  a2 * u2,
            y0  =  b0 * u0  +  b1 * u1  +  b2 * u2;
            u2  =  u1;
            u1  =  u0;
            return std::make_tuple(y0);
         };
      };
   }
}



template < typename T >
inline void escape(T& p) { asm volatile( "" :: "g"(&p) : "memory" ); }


auto sum_dirac = []( auto& proc )
{
   auto sum = std::get<0>(proc(1.f));
   for ( size_t n = 0; n < 100; ++n )
   {
      escape(proc);
      sum += std::get<0>(proc(.0f));
      sum += std::get<0>(proc(.0f));
   }
   return sum;
};



//   -----------------------------------------------------------------------------------------------
//    BENCHMARKS
//     ---------------------------------------------------------------------------------------------



static void biquad_direct_form_1_flowz(benchmark::State& s)
{
   float x;
   auto f = biquad::direct_form_1::make_flow();
   while (s.KeepRunning())
   {
      escape(x);
      x = sum_dirac(f);
   }
}
BENCHMARK(biquad_direct_form_1_flowz);


static void biquad_direct_form_1_custom(benchmark::State& s)
{
   float x;
   auto f = biquad::direct_form_1::make_custom();
   while (s.KeepRunning())
   {
      escape(x);
      x = sum_dirac(f);
   }
}
BENCHMARK(biquad_direct_form_1_custom);


static void biquad_direct_form_2_flowz(benchmark::State& s)
{
   float x;
   auto f = biquad::direct_form_2::make_flow();
   while (s.KeepRunning())
   {
      escape(x);
      x = sum_dirac(f);
   }
}
BENCHMARK(biquad_direct_form_2_flowz);


static void biquad_direct_form_2_custom(benchmark::State& s)
{
   float x;
   auto f = biquad::direct_form_2::make_custom();
   while (s.KeepRunning())
   {
      escape(x);
      x = sum_dirac(f);
   }
}
BENCHMARK(biquad_direct_form_2_custom);






BENCHMARK_MAIN();




