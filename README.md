**V 0.1 alpha preview**

The helper class VariantViewer that help to wrap std::visit to more user friendly usage with std::variant and std::optional<std::variant>
for example function execution may produce nullopt in some cases, or variant with some values, we want to access values by list our handlers to them by
specify types that we want to processing or do what we want with variant like adding extracting substracting it's types on fly, and do our handle processing tree without bothering about override visitor class for specific types
to help access of options via pipe | operator and << shift to no options or >> shift to all options callbacks.

**Example of usage:**

1. I wish to process result from function that can be certain type or no result for example I returning std::optional< std::variant< ... > > or better I have variant< std::monostate, ... TYPES ... >
2. I do processing through listing desired lambda's
```cpp
std::variant<std::monostate, bool, int, std::string> my = std::monostate{}; // we have no options for now

// we want to hanlde 3 states, no options, string, and int
custom_view::variants(my) << [](){ /* optional lambda called when variant is in std::monostate value or std::optional< std::variant< ... > > is std::nullopt */ }
  | [](const int & a) { /* do you job with a when variant holds integer */ }
  | [](const std::string &s) { /* do you job when there will be string };
```

we want to handle any valid not empty type stored in variant
```cpp
custom_view::variants(my) >> [](const auto &do_decuce_yourself) { std::cout << do_deduce_yourself; } // will produce printing for type if supported and contained in variant
```
 we want to handle both empty variant or empty optional with variant, or print any type data
```cpp
std::optional< std::variants< std::string, int, float > > may_be_empty = SOME_CONDITION ? std::nullopt : std::variant< std::string, int, float >{ 3.4f };
// Note that pipe operator for individual type processing should not be mixed with >> also as operator >> is bool and returns true after executing so you can check also if there was something in variant
custom_view::variants(may_be_empty) << [](){ std::cout << "no types in variant" } >> [](const auto &a){ std::cout << "variant contains" << a; }
```
for single processing no or all options you may skip one of operators upper.

// TODO: support for multitypes multivariants static assertion etc

Also there optional extraction of variant, here how I use if extraction not resulting to true (which mean's type is extracted) and then do test does current type is bool held by variant

```cpp
auto dir = vec2f_zero;
        std::optional<vec2f> move_dir = std::nullopt; // then I may distinct where dir comes from mean what is dir classification like
        if (move_dir ^= custom_view::variants(options))
            dir = *move_dir;
        else if ( custom_view::variants(options).contains<bool>() )
            dir = Trackable.get()->Origin.get() - pCamera->Origin.get();
```

