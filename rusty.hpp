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

#define RS_EXPORT // A hack for now...

// C++ Headers
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <numeric>
#include <memory>
#include <cstdint>
#include <format>
#include <optional>
#include <mutex>
#include <atomic>
#include <functional>
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
		constexpr i8 operator "" _i8(unsigned long long int value) noexcept { return static_cast<i8>(value); }
		constexpr i16 operator "" _i16(unsigned long long int value) noexcept { return static_cast<i16>(value); }
		constexpr i32 operator "" _i32(unsigned long long int value) noexcept { return static_cast<i32>(value); }
		constexpr i64 operator "" _i64(unsigned long long int value) noexcept { return static_cast<i64>(value); }
		constexpr u8 operator "" _u8(unsigned long long int value) noexcept { return static_cast<u8>(value); }
		constexpr u16 operator "" _u16(unsigned long long int value) noexcept { return static_cast<u16>(value); }
		constexpr u32 operator "" _u32(unsigned long long int value) noexcept { return static_cast<u32>(value); }
		constexpr u64 operator "" _u64(unsigned long long int value) noexcept { return static_cast<u64>(value); }
		constexpr f32 operator "" _f32(long double value) noexcept { return static_cast<f32>(value); }
		constexpr f64 operator "" _f64(long double value) noexcept { return static_cast<f64>(value); }
	}

#ifdef RS_EXPORT
	using namespace literals;
#endif
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

// The Exceptions
namespace rs
{
	namespace exceptions {
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

		class RefImmutableBorrowException : public std::exception
		{
		public:
			const char* what() const noexcept override
			{
				return "Cannot borrow mutably because the reference is immutable!";
			}
		};

		class ResultRawIsErrException : public std::exception
		{
		public:
			const char* what() const noexcept override
			{
				return "ResultRaw is Err!";
			}
		};
	}

#ifdef RS_EXPORT
	using namespace exceptions;
#endif
}

namespace rs {

	template <typename Type>
	using RawPtr = Type*;

	template<typename Type, bool ThreadSafe>
	class ValRaw;

	template<typename Type, bool Mutability, bool ThreadSafe>
	class RefRaw;

	template<typename Type, bool ThreadSafe>
	class OptionRaw;

	template<typename Type, typename Err, bool ThreadSafe>
	class ResultRaw;

	template<typename Type>
	concept IsRefRaw = requires(Type t) {
		{ t.is_ref_mutable() } -> std::same_as<bool>;
	};

	template <typename Type>
	concept IsSmartPtr = requires(Type t) {
		{ t.get() } -> std::same_as<typename Type::element_type*>;
		{ *t } -> std::same_as<typename Type::element_type&>;
		{ t.reset() } -> std::same_as<void>;
		{ t.use_count() };
	};

	template <typename Type>
	concept IsRawPtr = requires(Type t) {
		{ *t } ->  std::same_as<typename std::remove_pointer_t<Type>&>;
	} && !IsSmartPtr<Type>;

	template <typename Type>
	concept IsSmartPtrOrRawPtr = IsSmartPtr<Type> || IsRawPtr<Type>;

	template<bool ThreadSafe>
	class ValidityCheckBlock
	{
	public:
		inline ValidityCheckBlock() = delete;
		inline ValidityCheckBlock(const ValidityCheckBlock& other) = delete;
		inline ValidityCheckBlock(ValidityCheckBlock&& other) = delete;
		inline auto operator=(const ValidityCheckBlock& other)->ValidityCheckBlock & = delete;
		inline auto operator=(ValidityCheckBlock&& other)->ValidityCheckBlock & = delete;

		inline ValidityCheckBlock(bool validity) : m_Validity(validity), m_RefCounter(1) {}

		inline ~ValidityCheckBlock() = default;

		inline bool is_valid() const {
			return m_Validity;
		}

		inline void drop() {
			m_Validity = false;
		}

		inline void increment() {
			m_RefCounter++;
		}

		inline void decrement() {
			m_RefCounter--;
		}

		inline u32 get_ref_count() const {
			return m_RefCounter;
		}

	private:
		std::conditional_t<ThreadSafe, std::atomic_bool, bool> m_Validity = true;
		std::conditional_t<ThreadSafe, std::atomic_uint32_t, u32> m_RefCounter = 0;
	};


	template<bool ThreadSafe>
	class ValidityChecker
	{
	public:

		inline ValidityChecker()
			: m_Block(nullptr)
		{
			if constexpr (ThreadSafe) {
				m_Mutex = nullptr;
			}
		}

		inline ValidityChecker(bool value)
		{
			m_Block = new ValidityCheckBlock<ThreadSafe>(value);
			if constexpr (ThreadSafe) {
				m_Mutex = new std::mutex();
			}
		}

		inline ValidityChecker(const ValidityChecker& other) noexcept
			: m_Block(other.m_Block)
		{
			if constexpr (ThreadSafe) {
				m_Mutex = other.m_Mutex;
			}
			m_Block->increment();
		}

		inline ValidityChecker(ValidityChecker&& other) noexcept
			: m_Block(other.m_Block)
		{
			if constexpr (ThreadSafe) {
				m_Mutex = other.m_Mutex;
			}
			other.m_Block = nullptr;
			other.m_Mutex = nullptr;
		}

		inline auto operator=(const ValidityChecker& other) -> ValidityChecker& {
			if (this != &other) {
				reset();
				m_Block = other.m_Block;
				if constexpr (ThreadSafe) {
					m_Mutex = other.m_Mutex;
				}
				m_Block->increment();
			}
			return *this;
		}

