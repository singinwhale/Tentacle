// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

namespace FunctionTraits
{
	template <typename T>
	struct TFunctionTraits : TFunctionTraits<decltype(&T::operator())> {};

	// Member function pointer (non-const)
	template <typename C, typename R, typename... Args>
	struct TFunctionTraits<R (C::*)(Args...)>
	{
		using ResultType = R;
		using ArgsTuple  = TTuple<Args...>;
	};

	// Member function pointer (const)
	template <typename C, typename R, typename... Args>
	struct TFunctionTraits<R (C::*)(Args...) const>
	{
		using ResultType = R;
		using ArgsTuple  = TTuple<Args...>;
	};

	// Free function type
	template <typename R, typename... Args>
	struct TFunctionTraits<R(Args...)>
	{
		using ResultType = R;
		using ArgsTuple  = TTuple<Args...>;
	};

	// Free function pointer
	template <typename R, typename... Args>
	struct TFunctionTraits<R(*)(Args...)> : TFunctionTraits<R(Args...)> {};

	// UE's TFunction
	template <typename R, typename... Args>
	struct TFunctionTraits<TFunction<R(Args...)>> : TFunctionTraits<R(Args...)> {};
}