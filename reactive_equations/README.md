# Reactive Expressions

This is an experimental library that allows to creates at compile a
dependency graph from a set of equations involving user defined parameters.
If any of the involved source parameters is changed all dependent parameters
get updated automatically in the right order. This was inspired by QML properties.
The behaviour is also comparable to what happens in Excel.

## Example

```c++
#include "reactive_expressions.hpp"

// ...

// create a lazy version of the sin function:
auto sin = lazy_fun([](auto x){ return std::sin(x); });

PARAMETER( float, s1 );   // source parameter 1
PARAMETER( float, s2 );   // source parameter 2
PARAMETER( float, d1 );
PARAMETER( float, d2 );
PARAMETER( float, d3 );

auto state = make_reactive_expressions
(  s1  = 42
,  s2  = 1337
,  d1  = 1.f / (1.f + s1*s1)
,  d2  = 1.f / (1.f + s2*s2)
,  d3  = sin(s1) * d1 / d2
);

// read values:
cout << state.get(d2) << endl;
cout << state.get(d3) << endl;

// read values:
deps.set(s1, 3.14);

// all depedent parameters will have a new value:
cout << state.get(d2) << endl;
cout << state.get(d3) << endl;
```

