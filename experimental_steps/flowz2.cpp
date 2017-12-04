#include <tuple>
#include <utility>
#include <iostream>


template <int N_in, int N_out>
struct BoxConcept
{
   static constexpr int num_ins = N_in;
   static constexpr int num_outs = N_out;

   template <typename... Ins, typename... Outs>
   void operator()( std::tuple<Ins const&...> ins, std::tuple<Outs&...> outs )
   {
      static_assert( num_ins == sizeof...(Ins) );
      static_assert( num_outs == sizeof...(Outs) );
      //...
   }
};


template <typename Box>
struct input_cardinality
{
   static constexpr int value = Box::num_ins;
   static_assert( value >= 0, "Broken box: number of inputs must be non-negative" );
};

template <typename Box>
constexpr auto input_cardinality_v = input_cardinality<Box>::value;



template <typename Box>
struct output_cardinality
{
   static constexpr int value = Box::num_outs;
   static_assert( value >= 0, "Broken box: number of outputs must be non-negative" );
};

template <typename Box>
constexpr auto output_cardinality_v = output_cardinality<Box>::value;





template <int... Ns>
constexpr int sum = (0 + ... + Ns);


template <typename... Boxes>
struct Parallel
{
   static constexpr int num_ins = sum<Boxes::num_ins...>;
   static constexpr int num_outs = sum<Boxes::num_outs...>;
};


// Alternative approach:
//  • num_ins == num_outs !!!
//  • when assembling convinience method adds identity in parallel
//  • Box<2,1> >> Box<2,2>    --->     (Box<2,1> | ID<1>) >> Box<2,2>

template <typename Box1, typename Box2>
struct Sequence
{
   static constexpr int num_ins = Box1::num_ins + std::max(0, Box2::num_ins - Box1::num_outs);
   static constexpr int num_outs = Box2::num_outs + std::max(0, Box1::num_outs - Box2::num_ins);
};


template <typename Box>
struct Feedback
{
   static constexpr int num_ins = std::max(0, Box::num_ins - Box::num_outs);
   static constexpr int num_outs = Box::num_outs;
};






namespace atom {


   // TODO
   // input and output needs to be plced into context
   // → need some form of context tuple
   // → operations need to 'report' their input & output types

   /*

   template <size_t I, typename Context>
   decltype(auto) get_source(Context const& c);
   */

   template <typename Context, typename Input, typename Output>
   struct NestedContext
   {
      Context c;
      Input   i;
      Output  o;
   };

   template <typename Context, typename... Ts>
   decltype(auto) add_input(Context const& c, std::tuple<Ts...>& in)
   {
      return 
   }

   /*
   template <typename Context>
   decltype(auto) get_input(Context const& c)
   {

   }

   */



   template <size_t Index>
   struct Source
   {
      static constexpr size_t index = Index;
      static constexpr int num_outs = 1;
   };

   template <size_t I, typename Context>
   decltype(auto) eval( Source<I> const& s, Context const& c )
   {
      return std::get<I>(c);
   }




   template <int N_channels>
   struct Identity
   {
      static constexpr int num_ins  = N_channels;
      static constexpr int num_outs = N_channels;
   };

   template <int N, typename Context>
   decltype(auto) eval( Identity<N> const& id, Context const& c )
   {
      return  get_input(c);
   }






   template <typename... Boxes>
   struct Parallel
   {
      static constexpr int num_ins  = sum< input_cardinality_v<Boxes>... >;
      static constexpr int num_outs = sum< output_cardinality_v<Boxes>... >;
   };



   template <typename... Boxes>
   inline constexpr bool boxes_match = true;

   template <typename Box>
   inline constexpr bool boxes_match<Box> = true;

   template <typename Box1, typename Box2, typename... Boxes>
   inline constexpr bool boxes_match<Box1, Box2, Boxes...> =
      (output_cardinality_v<Box1> == input_cardinality_v<Box2>) && boxes_match<Box2, Boxes...>;


   template <typename... Ts>
   struct last;

   template <typename... Ts>
   using last_t = typename last<Ts...>::type;

   template <typename T>
   struct last<T> { using type = T; };

   template <typename T, typename... Ts>
   struct last<T,Ts...> { using type = typename last<Ts...>::type; };


   //template <typename Box1, typename Box2>
   template <typename Box, typename... Boxes>
   struct Sequence
   {
      static_assert( boxes_match<Box, Boxes...> );
      static constexpr int num_ins  = input_cardinality_v<Box>;
      static constexpr int num_outs = output_cardinality_v<last_t<Boxes...>>;
   };

   template <typename Box>
   struct Sequence<Box>
   {
      static constexpr int num_ins  = input_cardinality_v<Box>;
      static constexpr int num_outs = output_cardinality_v<Box>;
   };


   template <typename Box1, typename Box2, typename Context>
   decltype(auto) eval( Sequence<Box1,Box2> const& b, Context const& c )
   {
      //return eval(Box2{}, add_input(c, eval(Box1{}, c)) );
      return eval(Box2{}, eval(Box1{}, c) );
   }



   /*
   template <typename Box1, typename Box2>
   struct Sequence
   {
      static_assert( output_cardinality_v<Box1> == input_cardinality_v<Box2> );
      static constexpr int num_ins  = input_cardinality_v<Box1>;
      static constexpr int num_outs = output_cardinality_v<Box2>;
   };
   */


}


//(A,B)


// Seq A B  := A >> B
// (A >> B) >> C == A >> (B >> C)
//
// TODO canonical form:   (A >> B) >> C    :   Seq<Seq<A,B>,C>




int main()
{
   //Parallel<BoxConcept, BoxConcept, BoxConcept> b;
   Sequence<BoxConcept<2,3>, BoxConcept<2,2>> b;
   //Feedback<BoxConcept<2,1>> b;
   std::cout << b.num_ins << std::endl;
   std::cout << b.num_outs << std::endl;

   {
      atom::Sequence<BoxConcept<2,2>, BoxConcept<2,3>> b;
      std::cout << b.num_ins << std::endl;
      std::cout << b.num_outs << std::endl;
   }

   /*
   {
      std::tuple<int> context = { 1377 };
      atom::Source<0>  value;
      std::cout << eval(value, context) << std::endl;


      atom::Sequence<atom::Source<0>, atom::Identity<1>> s2i;
      std::cout << eval(s2i, context) << std::endl;
   }*/

}