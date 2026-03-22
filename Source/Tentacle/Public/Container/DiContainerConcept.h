// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "BindingSubscriptionList.h"
#include "Binding.h"
#include "Templates/Models.h"

namespace DI
{
	struct CTypeHasBindSpecific
	{
		template <class TDiContainer>
		auto Requires(TDiContainer& DiContainer,
		              TSharedRef<DI::FBinding> SpecificBinding,
		              EBindConflictBehavior ConflictBehavior) -> decltype(
			DiContainer.BindSpecific(SpecificBinding, ConflictBehavior)
		);
	};

	struct CTypeHasFindBinding
	{
		template <class TDiContainer>
		auto Requires(TDiContainer& DiContainer,
		              const FBindingId& BindingId) -> decltype(
			DiContainer.FindBinding(BindingId)
		);
	};

	struct CTypeHasSubscribe
	{
		template <class TDiContainer>
		auto Requires(const TDiContainer& DiContainer,
		              const FBindingId& BindingId) -> decltype(
			DiContainer.Subscribe(BindingId)
		);
	};

	struct CDiContainer
	{
		template <class TDiContainer>
		auto Requires(TDiContainer& DiContainer) -> decltype(
			Refines<CTypeHasBindSpecific, TDiContainer>(),
			Refines<CTypeHasFindBinding, TDiContainer>(),
			Refines<CTypeHasSubscribe, TDiContainer>()
		);
	};

	namespace Private
	{
		// PS5 is missing <concepts> C++20 library.
		// Backport std::convertible_to manually.
		template <class From, class To>
		concept convertible_to =
			std::is_convertible_v<From, To> &&
			requires
			{
				static_cast<To>(std::declval<From>());
			};
	}

	template <class T>
	concept DiContainerConcept = requires(T DiContainer)
	{
		{ DiContainer.BindSpecific(DeclVal<TSharedRef<DI::FBinding>>(), DeclVal<EBindConflictBehavior>()) } -> Private::convertible_to<EBindResult>;
		{ DiContainer.FindBinding(DeclVal<const FBindingId&>()) } -> Private::convertible_to<TSharedPtr<DI::FBinding>>;
		{ DiContainer.Subscribe(DeclVal<const FBindingId&>()) } -> Private::convertible_to<FBindingSubscriptionList::FOnInstanceBound&>;
	};
}
