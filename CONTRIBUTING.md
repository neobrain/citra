# Contributing
This guide is for developers who wish to contribute to the Dolphin codebase. It will detail how to properly style and format code to fit this project. This guide also offers suggestions on specific functions and other varia that may be used in code.

Citra is a brand new project, so we have a great opportunity to keep things clean and well organized early on. As such, coding style is very important when making commits. Following this guide and formatting your code as detailed will likely get your pull request merged much faster than if you don't (assuming the written code has no mistakes in itself). We understand that under certain circumstances some of them can be counterproductive, but still try to follow as many of them as possible:

### General Rules
* A lot of code was taken from other projects (e.g. Dolphin, PPSSPP, Gekko, SkyEye). In general, when editing other people's code, follow the style of the module you're in (or better yet, fix the style if it drastically differs from our guide).
* Line width is typically 100 characters, unless it hinders readability a lot.
* Don't introduce new external dependencies (libraries, etc) without prior discussion (don't say we didn't warn you when patches get rejected over this).
* Don't use any platform specific code in Core
* Use namespaces often
* Usage of C++11/14 features is OK and recommended, albeit you have to make sure to use the subset of features supported by our target compilers.

### Naming Rules
* Class, Structs, enums and functions:
 * names in CamelCase (`class SomeClassName`)
 * if the name contains an abbreviation, uppercase it (`enum IPCCommandType`)
 * "_" may occasionally be used for clarity (`ARM_InitCore`)
* Variables
 * names in lower case (`int variable`)
 * individual words separated by underscore underscored
 * use a `g_` prefix for global variables (`Config g_Config;`)
 * use descriptive names even for simple, temporary variables (NOT `int i;`, but instead `int counter;`)
* Compile time constants: TODO
* Preprocessor stuff: TODO
* Don't use [Hungarian notation](http://en.wikipedia.org/wiki/Hungarian_notation) prefixes. In fact, don't use any sort of variable prefix at all. The only exception to this rule is the `g_` prefix for global variables.

### General formatting rules: Indentation, spacing, layout
Follow the indentation/whitespace style shown below. Do not use tabs, use 4-spaces instead.

```cpp
// License header goes here, e.g.
// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

namespace Example {

// Namespace contents are not indented
 
// Put a space after comment slashes

// Declare globals at the top
int g_foo = 0;
char* g_some_pointer; // Notice the position of the *
 
enum SomeEnum {
    COLOR_RED,
    COLOR_GREEN,
    COLOR_BLUE
};
 
struct Position {
    int x, y;
};

template<class T>
void FooBar() {
    int some_array[] = {
        5,
        25,
        7,
        42
    };
 
    if (note == the_space_after_the_if) {
        CallAfunction();
    } else {
        // Use a space after the // when commenting
    }
 
    // Comment directly above code when possible
    if (some_condition) single_statement();
 
    // Place a single space after the for loop semicolons
    for (int i = 0; i != 25; ++i) {
        // This is how we write loops
    }
 
    DoStuff(this, function, call, takes, up, multiple,
            lines, like, this);
 
    if (this || condition_takes_up_multiple &&
        lines && like && this || everything ||
        alright || then) {
    }
 
    switch (var) {
    // No indentation for case label
    case 1: // Braces are put on the next line for cases
    {
        int case_var = var + 3;
        DoSomething(case_var);
        break;
    }
    
    // Separate cases with one blank line
    
    case 3:
        DoSomething(var);
        return;

    case 4:
        return;
 
    default:
        // Also break for the last case
        break;
    }
 
    std::vector<int> you_can_declare,
                     a_few,
                     variables,
                     like_this;
}
 
} // namespace (put this comment at the end of namespaces!)
```

### Conditionals
* Put spaces between `if` and `(`, but don't put any extra spaces within the condition (`if (some_value == 4)`)
* Do not leave `else` or `else if` conditions dangling unless the `if` condition lacks braces.
  * Do:

    ```c++
    if (condition)
    {
        // code
    }
    else
    {
        // code
    }
    ```
  * Acceptable:

    ```c++
    if (condition)
        // code line
    else
        // code line
    ```
  * Don't:

    ```c++
    if (condition)
    {
        // code
    }
    else
        // code line
    ```