		inline auto operator=(ValidityChecker&& other) -> ValidityChecker& {
			if (this != &other) {
				reset();
				m_Block = other.m_Block;
				if constexpr (ThreadSafe) {
					m_Mutex = other.m_Mutex;
				}
				other.m_Block = nullptr;
				if constexpr (ThreadSafe) {
					other.m_Mutex = nullptr;
				}
			}
			return *this;
		}

		inline ~ValidityChecker() {
			reset();
		}

		inline void reset() {
			if (m_Block != nullptr) {

				if constexpr (ThreadSafe) {
					bool didDrop = false;
					{
						std::lock_guard<std::mutex> lock(*m_Mutex);
						m_Block->decrement();

						if (m_Block->get_ref_count() == 0) {
							delete m_Block;
							m_Block = nullptr;
							didDrop = true;
						}
					}

					if (didDrop) {
						delete m_Mutex;
						m_Mutex = nullptr;
					}
				}
				else {
					m_Block->decrement();

					if (m_Block->get_ref_count() == 0) {
						delete m_Block;
						m_Block = nullptr;
					}
				}
			}
		}

		inline bool is_valid() const {
			return !is_null() && m_Block->is_valid();
		}

		inline bool is_null() const {
			return m_Block == nullptr;
		}

		inline auto ref_count() const {
			return m_Block->get_ref_count();
		}

		inline void drop() {
			m_Block->drop();
		}

		inline void swap(ValidityChecker& other) {
			std::swap(m_Block, other.m_Block);
			if constexpr (ThreadSafe) {
				std::swap(m_Mutex, other.m_Mutex);
			}
		}


		inline auto get_mutex() {
			return m_Mutex;
		}

	private:
		RawPtr<ValidityCheckBlock<ThreadSafe>> m_Block;
		std::conditional_t<ThreadSafe, RawPtr<std::mutex>, bool> m_Mutex;
	};

	template<typename Type, bool Mutability, bool ThreadSafe>
	class RefRaw {
	public:
		using RefBackType = std::conditional_t<Mutability,
			std::conditional_t<ThreadSafe, std::atomic_bool, bool>,
			std::conditional_t<ThreadSafe, std::atomic_uint32_t, u32>>;

		using ValueType = std::conditional_t<Mutability, RawPtr<std::remove_pointer_t<Type>>, RawPtr<std::remove_pointer_t<Type> const>>;

		inline RefRaw(RefRaw&& other) noexcept
			: m_Ref(other.m_Ref),
			m_ImmutableBorrowCount(other.m_ImmutableBorrowCount),
			m_DropCheck(other.m_DropCheck)
		{
			other.reset_values();
		}

		inline RefRaw(RefRaw& other) = delete;
		inline RefRaw(const RefRaw& other) = delete;

		inline ~RefRaw() {
			drop();
		}

		inline auto& operator=(RefRaw&& other) {
			if (this != &other) {
				drop();
				m_Ref = other.m_Ref;
				m_ImmutableBorrowCount = other.m_ImmutableBorrowCount;
				m_DropCheck = other.m_DropCheck;
				other.reset_values();
			}
			return *this;
		}

		inline auto& operator=(RefRaw& other) = delete;
		inline auto& operator=(const RefRaw& other) = delete;

		inline bool is_valid() const {
			return m_Ref != nullptr && m_DropCheck.is_valid();
		}

		inline operator bool() const {
			return is_valid();
		}

		inline const auto value() const {
			if (!is_valid()) {
				throw RefValueExpiredException();
			}

			return m_Ref;
		}

		inline ValueType value() requires(Mutability) {
			if (!is_valid()) {
				throw RefValueExpiredException();
			}

			return m_Ref;
		}

		inline void drop() {
			if (m_DropCheck.is_valid()) {
				if constexpr (Mutability) {
					*m_ImmutableBorrowCount = false;
				}
				else {
					*m_ImmutableBorrowCount -= 1;
				}
			}
			reset_values();
		}


		inline auto operator->() const {
			return value();
		}

		inline auto operator->() {
			return value();
		}

		inline const auto& operator*() const {
			return *value();
		}

		inline auto& operator*() {
			return *value();
		}

		inline constexpr bool is_ref_mutable() const {
			return Mutability;
		}

	private:
		RefRaw(
			ValueType ref,
			RawPtr<RefBackType> backCountPtr,
			ValidityChecker<ThreadSafe> dropCheck)
			: m_Ref(ref)
		{
			m_ImmutableBorrowCount = backCountPtr;
			m_DropCheck = dropCheck;
		}

		inline void reset_values() {
			m_Ref = nullptr;
			m_ImmutableBorrowCount = nullptr;
			m_DropCheck = ValidityChecker<ThreadSafe>();
		}

	private:
		ValueType m_Ref = nullptr;
		ValidityChecker<ThreadSafe> m_DropCheck;
		RawPtr<RefBackType> m_ImmutableBorrowCount;


		template <typename Ty, bool Ts>
		friend class ValRaw;
	};

	template<typename Type>
	using Ref = RefRaw<Type, false, false>;

	template<typename Type>
	using RefMut = RefRaw<Type, true, false>;

	template<typename Type>
	using SafeRef = RefRaw<Type, false, true>;

	template<typename Type>
	using SafeRefMut = RefRaw<Type, true, true>;


	template<typename Type, bool ThreadSafe>
	class ValRaw
	{
	public:

