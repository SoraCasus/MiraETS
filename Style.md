# C++ Coding Style Guide

This project follows a specific coding style defined in the `.clang-format` file. It is largely based on the **LLVM** style but with several customizations to improve readability and consistency.

## General Principles

- **Indentation**: Use 4 spaces per indentation level.
- **Continuation Indent**: Use 8 spaces for continued lines.
- **Tab Width**: 4 spaces.
- **Column Limit**: 120 characters.
- **Line Endings**: Ensure a newline at the end of every file (EOF).
- **Max Empty Lines**: A maximum of 2 consecutive empty lines is allowed.

## Formatting

### 1. Braces and Spacing

- **Brace Style**: Use a K&R-like style where braces open on the same line as the statement (class, function, control statements) and do not wrap.
- **Namespace Indentation**: All content within namespaces is indented.
- **Parentheses**: Add spaces inside parentheses.
  - `void function( int arg );`
  - `if ( condition ) { ... }`
- **Template Angles**: Add spaces inside template angle brackets.
  - `std::vector< int > myVector;`
  - `template< typename T >`
- **Square Brackets**: Add spaces inside square brackets.
  - `array[ i ] = value;`
- **Conditional Statements**: Add spaces in conditional statements.
  - `while ( true ) { ... }`
- **C-Style Casts**: Add spaces inside cast parentheses and after the cast.
  - `( int ) value`

### 2. Declarations

- **Pointer/Reference Alignment**: Align pointers and references to the left.
  - `int* ptr;`
  - `MyClass& ref;`
- **Return Types**: Always break after the return type in both definitions and declarations.
  ```cpp
  void
  MyFunction();
  ```
- **Template Declarations**: Always break after template declarations.
  ```cpp
  template< typename T >
  void
  MyTemplateFunction();
  ```
- **Access Modifiers**: `public`, `protected`, and `private` are aligned with the `class` or `struct` keyword (they are offset by -4 from the member indentation).
- **Constructor Initializers**: Break after the colon and do not put the comma before the initializer.
  ```cpp
  MyClass() :
      m_Value( 0 ),
      m_Other( 1 )
  {}
  ```

### 3. Alignment

- **Consecutive Assignments/Declarations**: Do not align consecutive assignments or declarations.
- **Operands**: Align operands in multi-line expressions.
- **Trailing Comments**: Do not align trailing comments.

### 4. Includes

Includes are categorized and sorted:
1. Standard Library headers (`<...>`)
2. Local headers (`"..."`)
3. All other headers

## Short Blocks and Functions

- **Short Blocks**: Do not put short blocks on a single line.
- **Short Functions**: Do not put short functions on a single line; they must follow the standard breaking rules.

## Example

```cpp
namespace Mira::ETS {
    template< typename T >
    class MyClass {
    public:
        int*
        GetValue( int index ) {
            if ( index >= 0 ) {
                return &m_Values[ index ];
            }
            return nullptr;
        }

    private:
        std::vector< int > m_Values;
    };
}
```
