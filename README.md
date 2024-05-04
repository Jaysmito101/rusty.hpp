# rusty.hpp

## What is `rusty.hpp`?

At the core, the idea is to have implement a minimal and lightweight yet powerful and performant system to be able to emulate Rust's borrow checker and general memory model as a C++ header-only library. 

Quoting from [rust-lang.org](https://rustc-dev-guide.rust-lang.org/borrow_check.html), the borrow check is Rust's "secret sauce" – it is tasked with enforcing a number of properties:

* That all variables are initialized before they are used.
* That you can't move the same value twice.
* That you can't move a value while it is borrowed.
* That you can't access a place while it is mutably borrowed (except through the reference).
* That you can't mutate a place while it is immutably borrowed.
* etc

Here too, `rusty.hpp` tries to add these concepts into a regular C++ codebase with ease. `rusty.hpp` also brings in additional features which are a very fundamental part of a Rust workflow like:

* Non-nullable value
* Option< T >
* Result< T, E >
* Rc (todo)
* Arc (todo)

## What to expect?

`rusty.hpp` as the time or writing this is a very experimental thing. Its primary purpose is to experiment and test out different coding styles and exploring a different than usual C++ workspace. This can also be a simple tool for C++ developers to try and get an idea of the typical workflows in a Rust dev environment staying in their comfort zone of C++. That being said, I did try my best to get the apis and behaviour to be as close to native rust as possible but obviously as one might expect there are exceptions to this. For instance, since this is not a part of the compiler itself, there is a limit to which it can go, what I mean by that is borrow errors which would usually be reported by the rust compiler at compile time will be redirected as exceptions in C++, although there are ways to not go the exception way and deal with things more gracefully, which I would go into in the examples. Another instance would be that, since C++ doesnt inherently has a strict concept of lifetime as Rust(they are not very same) cases of dangling references would also be redirected to exceptions and checks at runtime in case the main resorce gets freed.

## How to use this?

Whith all that being said, it still is a interesting library which you could easily try out in your own project. To get started all you need to do is get the header file [rusty.hpp](./rusty.hpp) and include it in your project. It heavily relies on templates to make it completely generic to be integrated anywhere. Also this has got dependencies apart from the C++20 standard library(C++20 is needed so that I could use things like std::format, concepts, etc). After that you need a C++20 compitable compiler (MSVC or gcc 13+) and you are ready to go.

## Examples / Usage

### Rust Like type proxies
`rusty.hpp` defines proxies to standard C++ types named like the Rust types also under the namespace ``rs::literals`` there are C++ literal helpers for these types.

```c++
i8 a = 45_i8;
i16 b = 45_i16;
i32 c = 45_i32;
i64 d = 45_i64;
u8 e = 45_u8;
u16 f = 45_u16;
u32 g = 45_u32;
u64 h = 45_u64;
f32 i = 45.0_f32;
f64 j = 45.0_f64;
```

### print proxies
`rusty.hpp` defines `print` and `println` methods to be like a eqivalent of Rust's `print!` and `println!` macros.

```
println("Hello World! {}", 45);
```

### The Borrow Checker 

In `rusty.hpp` the primary way to use the borrow checker is to use the `rs::Val` type wrapper around everyting. This is a special type that enfoces all the rules and manages the borrowing and ownership of the data in general. Also it is to be noted that this library doesnt use any sort of global state, everything is localized inside the `Val` type.

```
struct Foo {
  i32 a;
  i32 b;

  Foo(i32 a, i32 b) : a(a), b(b) { }
}

auto a = Val(46584); // With primitives
auto b = Val(std::string("Hello World")); // With stl
auto c = Val(Foo {4, 6}); // Pass object as it is
auto d = MakeVal<Foo>(5, 3); // Another way possible
auto e = Val(new Foo(3, 5)); // This pointer is now owned and manged by the Val and you dont need to delete it
```

Now, it is to be noted that we use Move constructors to move the data and take ownership but in C++ there is no way to implicitly know whether an object is moved properly or not, and The destructor will be called for after the move as in:

```
auto a = Val(Foo {4, 6}); // Pass object as it is
// ~Foo called here for the temporary object
```

Now, there are many very easy ways to deal with it,

* For simple object that can be easly be copied its not a issue so you could just ignore it
* For others there are two main options:
  - Implement a move constructor and desctuctor and then track the move and bypass the desctuctor call accordingly
  - Or, the easier way would be to just pass a pointer, the rest of the API will behave the same

Now about using the Val,
```
auto a = Val(45);
auto b = Val(5);
auto c = *a + *b; // here c is a i32, you can wrap it up in a Val if you want

// All of them share same api (mostly)
auto foo0 = Val(Foo{ 0, 0 }); 
auto foo1 = MakeVal<Foo>(1, 1);
auto foo3 = Val(new Foo(0, 0));
auto foo4 = Val(std::make_shared<Foo>(45, 5));
auto foo5 = Val(std::make_unique<Foo>(45, 5));
foo1->a = 2; // be it a pointer or a Object you should be able to acces inner filed like this

```


Now about the actual checks:

```
auto foo2 = foo0; // foo0's ownership is not transfered to foo2
println("foo2: {}", *foo2); // ok
println("foo0: {}", *foo0); // Error: foo0 was moved to foo2

foo0 = pass_through(foo0); // pass ownership of foo0 to pass_through function and the function returns it back
println("foo0: {}", *foo0); // Works as the ownership was returned

non_pass_through(foo0); // pass ownership of foo0 to pass_through function and it gets consumed by it
println("foo0: {}", *foo0); // Error: foo0 was moved to non_pass_through and consumed

non_pass_through(foo0.clone()); // This needs a copy enabled type
println("foo0: {}", *foo0); // Works as the ownership was cloned
```













