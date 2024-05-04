# rusty.hpp

## Index

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
