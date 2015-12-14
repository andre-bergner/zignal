#pragma once

#include <cxxabi.h>

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
inline auto type_name( T t = T{} )  { return demangle(typeid(T).name()); }