		// static assert so that the type is either a static type or
		// if it is a pointer, it is a pointer to a static type and not to another pointer
		// T -> ok, T* -> ok, T** -> not ok, std::unique_ptr<T> -> ok, std::shared_ptr<T> -> ok, std::unique_ptr<T*> -> not ok
		static_assert(
			std::is_pointer_v<Type> ?
			std::is_pointer_v<std::remove_pointer_t<Type>> ? false : true
			: true,
			"Type must be a static type or a pointer to a static type, and not a pointer to another pointer"
			);

		// WARNING: This will only copy the value and take ownership of a copy
		//          and not the actual value itself.
		//          If you want to take ownership of the actual value, use the
		//          move constructor instead.
		inline ValRaw(const Type& value) noexcept
			: m_Value(value)
		{
			m_DropCheck = ValidityChecker<ThreadSafe>(true);
			m_ImmutableBorrowCount = 0;
			m_IsMutableBorrowed = false;
		}

		inline ValRaw(Type&& value) noexcept
			: m_Value(std::move(value))
		{
			m_DropCheck = ValidityChecker<ThreadSafe>(true);
			m_ImmutableBorrowCount = 0;
			m_IsMutableBorrowed = false;
		}

		inline ValRaw(ValRaw&& other)
		{
			if (!other.is_valid()) {
				throw ValValueMovedException();
			}

			m_DropCheck = std::move(other.m_DropCheck);
			m_Value = std::move(other.m_Value);
			m_ImmutableBorrowCount = static_cast<u32>(other.m_ImmutableBorrowCount);
			m_IsMutableBorrowed = static_cast<bool>(other.m_IsMutableBorrowed);

			other.reset_values();
		}

		inline ValRaw(ValRaw& other) {
			if (!other.is_valid()) {
				throw ValValueMovedException();
			}

			m_DropCheck = other.m_DropCheck;
			m_Value = std::move(other.m_Value);
			m_ImmutableBorrowCount = static_cast<u32>(other.m_ImmutableBorrowCount);
			m_IsMutableBorrowed = static_cast<bool>(other.m_IsMutableBorrowed);

			other.reset_values();
		}

		inline ValRaw(const ValRaw& other) = delete;

		inline auto operator=(ValRaw&& other) -> ValRaw& {
			if (this != &other) {
				if (!other.is_valid()) {
					throw ValValueMovedException();
				}

				if (!m_DropCheck.is_null()) {
					m_DropCheck.drop(); // Ownership is consumed by this instance
					// all references to the value are now invalid
				}

				m_DropCheck = other.m_DropCheck;
				m_Value = std::move(other.m_Value);
				m_ImmutableBorrowCount = (u32)other.m_ImmutableBorrowCount;
				m_IsMutableBorrowed = (bool)other.m_IsMutableBorrowed;

				other.reset_values();
			}
			return *this;
		}

		inline auto operator=(ValRaw& other) -> ValRaw& {
			if (this != &other) {
				if (!other.is_valid()) {
					throw ValValueMovedException();
				}

				if (!m_DropCheck.is_null()) {
					m_DropCheck.drop(); // Ownership is consumed by this instance
					// all references to the value are now invalid
				}

				m_DropCheck = other.m_DropCheck;
				m_Value = std::move(other.m_Value);
				m_ImmutableBorrowCount = (u32)other.m_ImmutableBorrowCount;
				m_IsMutableBorrowed = (bool)other.m_IsMutableBorrowed;

				other.reset_values();
			}
			return *this;
		}

		inline auto operator=(const ValRaw& other) = delete;

		inline ~ValRaw() {
			drop();
		}

		inline void drop() {

			if (!is_valid()) return;

			if (!m_DropCheck.is_null()) {
				m_DropCheck.drop();
			}

			if constexpr (ThreadSafe) {
				std::lock_guard<std::mutex> lock(*m_DropCheck.get_mutex());

				if constexpr (IsRawPtr<Type>) {
					delete m_Value.value();
				}
				else if constexpr (IsSmartPtr<Type>) {
					m_Value.value().reset();
				}

				m_Value = std::nullopt;
			}
			else {
				if constexpr (IsRawPtr<Type>) {
					delete m_Value.value();
				}
				else if constexpr (IsSmartPtr<Type>) {
					m_Value.value().reset();
				}

				m_Value = std::nullopt;
			}

			// m_ImmutableBorrowCount = 0;
			// m_IsMutableBorrowed = false;
		}

		inline bool is_valid() const {
			return m_Value.has_value();
		}

		inline operator bool() const {
			return is_valid();
		}

		inline auto value() {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if constexpr (IsSmartPtrOrRawPtr<Type>) {
				return m_Value.value();
			}
			else if constexpr (IsRefRaw<Type>) {
				return m_Value.value().value();
			}
			else {
				return &m_Value.value();
			}
		}

		inline const auto value() const {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if constexpr (IsSmartPtrOrRawPtr<Type>) {
				return m_Value.value();
			}
			else if constexpr (IsRefRaw<Type>) {
				return m_Value.value().value();
			}
			else {
				return &m_Value.value();
			}
		}

		inline auto operator->() {
			return value();
		}

		inline const auto operator->() const {
			return value();
		}

		inline auto& operator*() {
			return *value();
		}

		inline const auto& operator*() const {
			return *value();
		}

		inline u32 num_borrows() const {
			return m_ImmutableBorrowCount;
		}

		inline bool is_mutable_borrowed() const {
			return m_IsMutableBorrowed;
		}

		inline auto clone() const {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if constexpr (IsSmartPtr<Type>) {
				return ValRaw<Type, ThreadSafe>(m_Value.value()->clone_sm());
			}
			else if constexpr (IsRawPtr<Type>) {
				return ValRaw<Type, ThreadSafe>(m_Value.value()->clone());
			}
			else {
				return ValRaw<Type, ThreadSafe>(m_Value.value());
			}
		}

