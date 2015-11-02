#include <iostream>
#include <cmath>
#include <boost/proto/proto.hpp>

using namespace boost;

namespace transforms {

    using namespace proto;

    template< typename X >
    struct max_fn : callable {
        using result_type = X;
        X operator()( X const & x , X const & y ) const { return std::max(x,y); }
    };

    template< typename X >
    struct zero_fn : callable {
        using result_type = X;
        X operator()() const { return {}; }
    };

    struct max_fn_int : max_fn<int> {};

    struct max_int : or_
        <   when
            <   terminal< int >
            ,   _value
            >
        ,   when
            <   terminal< _ >
            ,   call< zero_fn<int>() >
            >
        ,   when
            <   subscript< terminal<int> , terminal<int> >
            ,   max_int(_right)
            //,   call< zero_fn<int>() >
            >
        ,   when
            <   binary_expr< _, max_int, max_int >
            ,   call< max_fn<int>( max_int(_left), max_int(_right) ) >
            >
        >
    {};

}

using transforms::max_int;

int main()
{
    auto expr  =  proto::lit(2)[7] + 3 + 4;

    std::cout << max_int{}(expr) << std::endl;
}
