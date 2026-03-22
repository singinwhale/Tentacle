// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "BindConflictBehavior.h"
#include "BindingHelper.h"
#include "BindingSubscriptionList.h"
#include "BindResult.h"
#include "Binding.h"
#include "BindingId.h"
#include "DiContainerBase.h"
#include "DiContainerConcept.h"
#include "Injector.h"
#include "ResolveHelper.h"

namespace DI
{
	/**
	 * Simple Di Container that does everything locally.
	 */
	class TENTACLE_API FDiContainer final : public FDiContainerBase
	{
	public:
		// - DiContainerConcept
		/** Bind a specific binding. */
		virtual EBindResult BindSpecific(TSharedRef<DI::FBinding> SpecificBinding, EBindConflictBehavior ConflictBehavior) override;

		/** Find a binding by its ID. */
		virtual TSharedPtr<DI::FBinding> FindBinding(const FBindingId& BindingId) const override;

		/**
		 * Get the delegate that will be invoked a single time when the binding with the given ID is bound.
		 * If the binding is already bound the event will never fire.
		 * @param BindingId the ID of the binding to be notified about.
		 */
		virtual FBindingSubscriptionList::FOnInstanceBound& Subscribe(const FBindingId& BindingId) const override;
		// --

		/**
		 * Unsubscribe from being notified about a binding.
		 * @param BindingId The ID of the binding where there is a subscription
		 * @param DelegateHandle The handle that was returned when the subscription was created
		 * @return true if there was a subscription and it has been successfully removed.
		 */
		bool Unsubscribe(const FBindingId& BindingId, FDelegateHandle DelegateHandle) const;

		/** Call this from the owning type to prevent types and bindings to be garbage collected. */
		void AddReferencedObjects(FReferenceCollector& Collector);

		/** Get the Binding API */
		TBindingHelper<FDiContainer> Bind();
		/** Get the Resolving API */
		TResolveHelper<FDiContainer> Resolve() const;
		/** Get the Injection API */
		TInjector<FDiContainer> Inject();
	protected:
		TMap<FBindingId, TSharedRef<DI::FBinding>> Bindings = {};
		mutable FBindingSubscriptionList Subscriptions;
	};

	static_assert(TModels<CDiContainer, FDiContainer>::Value);
	static_assert(TModels<CTypeHasBindSpecific, FDiContainer>::Value);
	static_assert(TModels<CTypeHasFindBinding, FDiContainer>::Value);
	static_assert(TModels<CTypeHasSubscribe, FDiContainer>::Value);
	static_assert(DiContainerConcept<FDiContainer>);
}