		inline auto borrow() {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if (m_IsMutableBorrowed) {
				throw AlreadyBorrowedMutablyException();
			}

			m_ImmutableBorrowCount++;


			if constexpr (IsRawPtr<Type>) {
				return ValRaw< RefRaw<std::remove_pointer_t<Type>, false, ThreadSafe>, ThreadSafe>(RefRaw<std::remove_pointer_t<Type>, false, ThreadSafe>(m_Value.get(), &m_ImmutableBorrowCount, m_DropCheck));
			}
			else if constexpr (IsSmartPtr<Type>) {
				return ValRaw< RefRaw<std::remove_pointer_t<Type>, false, ThreadSafe>, ThreadSafe>(RefRaw<std::remove_pointer_t<Type>, false, ThreadSafe>(m_Value.get(), &m_ImmutableBorrowCount, m_DropCheck));
			}
			else {
				return ValRaw< RefRaw<std::remove_pointer_t<Type>, false, ThreadSafe>, ThreadSafe>(RefRaw<Type, false, ThreadSafe>(&m_Value.value(), &m_ImmutableBorrowCount, m_DropCheck));
			}
		}

		inline auto borrow_mut() {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if (m_IsMutableBorrowed) {
				throw AlreadyBorrowedMutablyException();
			}

			if (m_ImmutableBorrowCount > 0) {
				throw AlreadyBorrowedImmutablyException();
			}

			m_IsMutableBorrowed = true;

			if constexpr (IsRawPtr<Type>) {
				return ValRaw< RefRaw<std::remove_pointer_t<Type>, true, ThreadSafe>, ThreadSafe>(RefRaw<std::remove_pointer_t<Type>, true, ThreadSafe>(m_Value.get(), &m_IsMutableBorrowed, m_DropCheck));
			}
			else if constexpr (IsSmartPtr<Type>) {
				return ValRaw< RefRaw<std::remove_pointer_t<Type>, true, ThreadSafe>, ThreadSafe>(RefRaw<std::remove_pointer_t<Type>, true, ThreadSafe>(m_Value.get(), &m_IsMutableBorrowed, m_DropCheck));
			}
			else {
				return ValRaw< RefRaw<std::remove_pointer_t<Type>, true, ThreadSafe>, ThreadSafe>(RefRaw<Type, true, ThreadSafe>(&m_Value.value(), &m_IsMutableBorrowed, m_DropCheck));
			}
		}



		inline auto raw() {
			return &m_Value.value();
		}


	private:
		inline ValRaw() noexcept {
			reset_values();
		}

		template <typename Ty, bool Ts>
		static inline auto None() noexcept {
			return ValRaw<Ty, Ts>();
		}

		inline void reset_values() {
			m_Value = std::nullopt;
			m_ImmutableBorrowCount = 0;
			m_IsMutableBorrowed = false;
			m_DropCheck = ValidityChecker<ThreadSafe>();
		}

	private:
		std::optional<Type> m_Value;
		std::conditional_t<ThreadSafe, std::atomic_uint32_t, u32> m_ImmutableBorrowCount = 0;
		std::conditional_t<ThreadSafe, std::atomic_bool, bool> m_IsMutableBorrowed = false;
		ValidityChecker<ThreadSafe> m_DropCheck;

	private:

		template <typename Ty, bool Ts>
		friend class ValRaw;

		template<typename Ty, bool Ts>
		friend class OptionRaw;

		template<typename Ty, typename Er, bool Ts>
		friend class ResultRaw;
	};


	template<typename Type>
	using Val = ValRaw<Type, false>;

	template<typename Type>
	using SafeVal = ValRaw<Type, true>;

	template<typename Type, typename... Args>
	static inline auto MakeVal(Args&&... args) {
		return Val<Type>(Type(std::forward<Args>(args)...));
	}

	template<typename Type, typename... Args>
	static inline auto MakeSafeVal(Args&&... args) {
		return SafeVal<Type>(Type(std::forward<Args>(args)...));
	}


	// the exceptions
	class OptionRawIsNoneException : public std::exception
	{
	public:
		const char* what() const noexcept override
		{
			return "OptionRaw is None!";
		}
	};



	template<typename Type, bool ThreadSafe>
	class OptionRaw {
	public:


		inline ~OptionRaw() { }

		// a move constructor
		inline OptionRaw(OptionRaw&& other) {
			if (other.is_some()) {
				m_Value = std::move(other.m_Value);
			}
		}

		inline OptionRaw(const OptionRaw& other) = delete;

		inline OptionRaw(OptionRaw& other) {
			if (other.is_some()) {
				m_Value = std::move(other.m_Value);
			}
		}

		// a move assignment operator
		inline OptionRaw& operator=(OptionRaw&& other) {
			if (this != &other) {
				m_Value = std::move(other.m_Value);
			}
			return *this;
		}

		inline OptionRaw& operator=(const OptionRaw& other) = delete;

		inline OptionRaw& operator=(OptionRaw& other) {
			if (this != &other) {
				if (other.is_some()) {
					m_Value = std::move(other.m_Value);
				}
			}
			return *this;
		}

		/**
		 * Returns true if the option is a Some value.
		 */
		inline bool is_some() const { return m_Value.is_valid(); }

