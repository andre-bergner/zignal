#include <iostream>
#include <algorithm>
#include <array>
#include <vector>
#include <boost/mpl/int.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/next_prior.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/typeof/std/ostream.hpp>
#include <boost/typeof/std/iostream.hpp>
#include <boost/proto/core.hpp>
#include <boost/proto/context.hpp>
#include <boost/proto/transform.hpp>

#include <cxxabi.h>

std::string demangle(const char* name) {

    int status = -4;

    char* res = abi::__cxa_demangle(name, NULL, NULL, &status);

    const char* const demangled_name = (status==0)?res:name;

    std::string ret_val(demangled_name);

    free(res);

    return ret_val;
}

//  TODO 
//  √ use callable transform instead of context
//  • implement currying
//  √ operator() -> enable if # of args


namespace mpl = boost::mpl;
namespace proto = boost::proto;



// Forward declaration of the lambda expression wrapper

template<typename T>
struct lambda;

struct lambda_domain
  : proto::domain<proto::pod_generator<lambda> >
{};





namespace boost { namespace proto
{
    // boost.proto (similar to functional from the STL) requires a result
    // type or trait to be declared within function objects. This helper
    // works around this design limitation of boost.proto by automatically
    // deriving the result type by using decltype on the object itself.

    struct callable_decltype : callable
    {
        template< typename Signature >
        struct result;

        template< typename This , typename... Args >
        struct result< This( Args... ) > 
        {
            using type = decltype( std::declval<This>()( std::declval<Args>()... ));
        };
    };
}}


//  ------------------------------------------------------------------------------------------------
// main part
//  ------------------------------------------------------------------------------------------------


template< typename I >  struct placeholder       { using arity = I; };
template< typename T >  struct placeholder_arity { using type = typename T::arity; };



BOOST_PROTO_DEFINE_ENV_VAR( current_input_t, current_input );
BOOST_PROTO_DEFINE_ENV_VAR( delayed_input_t, delayed_input );


namespace transforms
{
    using namespace proto;

    template< typename idx = _ >
    using delayed_placeholder = subscript< terminal<placeholder<idx>> , terminal<placeholder<_>> >;


    struct lambda_arity : or_
    <   when
        <   delayed_placeholder<>
        ,   placeholder_arity<_value(_left)>()
        >
    ,   when
        <   terminal< placeholder<_> >
        ,   placeholder_arity<_value>()
        >
    ,   when
        <   terminal<_>
        ,   mpl::int_<0>()
        >
    ,   when
        <   nary_expr<_, vararg<_>>
        ,   fold<_, mpl::int_<0>(), mpl::max<lambda_arity, _state>()>
        >
    >
    {};


    template< typename idx >
    struct static_max_delay_impl   // excapsulates template parameter, proto gets confused elsewise
    {
        struct apply : or_
        <   when
            <   delayed_placeholder<idx>
            ,   placeholder_arity< _value(_right) >()
            >
        ,   when
            <   terminal<_>
            ,   mpl::int_<0>()
            >
        ,   when
            <   nary_expr<_, vararg<_>>
            ,   fold<_, mpl::int_<0>(), mpl::max<apply, _state>()>
            >
        >
        {};
    };

    template< typename idx = _ >
    using static_max_delay = typename static_max_delay_impl<idx>::apply;


    struct place_the_holder : callable_decltype
    {
        template< typename I , typename Tuple >
        auto operator()( placeholder<I> const & , Tuple const & args ) const
        {
            return std::get<I::value-1>( args );
        }
    };


    struct place_delay : callable_decltype
    {
        template< typename I , typename J , typename Delayed_input >
        auto operator()( placeholder<I> const & , placeholder<J> const & , Delayed_input const & del_in ) const
        {
            auto const & s = std::get<I::value-1>(del_in);
            return s[s.size()-J::value];
        }
    };


    struct eval_it : or_
    <   when
        <   delayed_placeholder<>
        ,   place_delay( _value(_left) , _value(_right) , _env_var<delayed_input_t> )
        >
    ,   when
        <   terminal< placeholder<_> >
        ,   place_the_holder( _value , _env_var<current_input_t> )
        >
    ,   when
        <   _
        ,   _default< eval_it >
        >
    >
    {};

}