### Loops
* If an infinite loop is required, use `while (true)` instead of `for (;;)`.
* Empty-bodied loops should use braces after their header, not a semicolon.
  * Do: `while (condition) {}`
  * Don't: `while (condition);`
* When looping over the elements of a standard library container, use iterators instead of plain counters. Use `std::distance(iterator, container.begin())` to get a loop counter if you really need it.
  * do:
  * for (auto it = container.begin() + 5; it != container.end(); ++it)
  * 
* If a [range-based for loop](http://en.cppreference.com/w/cpp/language/range-for) can be used instead of container iterators, use it. Make sure to use the proper loop variable type:
   * `for (const auto& element : container)` will invoke no copy constructions and will prevent erroneous modification of `element`
   * `for (auto& element : container)` will invoke no copy constructions, but modifying `element` will obviously change `container` too (which you might intend or not)
   * `for (auto element : container)` will invoke a copy construction for each element, but will allow you to modify `element` without changing `container` (which you might intend or not)
   * When used in range-based for loops, `const auto&` should be preferred over `auto&`, which should be preferred over `auto` (although obviously you shouldn't use the "preferred" ones if they behave different than what you intend to do!)

### Classes and Structs
* Use `struct` when defining "simple" types (e.g. [POD](http://en.wikipedia.org/wiki/Plain_Old_Data_Structures) or [standard-layout](http://www.cplusplus.com/reference/type_traits/is_standard_layout/) types). Otherwise, use `class`.
* Class layout should be in the order `public`, `protected`, and then `private`.
  * If one or more of these sections are not needed, then simply don't include them.
* For each of the above specified access levels, the contents of each should follow this given order:
  * nested structures, enums or typedefs.
  * constructor
  * destructor
  * operator overloads
  * methods
  * static member variables
  * non-static member variables.
```c++
class ExampleClass : public SomeParent
{
public:
    ExampleClass(int x, int y);
  
    int GetX() const;
    int GetY() const;

protected:
    virtual void SomeProtectedFunction() = 0;
    static float s_some_variable;

private:
    int m_x;
    int m_y;
};
```

* When using inheritance, follow these guidelines to ensure code maintainability:
  * if you don't intend to allow the class to be inherited, use the `final` keyword. Otherwise, don't use it.
  ```c++
  class ClassName final : ParentClass
  {
      // Class definitions
  };
  ```
  * when overriding methods, **always** use the `override` specifier.
* Be const-correct when defining methods, i.e. use the const specifier for the method itself, its return type and return value.
  * In particular, when returning non-primitive objects (e.g. classes) by value, use the const specifier to make sure the user doesn't accidently try to call non-const methods on the object, expecting it to act like a reference.

### Functions
* For non-writeable parameters of simple type, use pass-by-value (`void func(int parameter)`). Don't use const-values unless you have some reason to.
* For non-writeable parameters with non-trivial copy constructor, use pass-by-const-reference (`void func(const MyFancyClass& parameter)`).
* Functions that specifically modify their parameters should have the respective parameter(s) marked as a pointer so that the variables being modified are syntactically obvious.
  * Don't:

    ```c++
    template<class T>
    inline void Clamp(T& val, const T& min, const T& max)
    {
        if (val < min)
            val = min;
        else if (val > max)
            val = max;
    }
    ```

    Example call: `Clamp(var, 1000, 5000);`

  * What to do:

    ```c++
    template<class T>
    inline void Clamp(T* val, const T& min, const T& max)
    {
        if (*val < min)
            *val = min;
        else if (*val > max)
            *val = max;
    }
    ```

    Example call: `Clamp(&var, 1000, 5000);`
* If there is only one parameter that is being modified by the function, the final value should be returned by the function instead if possible.
    
### Headers and includes
* Use `#pragma once` for header guarding.
* Don't include header files for definitions you aren't actually using
  * In particular, prefer prototyping over `#include`s in header files
  * do: `int ThisOneFunctionINeedFromHeader();`
  * don't: `#include "header_that_defines_ThisOneFunctionINeedFromHeader.h"`
- When declaring includes in a source file, make sure they follow the given pattern:
  * Standard library headers (`map`, `vector`)
  * Boost headers
  * Other library headers
  * System-specific headers (wrapped within an `#ifdef` block unless the source file itself is system-specific).
  * Citra "common" headers
  * Citra "core" headers
  * Citra "core"-ish subsystem headers (e.g. `video_core`), ordered by subsystem in alphabetical order
  * Citra frontend headers (e.g. `citra_qt`)
* Each of the above header sections should also be in alphabetical order
* Project source file headers should be included in a way that is relative to the `[Citra Root]/src` directory, unless they reside in the current source directory.

### Exceptions
* We currently have mostly a "don't use exceptions" policy, however this might be subject to change in the future.

 ### Minor bits
* Use the [nullptr](http://en.cppreference.com/w/cpp/language/nullptr) type over the macro `NULL`.
* Use the `auto` keyword wherever appropriate, but don't go overboard:
  * Don't use `auto` for simple types like `int` or a global-scope class type (`class SomeClass`): that's just complicating things because the reader has to ask himself "uhm, of what type is this variable again?"
  * Use `auto` if the type is annoyingly long to write (`std::map<int, std::string>::iterator`) 
  * Use `auto` if there is no name for the type (e.g. when copying instances of anonymous unions)
  * Use `auto` if the type is not easily accessible (e.g. when copying private nested structures)
  * Use `auto` in range-based for loops (see above)
* Prefer `const type&` over `type&` for references that you don't need to modify
* Prefer `type&` over `type` for variables with non-trivial copy constructors
* Don't use anonymous structs as they aref not standard C++ (anonymous unions and nameless structures are, however):
  * don't:
  ```cpp
  struct Mystruct {
     struct { // nested struct
         int variable;
     };
  ```
  * do:
  ```cpp
  struct Mystruct {
     struct NestedStruct {
         int variable;
     };
     NestedStruct nested_struct;
 
     struct {
         int variable;
     } nested_struct2;
 
     union {
         int some_variable;
     };
  ```
* Prefer pre-increment over post-increment for non-trivial objects. In particular this covers for loops over iterators.
  * do: `for (auto it = container.begin(); it != container.end(); ++it)`
  * don't: `for (auto it = container.begin(); it != container.end(); it++)`
* Try not to use `goto` unless you have a good reason for it.
* Don't introduce compiler warnings. If you find a compiler warning in your or other people's code, please try and fix it.
* Try to avoid using raw pointers as much as possible. There are cases where using a raw pointer is unavoidable, and in these situations it is OK to use them.
  * When "pointing" to other variables, prefer C++'s references.
  * When using dynamical memory allocation (`new`) , prefer standard library containers or use a `unique_ptr`.
  * Use raw pointers if it leads to significant code size overhead or simply can't be avoided (e.g. if the functions from a C library require a raw pointer).
* When defining bit fields, use the `BitField` class instead of the `int variable :4` syntax. It takes a while to get used to, but it allows for some syntactic sugar once you get used to it
  * When passing `BitField` objects to non-typesafe functions, perform explicit casting (either manually or by calling the `Value()` method)
  * Sometimes, it's useful to assign special "meaning" to `BitField` objects by inheritance:
    struct : public BitField<0,5,u32> {
        int GetModifiedValue() {
            return 2 * this->Value();
        }
    } some_field;
* Do not use `using namespace [x];` or `using [x::y];` in headers. Try not to use it at all if you can.
  * C++11's `using new_name = some_type;` syntax is a different thing, and is fine to use in Citra.
* When using struct or class objects from const methods, always return a const-object (even if the return type is a newly constructed, non-reference stack-object). This avoid erroneous usage when one calls these functions expecting them to return references, e.g. TODO example.
