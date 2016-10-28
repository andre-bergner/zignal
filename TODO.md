# todo

* demos:
  * moog filter --> needs tanh
  * hilbert transformer
  * multi mode state variable filter

* use variable templates for compile time indexing
  _1<-2>   input argument one delayed by 2

* unary-to-binary-feedback
  - bug ~(_1[_1]+_2 |= _1) compiles, but does not run
  - bug in predicate when feedback path is thinning ?, e.g.  _1 + _2 |= _1[_1] |= _1,_1
  - handle nested feedback: idea transform on the way up from the leafs instead of on the way down
    current problem:
      ~~( _1 + _2 |= _1[_1] )
    more generic:
      ~( f(_1) ~( _1 + _2 |= _1[_1] ))
       ___________________________
      |        _________________  |
      |  ___  |  ___     ___    | |
      |_| f | ·-| + |___|mem|___|_|___
        |___|---|___|   |___|

    needs new private node: feeback_input_sequence ?

    structure is essentially:    ~( (_1,f(_1))  |= (_1 + _2)   |=  (_1[_1]) )
    translated to bin feedback:   ( (_1,f(_1))  |= (_1 + _2) )  ≈  (_1[_1])

  - handle parallel combiners

* canonicalization
  - unary to binary feedback expressions
  - handle doublicated states/expressions that can be shared, for instance
    ~( _1[_2] + _2 ) |= _1[_2]   -->  ( _1 + _2 ) ≈ _1[_2] |= _1 ???

* computing partial derivatives along the flow graph (a.k.a back-propagation)
  * http://colah.github.io/posts/2015-08-Backprop/
  * useful to implement neural networks within flowz

* migrate to nonius from google benchmark
* adapt tuple tricks from hana or switch to hana
* use more constexpr
* file proto::lit bug (you past me idiot didn't write down what the bug is!!!)
* benchmarks: unroll1,2,3 , long filter-chain
  * document clang vs gcc

* application: sorting network.

* analyze memory transfer, play with loop unrolling by 3, ...

* tb.discussed: allow expr[_n], e.g. (_1+_2)[_1]  which is equivalent to _1+_2 |= _1[_1]
                could be handled by transformer in compile that puts everything into a canonical form

* add demos (different wiring structures, standard filters, direct forms, etc.)
* minimize state size
  * tuple<tuple<no_state>> is not zero size
    --> map to no_state
    --> tuple_transform(no_state) = {}
  * use same state if sequence follows feedback, e.g. in ~_1[_1] |= _1[_1]
* remove expression as state member (too big)
  --> create eager expressions
  --> copy all values into tuple-hierachy
  --> treat values similar to state, i.e. pass them in from outside
      and hold expression as mere type
* bug, due to front_state_expr expressions w/o input cannot be compiled.
* split up into files
* remove currying
* expressions generating one output argument should return a plain value not a tuple<T>
* simple dft and transfer function expressions for testing
* biquad.h into flowz/
* state type based on example input
* parameter (referencing type)
* define proper grammar
* tuprix type in flowz


* look into:
    * https://en.wikipedia.org/wiki/Hardware_description_language
    * http://visualhdl.sysprogs.org/tutorial/04-advanced-templates.php
    * other work: Kronos a vectorizing compiler for music
      https://www.researchgate.net/publication/228369284_Kronos-a_vectorizing_compiler_for_music_dsp