using  transforms :: lambda_arity;
using  transforms :: static_max_delay;
using  transforms :: eval_it;

template< typename idx , typename Expr >
using static_max_delay_t = typename boost::result_of<static_max_delay<idx>(Expr)>::type;

template< typename Expr >
using lambda_arity_t = typename boost::result_of<lambda_arity(Expr)>::type;




template< size_t arity, typename Expr >
struct delay_for_placeholder
{
    using type = decltype( std::tuple_cat( std::declval<typename delay_for_placeholder<arity-1,Expr>::type>()
                                         , std::declval<std::tuple< static_max_delay_t<mpl::int_<arity>,Expr>>>() ));
};

template< typename Expr >
struct delay_for_placeholder<0,Expr>
{
    using type = std::tuple<>;
};




//  ------------------------------------------------------------------------------------------------
// supporting tools
//  ------------------------------------------------------------------------------------------------



//  ------------------------------------------------------------------------------------------------
// lift_into_tuple  --  lifts a meta-function into a tuple,
//                      i.e. applies it on each type in tuple, returns tuple of new types

template< typename F , typename Tuple >
struct lift_into_tuple;

template< typename F , typename... Ts >
struct lift_into_tuple< F , std::tuple<Ts...> >
{
    using type = std::tuple< typename F::template apply_t<Ts>... >;
};

template< typename F , typename Tuple >
using lift_into_tuple_t = typename lift_into_tuple<F,Tuple>::type;



//  ------------------------------------------------------------------------------------------------
// to_array  --  meta-function that maps mpl::int_<N> to array<T,N>

template< typename T >
struct to_array
{
    template< typename Int >
    struct apply
    {
        using type = std::array< T , Int::value >;
    };

    template< typename Int >
    using apply_t = typename apply<Int>::type;
};



//  ------------------------------------------------------------------------------------------------
// tuple_for_each  --  apply n-ary function on each value in list of n tuples of same size

template< size_t N , size_t n = 0 >
struct tuple_for_each_impl
{
    template< typename F, typename... Tuples >
    static void apply( F&& f, Tuples&&... tuples )
    {
        f( std::get<n>(tuples)... );
        tuple_for_each_impl<N,n+1>::apply( std::forward<F>(f), std::forward<Tuples>(tuples)... );
    }
};

template< size_t N >
struct tuple_for_each_impl<N,N>
{
    template< typename F, typename... Tuples >
    static void apply( F&& , Tuples&&... ) { }
};


template< typename F, typename... Ts , typename... Tuples >
// requires (sizeof...(Ts) == tuple_size(Tuples))...
void tuple_for_each( F&& f , std::tuple<Ts...>& t , Tuples&&... tuples )
{
    tuple_for_each_impl< sizeof...(Ts) >::apply( std::forward<F>(f), t, std::forward<Tuples>(tuples)... );
}


//  ------------------------------------------------------------------------------------------------
// very simple delay_line operation on std::array --> TODO should be own type static_delay_line !

struct {
    template< typename T , size_t N , typename Y >
    void operator()( std::array<T,N>& xs , Y y )
    {
        for ( size_t n = 1 ; n < xs.size() ; ++n )  xs[n-1] = xs[n];
        xs.back() = y;
    }
} rotate_push_back;




//  ------------------------------------------------------------------------------------------------
// compile  --  main function of framework
//              • takes a zignal/dsp expressions
//              • generta
//              • returns clojure
//  ------------------------------------------------------------------------------------------------

auto compile = []( auto expr )        // TODO need to define value_type for state
{
    using expr_t = decltype(expr);
    using tuple_t = typename delay_for_placeholder< lambda_arity_t<expr_t>::value, expr_t >::type;
    using state_t = lift_into_tuple_t< to_array<float> , tuple_t >;

    return [ expr , state = state_t{} ]( auto const&... xs ) mutable
    {
        auto result = eval_it{}( expr, proto::_state{}, ( current_input = std::make_tuple(xs...) , delayed_input = state ) );
        tuple_for_each( rotate_push_back, state, std::make_tuple(xs...) );
        return result;
    };
};



//  ------------------------------------------------------------------------------------------------
// compile2 -- same as compile, but return type is curryable
//  ------------------------------------------------------------------------------------------------