		/**
		* Returns true if the option is a Some and the value inside of it matches a predicate.
		*/
		inline auto is_some_and(std::function<bool(RawPtr<const std::remove_pointer_t<Type>>)> predicate) {
			if (is_some()) {
				if constexpr (IsRawPtr<Type>) {
					return predicate(m_Value.value());
				}
				else if constexpr (IsSmartPtr<Type>) {
					return predicate(m_Value.get());
				}
				else {
					return predicate(m_Value.operator->());
				}
			}
			return false;
		}


		/*
		* Returns true if the option is a None value.
		*/
		inline bool is_none() const { return !m_Value.is_valid(); }

		/*
		* Converts from OptionRaw<T> to OptionRaw<Ref<T>>.
		*
		* Examples
		* Calculates the length of an OptionRaw<String> as an OptionRaw<usize> without moving the String.
		* The map method takes the self argument by value, consuming the original, so this technique
		* uses as_ref to first take an OptionRaw to a reference to the value inside the original.
		*/
		inline auto as_ref() {
			if (is_some()) return SomeRaw(m_Value.borrow());
			else return NoneRaw<RefRaw<std::remove_pointer_t<Type>, false, ThreadSafe>, ThreadSafe >();
		}

		/*
		* Converts from OptionRaw<T> to OptionRaw<RefMut<T>>
		*/
		inline auto as_mut() {
			if (is_some()) return SomeRaw(m_Value.borrow_mut());
			else return NoneRaw< RefRaw<std::remove_pointer_t<Type>, true, ThreadSafe>, ThreadSafe>();
		}

		/*
		* Returns the contained Some value, consuming the self value.
		*
		* Panics
		* Panics if the value is a None with a custom panic message provided by msg.
		*/
		inline auto expect(const std::string& msg) {
			if (is_some()) return m_Value;
			else {
				throw std::runtime_error(msg);
			}
		}

		/*
		* Returns the contained Some value, consuming the self value.
		*
		* Because this function may panic, its use is generally discouraged.
		* Instead, prefer to check and handle the None case explicitly,
		* or call unwrap_or, unwrap_or_else, or unwrap_or_default.
		*/
		inline auto unwrap() {
			if (is_some()) return m_Value;
			else throw OptionRawIsNoneException();
		}

		/*
		* Returns the contained Some value or a provided default.
		*/
		inline auto unwrap_or(Type&& value) {
			if (is_some()) return m_Value;
			else return ValRaw<Type, ThreadSafe>(std::forward<Type>(value));
		}

		/*
		* Returns the contained Some value or computes it from a function.
		*/
		inline auto unwrap_or_else(std::function<Type()> f) {
			if (is_some()) return m_Value;
			else return ValRaw<Type, ThreadSafe>(f());
		}

		/*
		* Returns the contained Some value or a default.
		*
		* Consumes the self argument then, if Some, returns the contained value,
		* otherwise if None, returns the default value for that type.
		*/
		inline auto unwrap_or_default() {
			if (is_some()) return m_Value;
			else return ValRaw<Type, ThreadSafe>(Type());
		}

		/*
		* Maps an OptionRaw<T> to OptionRaw<U> by applying a function to a contained
		* value (if Some) or returns None (if None).
		*/
		template<typename U>
		inline auto map(std::function<U(RawPtr<const std::remove_pointer_t<Type>>)> f) {
			if (is_some()) {
				if constexpr (IsRawPtr<Type>) {
					return SomeRaw<U, ThreadSafe>(f(m_Value.value()));
				}
				else if constexpr (IsSmartPtr<Type>) {
					return SomeRaw<U, ThreadSafe>(f(m_Value.get()));
				}
				else {
					return SomeRaw<U, ThreadSafe>(f(m_Value.operator->()));
				}
			}
			return NoneRaw<U, ThreadSafe>();
		}

		/*
		* Inserts value into the option, then returns a mutable reference to it.
		*
		* If the option already contains a value, the old value is dropped.
		*/
		inline auto insert(Type&& value) {
			m_Value = ValRaw<Type, ThreadSafe>(std::forward<Type>(value));
		}

		/*
		* Takes the value out of the option, leaving a None in its place.
		*/
		inline auto take() {
			if (is_some()) {
				auto value = m_Value;
				m_Value = ValRaw<Type, ThreadSafe>::None();
				return Some(value);
			}
			else return NoneRaw<Type, ThreadSafe>();
		}

		/*
		* Maps an OptionRaw<T> to an OptionRaw<T> by cloning the contents of the option.
		*
		* Calls clone on the inner value, creating a new value of the same type.
		*
		* NOTE: For this to work the inner value must be cloneable.
		*/
		inline auto cloned() {
			if (is_some()) return Some(m_Value.clone());
			else return NoneRaw<Type, ThreadSafe>();
		}



		inline auto unsafe_ptr() const { return m_Value.value(); }

		template<typename Ty, bool Ts>
		static inline auto SomeRaw(ValRaw<Ty, Ts> value) { return OptionRaw<Ty, Ts>(value); }

		template<typename Ty, bool Ts>
		static inline auto NoneRaw() { return OptionRaw<Ty, Ts>(); }

	private:
		inline OptionRaw() noexcept { }
		inline OptionRaw(ValRaw<Type, ThreadSafe> value) : m_Value(value) {}


	private:
		ValRaw<Type, ThreadSafe> m_Value;

		template<typename U, bool Ts>
		friend class OptionRaw;
	};

	template <typename T>
	using Option = OptionRaw<T, false>;

	template <typename T>
	using SafeOption = OptionRaw<T, true>;

	template <typename T, bool ThreadSafe>
	inline auto SomeRaw(ValRaw<T, ThreadSafe> value) { return OptionRaw<T, ThreadSafe>::template SomeRaw<T, ThreadSafe>(value); }

