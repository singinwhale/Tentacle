// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "DiContainer.h"
#include "ChainedDiContainer.generated.h"

namespace DI
{
	/**
	 * DI Container that can defer resolving of bindings to its single parent.
	 *
	 * Binding will cause the container to notify its children that a new binding has been bound.
	 * This behavior to prevent the memory overhead of duplicate bindings in favor of worse performance at bind and resolve time.
	 *
	 * Children do not cache bindings that have been bound in parent containers.
	 */
	class TENTACLE_API FChainedDiContainer final
		: public TSharedFromThis<FChainedDiContainer>
		  , public FConnectedDiContainer
		  , public FDiContainerBase
	{
	public:
		FChainedDiContainer() = default;

		// Technically, we could have a copy constructor, but copying is usually a user error, so we delete it to catch these cases earlier.
		FChainedDiContainer(const FChainedDiContainer&) = delete;

		virtual ~FChainedDiContainer() override = default;

		/**
		 * Sets the chained parent of this DI Container.
		 * @warning This has to happen before any async resolves are performed.
		 */
		void SetParentContainer(TSharedPtr<FConnectedDiContainer> DiContainer);

		/** Call this from the owning type to prevent types and bindings to be garbage collected. */
		void AddReferencedObjects(FReferenceCollector& Collector);

		/** Get the Binding API */
		TBindingHelper<FChainedDiContainer> Bind() { return TBindingHelper(*this); }
		/** Get the Resolving API */
		TResolveHelper<FChainedDiContainer> Resolve() const { return TResolveHelper(*this); };
		/** Get the Injection API */
		TInjector<FChainedDiContainer> Inject() { return TInjector(*this); };

		// - FDiContainerBase
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

	private:
		// - FConnectedDiContainer
		virtual bool TryConnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer) override;
		virtual bool TryDisconnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer) override;
		virtual void NotifyInstanceBound(const DI::FBinding& NewBinding) const override;
		virtual void RetryAllPendingWaits() const override;
		virtual TSharedPtr<DI::FBinding> FindConnectedBinding(const DI::FBindingId& BindingId) const override;
		// --

		/** Our own registered Bindings */
		TMap<FBindingId, TSharedRef<DI::FBinding>> Bindings = {};

		// mutable so we can use it in const resolve methods
		mutable FBindingSubscriptionList Subscriptions;

		TWeakPtr<FConnectedDiContainer> ParentContainer;

		// Mutable so we can clean up invalid children in getters
		mutable TArray<TWeakPtr<FConnectedDiContainer>, TInlineAllocator<1>> ChildrenContainers;
	};

	static_assert(TModels<CDiContainer, FChainedDiContainer>::Value);
	static_assert(TModels<CTypeHasBindSpecific, FChainedDiContainer>::Value);
	static_assert(TModels<CTypeHasFindBinding, FChainedDiContainer>::Value);
	static_assert(TModels<CTypeHasSubscribe, FChainedDiContainer>::Value);
	static_assert(DiContainerConcept<FChainedDiContainer>);
}


/**
 * Helper struct that makes it more convenient to implement DI Contexts by being GC collectable by default if used as UPROPERTY.
 */
USTRUCT(NotBlueprintType)
struct FChainedDiContainerGCd
{
	GENERATED_BODY()

public:
	FChainedDiContainerGCd() = default;
	void AddStructReferencedObjects(FReferenceCollector& Collector);

	// Define a bunch of convenience operators to make the code cleaner.
	DI::FChainedDiContainer& operator*() { return *DiContainer; }
	const DI::FChainedDiContainer& operator*() const { return *DiContainer; }
	const DI::FChainedDiContainer* operator->() const { return &*DiContainer; }
	DI::FChainedDiContainer* operator->() { return &*DiContainer; }
	operator const TSharedRef<DI::FChainedDiContainer>() const { return DiContainer; }
	operator TSharedRef<DI::FChainedDiContainer>() { return DiContainer; }
	operator const TSharedPtr<DI::FChainedDiContainer>() const { return DiContainer; }
	operator TSharedPtr<DI::FChainedDiContainer>() { return DiContainer; }
	// --

	TSharedRef<DI::FChainedDiContainer> DiContainer = MakeShared<DI::FChainedDiContainer>();
};

template<>
struct TStructOpsTypeTraits<FChainedDiContainerGCd> : public TStructOpsTypeTraitsBase2<FChainedDiContainerGCd>
{
	enum
	{
		WithAddStructReferencedObjects = true,
	};
};
