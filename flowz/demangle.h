#pragma once

#include <iostream>
#include <cstdlib>
#include <cxxabi.h>
#include <boost/algorithm/string/erase.hpp>

inline std::string demangle(const char* name)
{

    int status = -4;

    char* res = abi::__cxa_demangle(name, NULL, NULL, &status);

    const char* const demangled_name = (status==0)?res:name;

    std::string ret_val(demangled_name);

    free(res);

    return ret_val;
}


template< typename T >
inline auto type_name( T t )  { return demangle(typeid(T).name()); }

template< typename T >
inline auto type_name()       { return demangle(typeid(T).name()); }


template< typename S >
void print_state( S const & s)
{
   auto s_name = type_name(s);
   boost::erase_all( s_name, "std::__1::" );
   boost::erase_all( s_name, "mpl_::" );
   std::cout << s_name << std::endl;
}