	template <typename T, bool ThreadSafe>
	inline auto NoneRaw() { return OptionRaw<T, ThreadSafe>::template NoneRaw<T, ThreadSafe>(); } 

	template<typename T>
	inline auto None() { return OptionRaw<T, false>::template NoneRaw<T, false>(); }

	template<typename T>
	inline auto Some(Val<T> value) {
		return OptionRaw<T, false>::template SomeRaw<T, false>(value);
	}

	template<typename T>
	inline auto Some(T&& value) {
		return OptionRaw<T, false>::template SomeRaw<T, false>(ValRaw<T, false>(std::forward<T>(value)));
	}

	template<typename T>
	inline auto SafeNone() { return OptionRaw<T, true>::template NoneRaw<T, true>(); }

	template<typename T>
	inline auto SafeSome(SafeVal<T> value) {
		return OptionRaw<T, true>::template SomeRaw<T, true>(value);
	}

	template<typename Type, typename Err, bool ThreadSafe>
	class ResultRaw {
	public:


		// delete the copy constructor and copy assignment operator
		inline ResultRaw(const ResultRaw& other) = delete;

		inline ResultRaw(ResultRaw& other) {
			if (other.m_Value.is_valid()) {
				m_Value = std::move(other.m_Value);
			}
			else {
				m_Error = std::move(other.m_Error);
			}
		}

		inline ResultRaw(ResultRaw&& other) {
			if (other.m_Value.is_valid()) {
				m_Value = std::move(other.m_Value);
			}
			else {
				m_Error = std::move(other.m_Error);
			}
		}

		inline ResultRaw& operator=(ResultRaw&& other) {
			if (this != &other) {
				if (other.m_Value.is_valid()) {
					if (m_Error.is_valid()) m_Error.drop();
					m_Value = std::move(other.m_Value);
				}
				else {
					if (m_Value.is_valid()) m_Value.drop();
					m_Error = std::move(other.m_Error);
				}
			}
			return *this;
		}

		inline ResultRaw& operator=(const ResultRaw& other) = delete;

		/*
		* Returns true if the result is valid and not consumed
		*/
		inline bool is_valid() const { return m_Value.is_valid() || m_Error.is_valid(); }

		/*
		* Returns true if the result is Ok.
		*/
		inline bool is_ok() const { return m_Value.is_valid(); }

		/*
		* Returns true if the result is Ok and the value inside of it matches a predicate.
		*/
		inline bool is_ok_and(std::function<bool(RawPtr<const std::remove_pointer_t<Type>>)> f) const {
			if (is_ok()) {
				if constexpr (IsRawPtr<Type>) {
					return f(m_Value.value());
				}
				else if constexpr (IsSmartPtr<Type>) {
					return f(m_Value.get());
				}
				else {
					return f(m_Value.operator->());
				}
			}
			return false;
		}

		/*
		* Returns true if the result is Err.
		*/
		inline bool is_err() const { return m_Error.is_valid(); }

		/*
		* Returns true if the result is Err and the error inside of it matches a predicate.
		*/
		inline bool is_err_and(std::function<bool(const RawPtr<std::remove_pointer_t<Err>>)> f) const {
			if (is_err()) {
				if constexpr (IsRawPtr<Err>) {
					return f(m_Error.value());
				}
				else if constexpr (IsSmartPtr<Err>) {
					return f(m_Error.get());
				}
				else {
					return f(m_Error.operator->());
				}
			}
			return false;
		}

		/*
		* Returns OptionRaw<Ok> if the result is Ok.
		*/
		inline auto ok() {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if (is_ok()) {
				return SomeRaw<Type, ThreadSafe>(m_Value);
			}
			else {
				return NoneRaw<Type, ThreadSafe>();
			}
		}

		/*
		* Returns OptionRaw<Err> if the result is Err.
		*/
		inline auto err() {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if (is_err()) {
				return SomeRaw<Err, ThreadSafe>(m_Error);
			}
			else {
				return NoneRaw<Err, ThreadSafe>();
			}
		}


		/*
		* Converts from &ResultRaw<T, E> to ResultRaw<Ref<T>, Ref<E>>.
		* Produces a new ResultRaw, containing a reference into the original, 
		* leaving the original in place.
		*/
		inline auto as_ref() {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if (is_ok()) {
				return OkRaw<RefRaw<Type, false, ThreadSafe>, RefRaw<Err, false, ThreadSafe>, ThreadSafe>(m_Value.borrow());
			}
			else {
				return ErrRaw<RefRaw<Type, false, ThreadSafe>, RefRaw<Err, false, ThreadSafe>, ThreadSafe>(m_Error.borrow());
			}
		}


		/*
		* Converts from &mut ResultRaw<T, E> to ResultRaw<RefMut<T>, RefMut<E>>.
		* Produces a new ResultRaw, containing a mutable reference into the original,
		* leaving the original in place.
		*/
		inline auto as_mut() {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if (is_ok()) {
				return OkRaw<RefRaw<Type, true, ThreadSafe>, RefRaw<Err, true, ThreadSafe>, ThreadSafe>(m_Value.borrow_mut());
			}
			else {
				return ErrRaw<RefRaw<Type, true, ThreadSafe>, RefRaw<Err, true, ThreadSafe>, ThreadSafe>(m_Error.borrow_mut());
			}
		}


