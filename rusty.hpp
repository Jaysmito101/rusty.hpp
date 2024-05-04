/*
MIT License

Copyright (c) 2024 Jaysmito Mukherjee

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#pragma once

// C++ Headers
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <queue>
#include <stack>
#include <algorithm>
#include <numeric>
#include <functional>
#include <memory>
#include <utility>
#include <cstdint>
#include <format>
#include <optional>
#include <exception>

// Types
namespace rs 
{
    using i8 = int8_t;
    using i16 = int16_t;
    using i32 = int32_t;
    using i64 = int64_t;
    
    using u8 = uint8_t;
    using u16 = uint16_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using f32 = float;
    using f64 = double;

    using str = std::string;

    // The literal suffixes
    inline namespace literals
    {
        constexpr i8 operator "" _i8(unsigned long long int value) noexcept
        {
            return static_cast<i8>(value);
        }

        constexpr i16 operator "" _i16(unsigned long long int value) noexcept
        {
            return static_cast<i16>(value);
        }

        constexpr i32 operator "" _i32(unsigned long long int value) noexcept
        {
            return static_cast<i32>(value);
        }

        constexpr i64 operator "" _i64(unsigned long long int value) noexcept
        {
            return static_cast<i64>(value);
        }

        constexpr u8 operator "" _u8(unsigned long long int value) noexcept
        {
            return static_cast<u8>(value);
        }

        constexpr u16 operator "" _u16(unsigned long long int value) noexcept
        {
            return static_cast<u16>(value);
        }

        constexpr u32 operator "" _u32(unsigned long long int value) noexcept
        {
            return static_cast<u32>(value);
        }

        constexpr u64 operator "" _u64(unsigned long long int value) noexcept
        {
            return static_cast<u64>(value);
        }

        constexpr f32 operator "" _f32(long double value) noexcept
        {
            return static_cast<f32>(value);
        }

        constexpr f64 operator "" _f64(long double value) noexcept
        {
            return static_cast<f64>(value);
        }
    }

    using namespace literals;
}

// Print proxies with std::format
namespace rs
{
    template<typename... Args>
    void print(const str& format, Args&&... args)
    {
        std::cout << std::vformat(format, std::make_format_args(std::forward<Args>(args)...));
    }

    template<typename... Args>
    void println(const str& format, Args&&... args)
    {
        print(format, std::forward<Args>(args)...);
        std::cout << '\n';
    }
}



// formatter generator macro which just can be used to use the std::ostream operator<< implementation for the rust types
#define RS_FORMATTER_GENERATOR(type) \
    template<> \
    struct std::formatter<type> \
    { \
        template<typename ParseContext> \
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); } \
        template<typename FormatContext> \
        auto format(const type& value, FormatContext& ctx) const \
        { \
            auto ostream = std::ostringstream(); \
            ostream << value; \
            return format_to(ctx.out(), "{}",  ostream.str()); \
        } \
    }; \

// The borrow checker
namespace rs
{

    // the custom exception for the for the Val class
    class ValValueMovedException : public std::exception 
    {
    public:
        const char* what() const noexcept override
        {
            return "Value has already been moved out of the Val object!";
        }
    };

    class AlreadyBorrowedMutablyException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Value has already been borrowed mutably cannot borrow mutably or immutably again!";
        }
    };

    class AlreadyBorrowedImmutablyException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Value has already been borrowed immutably cannot borrow mutably!";
        }
    };

    class RefValueExpiredException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Value this reference is pointing to has already been dropped!";
        }
    };

    class RefMutValueExpiredException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Value this reference is pointing to has already been dropped!";
        }
    };

    class StillBorrowedMutablyException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Value is still borrowed mutably!";
        }
    };

    class StillBorrowedImmutablyException : public std::exception
    {
    public:
        const char* what() const noexcept override
        {
            return "Value is still borrowed immutably!";
        }
    };
    


    namespace option {
        template<typename T>
        class Option;
    }

    namespace result {
		template<typename T, typename E>
		class Result;
	}
    
    template<typename T>
    class Val;

    template <typename T>
    class Ref;

    template <typename T>  
    class RefMut;

    template <typename T>
    concept IsSmartPtr = requires(T t)
    {
        { t.get() } -> std::same_as<typename T::element_type*>;
    };

    template <typename T>
    concept IsRawPointer = requires (T t)
    {
        { *t } -> std::same_as<typename std::remove_pointer_t<T>&>;
    };

    template <typename T>
    concept IsPointerOrSmartPtr = IsSmartPtr<T> || IsRawPointer<T>;

    template <typename T>
    concept IsRef = requires (T t)
    {
        { t.is_ref() } -> std::same_as<bool>;        
    }; 

    template <typename T>
    inline void FreeIfPointerOrSmartPtr(T& value)
    {
        if constexpr (IsRawPointer<T>)
        {
            delete value;
            value = nullptr;
        }
        else if constexpr (IsSmartPtr<T>)
        {
            value.reset();
        }
    }

    template <typename T>
    class Ref 
    {
    public:
        ~Ref() { if (!m_IsMoved && *m_IsValid) { m_DropFunc(); } }

        inline const auto& operator*() const { if(!m_IsMoved && !*m_IsValid) throw RefValueExpiredException(); return *m_Value; }
        inline const auto operator->() const { if(!m_IsMoved && !*m_IsValid) throw RefValueExpiredException(); return m_Value; }
        inline const auto value() const { if(!m_IsMoved && !*m_IsValid) throw RefValueExpiredException(); return m_Value; }

        inline bool is_valid() const { return !m_IsMoved && *m_IsValid; }

        inline constexpr bool is_ref() const { return true; }

        // a move constructor to move the Ref to another Ref and invalidate the current one
        inline Ref(Ref&& other) {
            if (!other.m_Value) throw RefValueExpiredException();

            m_Value = other.m_Value;
            m_IsValid = other.m_IsValid; 

            m_DropFunc = other.m_DropFunc;
            other.m_IsMoved = true;
            other.m_DropFunc = []() {};
        }

        // delete the copy constructor
        inline Ref(const Ref& other) = delete;
        inline Ref(Ref& other) = delete;

        inline Ref& operator=(Ref&& other) {
            if (!other.m_Value) throw RefValueExpiredException();

            m_Value = other.m_Value;
            m_IsValid = other.m_IsValid; 

            m_DropFunc = other.m_DropFunc;
            other.m_IsMoved = true;
            other.m_DropFunc = []() {};

            return *this;
        }

        inline Ref& operator=(Ref& other) = delete;
        inline Ref& operator=(const Ref& other) = delete;



        // bool cast operator to check if the Ref is valid
        inline operator bool() const { return is_valid(); }

        
    private:
        Ref(const T* value, const std::shared_ptr<bool>& isValid, const std::function<void()>& dropFunc) : m_Value(value), m_IsValid(isValid), m_DropFunc(dropFunc) {}
        static inline auto None() { return Ref(nullptr, nullptr, []() {}); }

    private:
        const T* m_Value = nullptr;
        std::function<void()> m_DropFunc;
        std::shared_ptr<const bool> m_IsValid;
        bool m_IsMoved = false;

        template<typename U>
        friend class Val;

        // friend Result
        template <typename U, typename E>
        friend class result::Result;
    };

    template <typename T>
    class RefMut
    {
    public:
        ~RefMut() { if (!m_IsMoved && *m_IsValid) { m_DropFunc(); } }

        inline auto& operator*() { if(!m_IsMoved && !*m_IsValid) throw RefMutValueExpiredException(); return *m_Value; }
        inline const auto& operator*() const { if(!m_IsMoved && !*m_IsValid) throw RefMutValueExpiredException(); return *m_Value; }

        inline auto operator->() { if(!m_IsMoved && !*m_IsValid) throw RefMutValueExpiredException(); return m_Value; }
        inline const auto operator->() const { if(!m_IsMoved && !*m_IsValid) throw RefMutValueExpiredException(); return m_Value; }

        inline auto value() { if(!m_IsMoved && !*m_IsValid) throw RefMutValueExpiredException(); return m_Value; }
        inline const auto value() const { if(!m_IsMoved && !*m_IsValid) throw RefMutValueExpiredException(); return m_Value; }

        inline bool is_valid() const { return !m_IsMoved && *m_IsValid; }

        inline constexpr bool is_ref() const { return true; }


        // a move constructor to move the RefMut to another RefMut and invalidate the current one
        inline RefMut(RefMut&& other) {
            if (!other.m_Value) throw RefMutValueExpiredException();

            m_Value = other.m_Value;
            m_IsValid = other.m_IsValid; 

            m_DropFunc = other.m_DropFunc;
            other.m_IsMoved = true;
            other.m_DropFunc = []() {};
        }

        
        // delete the copy constructor
        inline RefMut(RefMut& other) = delete;
        inline RefMut(const RefMut& other) = delete;
        
        inline RefMut& operator=(RefMut&& other) {
            if (!other.m_Value) throw RefMutValueExpiredException();

            m_Value = other.m_Value;
            m_IsValid = other.m_IsValid; 

            m_DropFunc = other.m_DropFunc;
            other.m_IsMoved = true;
            other.m_DropFunc = []() {};

            return *this;
        }

        inline RefMut& operator=(RefMut& other) = delete;
        inline RefMut& operator=(const RefMut& other) = delete;


        // bool cast operator to check if the RefMut is valid
        inline operator bool() const { return is_valid(); }


    private:
        RefMut(T* value, const std::shared_ptr<bool> isValid, const std::function<void()>& dropFunc) : m_Value(value), m_IsValid(isValid), m_DropFunc(dropFunc) {}
        static inline auto None() { return RefMut(nullptr, nullptr, []() {}); }

    private:
        T* m_Value = nullptr;
        std::function<void()> m_DropFunc;
        std::shared_ptr<const bool> m_IsValid;
        bool m_IsMoved = false;

        template<typename U>
        friend class Val;

        // friend Result
        template <typename U, typename E>
        friend class result::Result;
    };

    /// The Val class. This is the main Value type class for the borrow checker
    template<typename T>
    class Val
    {
    public:
        inline Val(Val&& other) {
            if (this == &other) return;

            if (!other.m_Value.has_value()) throw ValValueMovedException();
            m_Value.swap(other.m_Value);
            m_IsValid.swap(other.m_IsValid);

            std::swap(m_NumMutableCopies, other.m_NumMutableCopies);
            std::swap(m_NumImutableCopies, other.m_NumImutableCopies);

            if (other.m_IsValid) *other.m_IsValid = false;
            // other.m_IsValid = std::make_shared<bool>(false); // Maybe this is not needed at all
            other.m_Value.reset();
        }

        inline Val(Val& other) {
            if (!other.m_Value.has_value()) throw ValValueMovedException();
            m_Value.swap(other.m_Value); 
            m_IsValid.swap(other.m_IsValid); 

            std::swap(m_NumMutableCopies, other.m_NumMutableCopies);
            std::swap(m_NumImutableCopies, other.m_NumImutableCopies);


            if (other.m_IsValid) *other.m_IsValid = false;
            // other.m_IsValid = std::make_shared<bool>(false); // Maybe this is not needed at all
            other.m_Value.reset();
        }

        inline Val(const Val& other) = delete;
        inline Val(T&& value) : m_Value(std::move(value)) { m_IsValid = std::make_shared<bool>(true); }
        
        inline auto& operator*() {
            if (!m_Value.has_value()) throw ValValueMovedException();

            if constexpr (IsRef<T>)
                return m_Value.value().operator*();
            else if constexpr (IsPointerOrSmartPtr<T>)
                return *m_Value.value();
            else
                return m_Value.value();            
        }
        
        inline const auto& operator*() const {
            if (!m_Value.has_value()) throw ValValueMovedException();

            if constexpr (IsRef<T>)
                return m_Value.value().operator*();
            else if constexpr (IsPointerOrSmartPtr<T>)
                return *m_Value.value();
            else
                return m_Value.value();
        }

        inline auto value() { 
            if (!m_Value.has_value()) throw ValValueMovedException(); 
            // if T is a Ref or RefMut return the value of the Ref or RefMut .value() function
            if constexpr (IsRef<T>)
                return m_Value.value().value();
            else if constexpr (IsPointerOrSmartPtr<T>)
                return m_Value.value(); 
            else 
                return &m_Value.value();
        }
        
        inline const auto value() const { 
            if (!m_Value.has_value()) throw ValValueMovedException(); 
            // if T is a Ref or RefMut return the value of the Ref or RefMut .value() function
            if constexpr (IsRef<T>)
                return m_Value.value().value();
            else if constexpr (IsPointerOrSmartPtr<T>)
                return m_Value.value(); 
            else 
                return &m_Value.value();
        }

        inline auto operator->() { return this->value(); }
        inline const auto operator->() const { return this->value(); } 

        inline Val& operator=(Val&& other) {
            if (!other.m_Value.has_value()) throw ValValueMovedException(); 
            m_Value.swap(other.m_Value);
            m_IsValid.swap(other.m_IsValid);

            std::swap(m_NumMutableCopies, other.m_NumMutableCopies);
            std::swap(m_NumImutableCopies, other.m_NumImutableCopies);

            if (other.m_IsValid) *other.m_IsValid = false;
            // other.m_IsValid = std::make_shared<bool>(false); // Maybe this is not needed at all
            other.m_Value.reset();
            return *this;
        }

        inline Val& operator=(Val& other) {
            if (!other.m_Value.has_value()) throw ValValueMovedException(); 
			m_Value.swap(other.m_Value);
			m_IsValid.swap(other.m_IsValid);
			std::swap(m_NumMutableCopies, other.m_NumMutableCopies);
			std::swap(m_NumImutableCopies, other.m_NumImutableCopies);
			if (other.m_IsValid) *other.m_IsValid = false;
			// other.m_IsValid = std::make_shared<bool>(false); // Maybe this is not needed at all
			other.m_Value.reset();
			return *this;
        }

        inline Val& operator=(const Val& other) = delete;

        inline Val<T> clone() const { return Val<T>(T(*m_Value)); }

        inline bool is_valid() const { return m_Value.has_value(); }

        inline void drop() {
			if (!m_Value.has_value()) throw ValValueMovedException();
			if (m_NumImutableCopies > 0) throw StillBorrowedImmutablyException();
			if (m_NumMutableCopies > 0) throw StillBorrowedMutablyException();
			m_Value.reset();
            if (m_IsValid) *m_IsValid = false;
		}

        inline auto borrow() {
            if (!m_Value.has_value()) throw ValValueMovedException();

            if (m_NumMutableCopies > 0) throw AlreadyBorrowedMutablyException();
            m_NumImutableCopies++;

            if constexpr (IsSmartPtr<T>)
                return Val<Ref<std::remove_pointer_t<T>>>( Ref<std::remove_pointer_t<T>>( m_Value.get(), m_IsValid, [this]() { m_NumImutableCopies--; } ));
            else if constexpr (IsRawPointer<T>)
                return Val<Ref<std::remove_pointer_t<T>>>( Ref<std::remove_pointer_t<T>>( m_Value.value(), m_IsValid, [this]() { m_NumImutableCopies--; } ));
            else
                return Val<Ref<T>>( Ref<T>( &m_Value.value(), m_IsValid, [this]() { m_NumImutableCopies--; } ));
        }

        inline auto borrow_mut() {
            if (!m_Value.has_value()) throw ValValueMovedException();

            if (m_NumMutableCopies > 0) throw AlreadyBorrowedMutablyException();
            if (m_NumImutableCopies > 0) throw AlreadyBorrowedImmutablyException();
            m_NumMutableCopies++;

            if constexpr (IsSmartPtr<T>)
                return Val<RefMut<std::remove_pointer_t<T>>>( RefMut<std::remove_pointer_t<T>>( m_Value.get(), m_IsValid, [this]() { m_NumMutableCopies--; } ));
			else if constexpr (IsRawPointer<T>)
				return Val<RefMut<std::remove_pointer_t<T>>>( RefMut<std::remove_pointer_t<T>>( m_Value.value(), m_IsValid, [this]() { m_NumMutableCopies--; } ));
			else
				return Val<RefMut<T>>( RefMut<T>( &m_Value.value(), m_IsValid, [this]() { m_NumMutableCopies--; } ));
        }

        inline u64 num_mutable_copies() const { return m_NumMutableCopies; }
        inline u64 num_imutable_copies() const { return m_NumImutableCopies; }

        inline operator bool() const { return is_valid(); }

        inline ~Val() {
            if (is_valid()) {
                *m_IsValid.get() = false;  
                FreeIfPointerOrSmartPtr(m_Value.value());
            } 
        }


    private:
        inline Val() { m_IsValid = std::make_shared<bool>(false); m_Value.reset(); }

        static inline Val<T> None() { return Val<T>(); }

    private:
        u64 m_NumMutableCopies = 0;
        u64 m_NumImutableCopies = 0;
        std::optional<T> m_Value;
        std::shared_ptr<bool> m_IsValid;

        template<typename U>
        friend class Ref;

        template<typename U>
        friend class RefMut;

        template<typename U>
        friend class Val;

        template<typename U>
        friend class option::Option;

        template<typename U, typename V>
        friend class result::Result;
    };

    template<typename T>
    Val(T&&) -> Val<T>;

    template<typename T, typename... Args>
    inline Val<T> MakeVal(Args&&... args)
    {
        return Val(T(std::forward<Args>(args)...));
    }

}

template<typename T>
auto operator<<(std::ostream& os, const rs::Val<T>& val) -> std::ostream&
{
	os << "Val<" << typeid(T).name() << ">";
	if (val.is_valid()) os << "{" << *val << "}";
	return os;
}

template <typename T>
auto operator<<(std::ostream& os, const rs::Ref<T>& val) -> std::ostream&
{
    os << "Ref<" << typeid(T).name() << ">";
    if (val.is_valid()) os << "{" << *val << "}";
    return os;
}

template <typename T>
auto operator<<(std::ostream& os, const rs::RefMut<T>& val) -> std::ostream&
{
    os << "RefMut<" << typeid(T).name() << ">";
    if (val.is_valid()) os << "{" << *val << "}";
    return os;
}


namespace std
{


    template<typename T>
    struct formatter<rs::Val<T>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
        template<typename FormatContext>
        auto format(const rs::Val<T>& value, FormatContext& ctx) const
        {
            std::stringstream ss;
            ss << "Val<" << typeid(T).name() << ">";           
            if (value.is_valid()) ss << "{" << *value << "}";
            return format_to(ctx.out(), "{}", ss.str());
        }
    };

    template<typename T>
    struct formatter<rs::Ref<T>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
        template<typename FormatContext>
        auto format(const rs::Ref<T>& value, FormatContext& ctx) const
        {
            std::stringstream ss;
            ss << "Ref<" << typeid(T).name() << ">";
            if (value.is_valid()) ss << "{" << *value << "}";
            return format_to(ctx.out(), "{}", ss.str());
        }
    };

    template<typename T>
    struct formatter<rs::RefMut<T>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
        template<typename FormatContext>
        auto format(const rs::RefMut<T>& value, FormatContext& ctx) const
        {
            std::stringstream ss;
            ss << "RefMut<" << typeid(T).name() << ">";
            if (value.is_valid()) ss << "{" << *value << "}";
            return format_to(ctx.out(), "{}", ss.str());
        }
    };
}


// Option and Result 
namespace rs {

    namespace option {

        // the exceptions
        class OptionIsNoneException : public std::exception
        {
        public:
            const char* what() const noexcept override
            {
                return "Option is None!";
            }
        };



        template<typename T>
        class Option {
        public:

            inline bool is_some() const { return m_Value.is_valid(); }
            inline bool is_none() const { return !m_Value.is_valid(); }

            inline auto as_ref() {
                if (is_some()) return Some(m_Value.borrow());
                else return None< Ref<std::remove_pointer_t<T>> >();
            }

            inline auto as_mut() {
                if (is_some()) return Some(m_Value.borrow_mut());
                else return None< RefMut<std::remove_pointer_t<T>>>();
			}

            inline auto take() {
                if (is_some()) {
					auto value = m_Value;
					m_Value = Val<T>::None();
					return Some(value);
				}
				else return None<T>();
			}

            inline auto expect(const std::string& msg) {
				if (is_some()) return m_Value;
				else throw OptionIsNoneException();
			}

            // here this is same as take in rust except that it throws an exception if the Option is None
            inline auto unwrap() {
                if (is_some()) return m_Value;
                else throw OptionIsNoneException();
            }

            inline auto unwrap_or(T&& value) {
                if (is_some()) return m_Value;
                else return Val<T>(std::forward<T>(value));
            }

            inline auto unwrap_or_else(std::function<T()> f) {
                if (is_some()) return m_Value;
                else return Val<T>(f());
            }

            template<typename U>
            inline auto map(std::function<U(T)> f) {
				if (is_some()) return Some<U>(f(m_Value.value()));
				else return None<U>();
			}


            inline auto unsafe_ptr() const { return m_Value.value(); }

            template <typename U>
            static inline auto Some(Val<U> value) { return Option<U>(value); }

            template <typename U>
            static inline auto None() { return Option<U>(); }

            inline ~Option() { }

            // a move constructor
            inline Option(Option&& other) { m_Value = other.m_Value; }

            // a move assignment operator
            inline Option& operator=(Option&& other) {
                if (this != &other) {
					m_Value = other.m_Value;
				}
				return *this;
			}

            // delete the copy constructor and copy assignment operator
            inline Option(const Option& other) = delete;
            inline Option& operator=(const Option& other) = delete;

            inline auto cloned() {
                if (is_some()) return Some(m_Value.clone());
				else return None<T>();
            }

        private:
            inline Option() = default;
            inline Option(Val<T> value) : m_Value(value) {}


        private:
            Val<T> m_Value;

            template<typename U>
            friend class Option;
        };

        template<typename T>
        inline auto None() { return Option<T>::template None<T>(); }

        template<typename T>
        inline auto Some(Val<T> value) {
            return Option<T>::template Some(value);
        }


        template<typename T>
        inline auto Some(T&& value) {
            return Option<T>::template Some(Val<T>(std::forward<T>(value)));
        }

    }
    

    using namespace option;
}

template<typename T>
auto operator<<(std::ostream& os, const rs::option::Option<T>& option) -> std::ostream&
{
    os << "Option<" << typeid(T).name() << ">";
    if (option.is_some()) os << "{" << *(option.unsafe_ptr()) << "}";
    else os << "{None}";
    return os;
}

namespace std
{
    template<typename T>
    struct formatter<rs::option::Option<T>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
        template<typename FormatContext>
        auto format(const rs::option::Option<T>& value, FormatContext& ctx) const
        {
            std::stringstream ss;
            ss << "Option<" << typeid(T).name() << ">";
            if (value.is_some()) ss << "{" << *(value.unsafe_ptr()) << "}";
            else ss << "{None}";
            return format_to(ctx.out(), "{}", ss.str());
        }
    };
}

namespace rs {
    namespace result {
		// the exceptions
        class ResultIsErrException : public std::exception
        {
		public:
            const char* what() const noexcept override
            {
				return "Result is Err!";
			}
		};

		template<typename T, typename E>
        class Result {
		public:

			inline bool is_ok() const { return m_Value.is_valid(); }
			inline bool is_err() const { return m_Error.is_valid(); }
            inline bool is_valid() const { return m_Value.is_valid() || m_Error.is_valid(); }
            
            inline auto unsafe_ok_ptr() const { return m_Value.value(); }
            inline auto unsafe_err_ptr() const { return m_Error.value(); }


            inline auto ok() {
                if (m_Value.is_valid()) return Some(m_Value);
                return None<T>();
            }

            inline auto err() {
				if (m_Error.is_valid()) return Some(m_Error);
				return None<E>();
			}

            inline auto as_ref() {
			    if (m_Value.is_valid()) return Ok<Ref<T>, Ref<E>>(m_Value.borrow());
                else return Err<Ref<T>, Ref<E>>(m_Error.borrow());
			}

            inline auto as_mut() {
				if (m_Value.is_valid()) return Ok<RefMut<T>, RefMut<E>>(m_Value.borrow_mut());
				else return Err<RefMut<T>, RefMut<E>>(m_Error.borrow_mut());
            }

            template <typename U>
            inline auto map(std::function<U(T)> f) {
                if (m_Value.is_valid()) return Ok<U, E>(f(m_Value.value()));
                else return Err<U, E>(m_Error);
            }

            inline auto expect(const std::string& msg) {
				if (m_Value.is_valid()) return m_Value;
				else throw std::runtime_error(msg);
			}

            inline auto unwrap() {
				if (m_Value.is_valid()) return m_Value;
				else throw ResultIsErrException();
			}

            inline auto unwrap_err() {
                if (m_Error.is_valid()) return m_Error;
				else throw ResultIsErrException();
            }

            inline auto unwrap_or(T&& value) {
				if (m_Value.is_valid()) return m_Value;
				else return Val<T>(std::forward<T>(value));
			}

            inline auto cloned() {
				if (m_Value.is_valid()) return Ok<T, E>(m_Value.clone());
                else return Err<T, E>(m_Error.clone());
            }




            template<typename U, typename V>
            static inline auto Ok(Val<U> value) { return Result<U, V>(value, Val<V>::None()); }

            template<typename U, typename V>
            static inline auto Err(Val<V> value) { return Result<U, V>(Val<U>::None(), value); }

            // The move constructor and move assignment operator
            inline Result(Result&& other) {
                if (this != &other) {
                    if (other.m_Value.is_valid()) {
                        if (m_Error.is_valid()) m_Error.drop();
                        m_Value = other.m_Value;
                    }
                    else {
                        if (m_Value.is_valid()) m_Value.drop();
                        m_Error = other.m_Error;
                    }
                }
            }

            inline Result& operator=(Result&& other) {
                if (this != &other) {
                    if (other.m_Value.is_valid()) {
                        if (m_Error.is_valid()) m_Error.drop();
                        m_Value = other.m_Value;
                    }
                    else {
                        if (m_Value.is_valid()) m_Value.drop();
                        m_Error = other.m_Error;
                    }
                }
                return *this;
            }

            // delete the copy constructor and copy assignment operator
            inline Result(const Result& other) = delete;
            inline Result& operator=(const Result& other) = delete;




        private:
            inline Result(Val<T> value, Val<E> error) {
                if (value.is_valid()) {
                    m_Value = value;
                }
                else {
                    m_Error = error;
                }
            }

        private:
            Val<T> m_Value;
            Val<E> m_Error;

            template<typename U, typename V>
            friend class Result;
        };

        template<typename T, typename E>
        inline auto Ok(Val<T> value) {
            return Result<T, E>::template Ok<T, E>(value);
        }

        template<typename T, typename E>
        inline auto Ok(T&& value) {
			return Result<T, E>::template Ok<T, E>(Val<T>(std::forward<T>(value)));
		}

        template<typename T, typename E>
        inline auto Err(Val<E> value) {
            return Result<T, E>::template Err<T, E>(value);
        }

        template<typename T, typename E>
        inline auto Err(E&& value) {
            return Result<T, E>::template Err<T, E>(Val<E>(std::forward<E>(value)));
        }


    }

    using namespace result;
}

namespace std
{
    template<typename T, typename E>
    struct formatter<rs::result::Result<T, E>>
    {
        template<typename ParseContext>
        constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
        template<typename FormatContext>
        auto format(const rs::result::Result<T, E>& value, FormatContext& ctx) const
        {
            std::stringstream ss;
            ss << "Result<" << typeid(T).name() << ", " << typeid(E).name() << ">";
            if (value.is_ok()) ss << "{Ok: " << *(value.unsafe_ok_ptr()) << "}";
            else ss << "{Err: " << *(value.unsafe_err_ptr()) << "}";
            return format_to(ctx.out(), "{}", ss.str());
        }
    };
}