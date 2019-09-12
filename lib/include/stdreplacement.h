#pragma once

// replacement header for some std functions and traits

#include <stddef.h>

#ifndef DONT_REPLACE_PLACEMENTNEW
inline void* operator new(size_t size, void* ptr) noexcept { (void)size; return ptr; }
#else
#include <new>
#endif

//#include <type_traits>

#ifdef DONT_REPLACE_TRAITS
#include <type_traits>
#include <utility>
namespace spvgentwo::std
{
	using namespace ::std;
}
#else
namespace spvgentwo::std
{
	template <class>
	inline constexpr bool is_lvalue_reference_v = false;

	template <class T>
	inline constexpr bool is_lvalue_reference_v<T&> = true;

	template <class T>
	struct remove_reference { using type = T;};
	template <class T>
	struct remove_reference<T&> {using type = T;};
	template <class T>
	struct remove_reference<T&&> { using type = T;};

	template <class T>
	using remove_reference_t = typename remove_reference<T>::type;

	template <class T>
	[[nodiscard]] constexpr T&& forward(remove_reference_t<T>& _Arg) noexcept {
		return static_cast<T&&>(_Arg);
	}

	template <class T>
	[[nodiscard]] constexpr T&& forward(remove_reference_t<T>&& _Arg) noexcept { 
		static_assert(!is_lvalue_reference_v<T>, "forward of lvalue");
		return static_cast<T&&>(_Arg);		
	}

	template <class, class>
	inline constexpr bool is_same_v = false;
	template <class T>
	inline constexpr bool is_same_v<T, T> = true;

	template <class T>
	struct remove_cv { using type = T; };
	template <class T>
	struct remove_cv<const T> { using type = T; };
	template <class T>
	struct remove_cv<volatile T> { using type = T; };
	template <class T>
	struct remove_cv<const volatile T> { using type = T; };
	template <class T>
	using remove_cv_t = typename remove_cv<T>::type;
} // !spvgentwo::std
#endif

// custom traits
namespace spvgentwo
{
	// cpp20
	template <class T>
	using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
	template <class T>
	struct remove_cvref { using type = remove_cvref_t<T>; };

	template <class T, class BASE>
	inline constexpr bool is_same_base_type_v = std::is_same_v<remove_cvref_t<T>, BASE>;
} // !spvgentwo