#include <iostream>
#include <algorithm>
#include <array>
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
        template< typename I , typename J , typename State >
        auto operator()( placeholder<I> const & , placeholder<J> const & , State const & state ) const
        {
            auto const & s = std::get<I::value-1>(state);
            return s[s.size()-J::value];
            //return state[state.size()-J::value];
        }
    };


    struct eval_it : or_
    <   when
        <   delayed_placeholder<>
        ,   place_delay( _value(_left) , _value(_right) , _state )
        >
    ,   when
        <   terminal< placeholder<_> >
        ,   place_the_holder( _value , _state )
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



template< typename F , typename Tuple >
struct lift_into_tuple;

template< typename F , typename... Ts >
struct lift_into_tuple< F , std::tuple<Ts...> >
{
    using type = std::tuple< typename F::template apply_t<Ts>... >;
};

template< typename F , typename Tuple >
using lift_into_tuple_t = typename lift_into_tuple<F,Tuple>::type;



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
void tuple_for_each( F&& f , std::tuple<Ts...>& t , Tuples&&... tuples )
{
    tuple_for_each_impl< sizeof...(Ts) >::apply( std::forward<F>(f), t, std::forward<Tuples>(tuples)... );
}


struct {
    template< typename T , size_t N , typename Y >
    void operator()( std::array<T,N>& xs , Y y )
    {
        for ( size_t n = 1 ; n < xs.size() ; ++n )  xs[n-1] = xs[n];
        xs.back() = y;
    }
} rotate_push_back;



auto compile = []( auto expr )        // TODO need to define value_type for state
{
    using expr_t = decltype(expr);
    using tuple_t = typename delay_for_placeholder< lambda_arity_t<expr_t>::value, expr_t >::type;
    using state_t = lift_into_tuple_t< to_array<float> , tuple_t >;

    return [ expr , state = state_t{} ]( auto&&... xs ) mutable
    {
        auto result = eval_it{}( expr, state );
        tuple_for_each( rotate_push_back, state, std::forward_as_tuple(xs...) );
        //rotate_push_back( state, std::forward<decltype(xs)>(xs)... );
        return result;
    };
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
    auto differntiator = compile( _1[_1] - _1[_2] );

    for ( auto x : {1,1,1,1,1} )
        std::cout << differntiator(x) << std::endl;
//    for ( auto x : {1,2,3,4,5,6,7,8,9} )
//        std::cout << differntiator(x,x) << std::endl;

}