template
<   typename Expression
,   typename State
,   size_t   arity
>
struct stateful_lambda
{
private:

    Expression  expr_;
    State       state_;

    template< typename... Args >
    auto call_impl( mpl::int_<0> , Args const &... args ) -> decltype(auto)
    {
        auto result = eval_it{}( expr_, proto::_state{}, ( current_input = std::make_tuple(args...) , delayed_input = state_ ) );
        tuple_for_each( rotate_push_back, state_, std::make_tuple(args...) );
        return result;
    }

    template< int arg_diff , typename... Args >
    auto call_impl( mpl::int_<arg_diff> , Args const &... args ) const
    {
        return [ &args...                 // TODO should copy args into lambda
               , expr = *this ]           // TODO should not be a lambda, but a type that is
        ( auto const &... missing_args )  //      aware of the # of args (enable_if them)
        {
            return expr( args..., missing_args... );
        };
    }

public:

    stateful_lambda( Expression expr ) : expr_( std::move(expr)) {}

    template< typename... Args , typename = std::enable_if_t< sizeof...(Args) <= arity > >
    auto operator()( Args const &... args ) -> decltype(auto)
    {
        return call_impl( mpl::int_< arity - sizeof...(Args) >{} , args... );
    }
};


auto compile2 = []( auto expr )        // TODO need to define value_type for state
{
    using expr_t = decltype(expr);
    using arity_t = lambda_arity_t<expr_t>;
    using tuple_t = typename delay_for_placeholder< arity_t::value, expr_t >::type;
    using state_t = lift_into_tuple_t< to_array<float> , tuple_t >;

    return stateful_lambda< expr_t, state_t, arity_t::value>{ expr };
};










const proto::terminal< placeholder< mpl::int_<1> >>::type   _1  = {{}};
const proto::terminal< placeholder< mpl::int_<2> >>::type   _2  = {{}};
const proto::terminal< placeholder< mpl::int_<3> >>::type   _3  = {{}};
const proto::terminal< placeholder< mpl::int_<4> >>::type   _4  = {{}};
const proto::terminal< placeholder< mpl::int_<5> >>::type   _5  = {{}};



int main()
{
    std::cout << "arities -----------------" << std::endl;
    std::cout << lambda_arity{}( _1 + _2 ) << std::endl;
    std::cout << lambda_arity{}( _1[_4] + _3 ) << std::endl;
    std::cout << lambda_arity{}( _1 + _2[_1] ) << std::endl;
    std::cout << "delays -----------------" << std::endl;
    std::cout << static_max_delay<mpl::int_<1>>{}( _1 + _2 ) << std::endl;
    std::cout << static_max_delay<mpl::int_<1>>{}( _1[_4] + _3 ) << std::endl;
    std::cout << static_max_delay<mpl::int_<1>>{}( _1 + _2[_1] ) << std::endl;
    std::cout << "-------------------------" << std::endl;

    auto print_expr_state = []( auto expr )
    {
        using expr_t = decltype(expr);
        using state_t = typename delay_for_placeholder< lambda_arity_t<expr_t>::value, expr_t >::type;
        std::cout << lambda_arity_t<expr_t>::value << "  " << demangle(typeid(state_t).name()) << std::endl;
    };

    print_expr_state( _1 + _2 );
    print_expr_state( _1[_4] + _3 );
    print_expr_state( _1 + _2[_1] + _4[_3] );

    std::cout << "-------------------------" << std::endl;


    auto unit_delay = compile( _1[_1] );
    auto differentiator = compile2( _1 - _1[_1] );

    for ( auto x : {1,1,1,1,1} )
        std::cout << differentiator(x) << std::endl;

/*
    {  // test to check assembly output
        auto test_expr = compile( _1[_1] - _1[_2]*2.f );

        std::vector<float> xs(1000);
        {
            float u = 0.87;
            for ( auto& x : xs ) x = u *= 4.0*(1.-u);
        }

        auto acc = 0.f;
        __asm__ __volatile__("nop":::"memory");
        for ( auto x : xs )
            acc += test_expr(x);
        __asm__ __volatile__("nop":::"memory");
        std::cout << acc << std::endl;

        for ( auto x : xs )
            std::cout << test_expr(x) << std::endl;
    }
*/
}