		/*
		* Maps a ResultRaw<T, E> to ResultRaw<U, E> by applying a function to a 
		* contained Ok value, leaving an Err value untouched.
		* 
		* If the result is Err, the function will not be called and Err will 
		* be cloned and returned.
		*
		* This function can be used to compose the results of two functions.
		*/
		template<typename U>
		inline auto map(std::function<U(RawPtr<const std::remove_pointer_t<Type>>)> f) {
			if (!is_valid()) {
				throw ValValueMovedException();
			}

			if (is_ok()) {

				if constexpr (IsRawPtr<Type>) {
					return OkRaw<U, Err, ThreadSafe>(f(m_Value.value()));
				}
				else if constexpr (IsSmartPtr<Type>) {
					return OkRaw<U, Err, ThreadSafe>(f(m_Value.get()));
				}
				else {
					return OkRaw<U, Err, ThreadSafe>(f(m_Value.operator->()));
				}

			}
			else {
				return ErrRaw<U, Err, ThreadSafe>(m_Error.clone());
			}
		}


        /*
        * Maps a ResultRaw<T, E> to ResultRaw<T, F> by applying a function to a
        * contained Err value, leaving an Ok value untouched.
        * 
        * If the result is Ok, the function will not be called and Ok will
        * be cloned and returned.
        * 
        * This function can be used to pass through a successful result while handling an error.
        */
        template<typename U>
        inline auto map_err(std::function<U(const RawPtr<std::remove_pointer_t<Err>>)> f) {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_err()) {

                if constexpr (IsRawPtr<Err>) {
                    return ErrRaw<Type, U, ThreadSafe>(f(m_Error.value()));
                }
                else if constexpr (IsSmartPtr<Err>) {
                    return ErrRaw<Type, U, ThreadSafe>(f(m_Error.get()));
                }
                else {
                    return ErrRaw<Type, U, ThreadSafe>(f(m_Error.operator->()));
                }

            }
            else {
                return OkRaw<Type, U, ThreadSafe>(m_Value.clone());
            }
        }

        /*
        * Returns the contained Ok value, consuming the self value.
        *
        * Panics
        * Panics if the value is an Err with a custom panic message provided by msg.
        */
        inline auto expect(const std::string& msg) {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_ok()) {
                return m_Value;
            }
            else {
                throw std::runtime_error(msg);
            }
        }

        /*
        * Returns the contained Ok value, consuming the self value.
        *
        * Because this function may panic, its use is generally discouraged.
        * Instead, prefer to check and handle the Err case explicitly,
        * or call unwrap_or, unwrap_or_else, or unwrap_or_default.
        */
        inline auto unwrap() {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_ok()) {
                return m_Value;
            }
            else {
                throw OptionRawIsNoneException();
            }
        }

        /*  
        * Returns the contained Ok value or a provided default.
        */
        inline auto unwrap_or(Type&& value) {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_ok()) {
                return m_Value;
            }
            else {
                return ValRaw<Type, ThreadSafe>(std::forward<Type>(value));
            }
        }

        /*
        * Returns the contained Ok value or default
        */
        inline auto unwrap_or_default() {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_ok()) {
                return m_Value;
            }
            else {
                return ValRaw<Type, ThreadSafe>(Type());
            }
        }

        /*
        * Returns the contained Ok value or computes it from a function.
        */
        inline auto expect_err(const std::string& msg) {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_err()) {
                return m_Error;
            }
            else {
                throw std::runtime_error(msg);
            }
        }

        /*
        * Returns the contained Err value or a provided default.
        */
        inline auto unwrap_err() {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_err()) {
                return m_Error;
            }
            else {
                throw OptionRawIsNoneException();
            }
        }

        /* 
        * Returns a result with internal value/error cloned
        */
        inline auto cloned() {
            if (!is_valid()) {
                throw ValValueMovedException();
            }

            if (is_ok()) {
                return OkRaw(m_Value.clone());
            }
            else {
                return ErrRaw(m_Error.clone());
            }
        }

		inline auto unsafe_ptr() const { return m_Value.value(); }
		inline auto unsafe_error_ptr() const { return m_Error.value(); }

		template<typename Ty, typename Er, bool Ts> 
		static inline auto OkRaw(ValRaw<Ty, Ts>&& value) {
			return ResultRaw<Ty, Er, Ts>( std::move(value), ValRaw<Er, Ts>::template None<Er, Ts>());
		}

		template<typename Ty, typename Er, bool Ts>
		static inline auto ErrRaw(ValRaw<Er, Ts>&& error) {
			return ResultRaw<Ty, Er, Ts>( ValRaw<Ty, Ts>::template None<Ty, Ts>(), std::move(error));
		}




	private:
		inline ResultRaw(ValRaw<Type, ThreadSafe> value, ValRaw<Err, ThreadSafe> error)
		{
			if (value.is_valid())
			{
				m_Value = std::move(value);
			}
			else {
				m_Error = std::move(error);
			}
		}

	private:
		ValRaw<Type, ThreadSafe> m_Value;
		ValRaw<Err, ThreadSafe> m_Error;

		template<typename U, typename E, bool Ts>
		friend class ResultRaw;
	};

	template<typename T, typename E>
	using Result = ResultRaw<T, E, false>;

	template<typename T, typename E>
	using ResultThreadSafe = ResultRaw<T, E, true>;

	template<typename T, typename E, bool Ts>
	inline auto OkRaw(ValRaw<T, Ts>&& value) {
		return ResultRaw<T, E, Ts>::template OkRaw<T, E, Ts>(std::move(value));
	}

	template<typename T, typename E, bool Ts>
	inline auto ErrRaw(ValRaw<E, Ts>&& error) {
		return ResultRaw<T, E, Ts>::template ErrRaw<T, E, Ts>(std::move(error));
	}

	template<typename T, typename E>
	inline auto Ok(T&& value) {
		return ResultRaw<T, E, false>::template OkRaw<T, E, false>(ValRaw<T, false>(std::forward<T>(value)));
	}

	template <typename T, typename E>
	inline auto Err(E&& error) {
		return ResultRaw<T, E, false>::template ErrRaw<T, E, false>(ValRaw<E, false>(std::forward<E>(error)));
	}

	template<typename T, typename E>
	inline auto SafeOk(T&& value) {
		return ResultRaw<T, E, true>::template OkRaw<T, E, true>(ValRaw<T, true>(std::forward<T>(value)));
	}

	template<typename T, typename E>
	inline auto SafeErr(E&& error) {
		return ResultRaw<T, E, true>::template ErrRaw<T, E, true>(ValRaw<E, true>(std::forward<E>(error)));
	}

}

