# todo

* unary-to-binary-feedback
  - handle full delay in first expression, i.e. before first seq-op --> could use make_front
  - handle nested feedback: idea transform on the way up from the leafs instead of on the way down
  - handle parallel combiners
* canonicalization
  - unary to binary feedback expressions
  - handle doublicated states/expressions that can be shared, for instance
    ~( _1[_2] + _2 ) |= _1[_2]   -->  ( _1 + _2 ) ~~ _1[_2] |= _1 ???
* file proto::lit bug
* benchmarks: unroll1,2,3 , long filter-chain
  * document clang vs gcc

* analyze memory transfer, play with loop unrolling by 3, ...

* tb.discussed: allow expr[_n], e.g. (_1+_2)[_1]  which is equivalent to _1+_2 |= _1[_1]
                could be handled by transformer in compile that puts everything into a canonical form

* ~( _1 |= _1[_1] )   must compile !  pull model?
  more complicated:  ~( _1 | _1[_1]   |=   _1[_1] | _1 )
  ideas:
  - push vs pull:
    - pull-transform: builds up callback-tree, callbacks that return the values
    - problem: results are lazy, values get computed several times
  - static_promise (instead of bottom_type):
    - build up new expression that gets called when input is ready
      --> input type is place-holder again (or another version?),
          i.e. place_the_holder returns placeholder, that builds up new expression.
          expression build up must stop when delay gets accessed
    - pass along,
  - alternative two calls
    1.) with bottom_type, bottom_type operations are no-ops
    2.) with result of first call
    problems:
      - no-op call might be to late, e.g. (_1[_1]+_2[_2]) + bottom_type -> does left side gets optimised away?
      - when/how does second evaluation stop? Should stop at part that evaluated succ. in the first call

  - tag (no input, second_run == no ouput, normal ) dispatch feedback, etc. 

  - both prev. options together?
    1.) bottom_type to get value 
    2.) expression builder to get expression
    3.) call built expression from 2 with value from 1

    when does build up stop? -> answer: when eval of expr with bottom_type would compile -> sfinae!

    _1 + _1 |= _1[_1] + _1 |= _1[_1]


   rebuild : or
   - placeholder -> value<child>
   - sequence -> return r
   - otherwise -> default<rebuild>


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