// the print proxy for all the types
namespace rs {

	template<bool ThreadSafe>
	inline auto operator<<(std::ostream& os, const ValidityCheckBlock<ThreadSafe>& a) -> std::ostream& {
		return os << typeid(a).name() << " { is_valid: " << a.is_valid() << " }";
	}

	template<bool ThreadSafe>
	inline auto operator<<(std::ostream& os, const ValidityChecker<ThreadSafe>& a) -> std::ostream& {
		if (a.is_null()) {
			return os << typeid(a).name() << " { is_null: true }";
		}
		return os << typeid(a).name() << " { is_valid: " << a.is_valid() << ", ref_count: " << a.ref_count() << " }";
	}

	template<typename Type, bool ThreadSafe>
	inline auto operator<<(std::ostream& os, const ValRaw<Type, ThreadSafe>& a) -> std::ostream& {
		if (!a.is_valid()) {
			return os << typeid(a).name() << " { is_valid: false }";
		}
		return os << typeid(a).name() << " { value: " << *a.value() << " }";
	}

	template<typename Type, bool Mutable, bool ThreadSafe>
	inline auto operator<<(std::ostream& os, const RefRaw<Type, Mutable, ThreadSafe>& a) -> std::ostream& {
		if (!a.is_valid()) {
			return os << typeid(a).name() << " { is_valid: false }";
		}
		return os << typeid(a).name() << " { value: " << *a.value() << " }";
	}

	template<typename Type, bool ThreadSafe>
	inline auto operator<<(std::ostream& os, const OptionRaw<Type, ThreadSafe>& a) -> std::ostream& {
		if (a.is_some()) {
			return os << typeid(a).name() << " { is_some: true, value: " << *a.unsafe_ptr() << " }";
		}
		return os << typeid(a).name() << " { is_some: false }";
	}

	template<typename Type, typename Err, bool ThreadSafe>
	inline auto operator<<(std::ostream& os, const ResultRaw<Type, Err, ThreadSafe>& a) -> std::ostream& {
		if (a.is_ok()) {
			return os << typeid(a).name() << " { Ok, value: " << *a.unsafe_ptr() << " }";
		}
		return os << typeid(a).name() << " { Err, error: " << *a.unsafe_error_ptr() << " }";
	}
}

#ifdef RS_EXPORT

namespace std {
	template<bool ThreadSafe>
	struct formatter<rs::ValidityCheckBlock<ThreadSafe>>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
		template<typename FormatContext>
		auto format(const rs::ValidityCheckBlock<ThreadSafe>& value, FormatContext& ctx) const
		{
			auto ss = std::stringstream();
			ss << value;
			return format_to(ctx.out(), "{}", ss.str());
		}
	};

	template<bool ThreadSafe>
	struct formatter<rs::ValidityChecker<ThreadSafe>>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
		template<typename FormatContext>
		auto format(const rs::ValidityChecker<ThreadSafe>& value, FormatContext& ctx) const
		{
			auto ss = std::stringstream();
			ss << value;
			return format_to(ctx.out(), "{}", ss.str());
		}
	};

	template<typename Type, bool ThreadSafe>
	struct formatter<rs::ValRaw<Type, ThreadSafe>>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
		template<typename FormatContext>
		auto format(const rs::ValRaw<Type, ThreadSafe>& value, FormatContext& ctx) const
		{
			auto ss = std::stringstream();
			ss << value;
			return format_to(ctx.out(), "{}", ss.str());
		}
	};

	template<typename Type, bool Mutable, bool ThreadSafe>
	struct formatter<rs::RefRaw<Type, Mutable, ThreadSafe>>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
		template<typename FormatContext>
		auto format(const rs::RefRaw<Type, Mutable, ThreadSafe>& value, FormatContext& ctx) const
		{
			auto ss = std::stringstream();
			ss << value;
			return format_to(ctx.out(), "{}", ss.str());
		}
	};

	template<typename Type, bool ThreadSafe>
	struct formatter<rs::OptionRaw<Type, ThreadSafe>>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
		template<typename FormatContext>
		auto format(const rs::OptionRaw<Type, ThreadSafe>& value, FormatContext& ctx) const
		{
			auto ss = std::stringstream();
			ss << value;
			return format_to(ctx.out(), "{}", ss.str());
		}
	};

	template<typename Type, typename Err, bool ThreadSafe>
	struct formatter<rs::ResultRaw<Type, Err, ThreadSafe>>
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }
		template<typename FormatContext>
		auto format(const rs::ResultRaw<Type, Err, ThreadSafe>& value, FormatContext& ctx) const
		{
			auto ss = std::stringstream();
			ss << value;
			return format_to(ctx.out(), "{}", ss.str());
		}
	};
}

#endif