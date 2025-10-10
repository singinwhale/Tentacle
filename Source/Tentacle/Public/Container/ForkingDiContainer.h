// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "DiContainer.h"
#include "DiContainerBase.h"

namespace DI
{
	/**
	 * Virtual DI Container that can defer resolving of bindings to multiple parent.
	 *
	 * This DI Container *cannot* contain bindings of its own!
	 *
	 * Connect it to a FChainedDiContainer to store bindings.
	 */
	class TENTACLE_API FForkingDiContainer final
		: public TSharedFromThis<FForkingDiContainer>
		  , public FConnectedDiContainer
	{
	public:
		FForkingDiContainer() = default;

		// Technically, we could have a copy constructor, but copying is usually a user error, so we delete it to catch these cases earlier.
		FForkingDiContainer(const FForkingDiContainer&) = delete;

		virtual ~FForkingDiContainer() override = default;

		/**
		 * Add a parent to the chain.
		 * Higher priority containers will be checked first.
		 * Adding the same parent twice will cause the priority to be changed to the last added priority.
		 * @warning This has to happen before any async resolves are performed.
		 */
		void AddParentContainer(TSharedRef<FConnectedDiContainer> DiContainer, int32 Priority = 0);

		/**
		 * Remove a parent DiContainer.
		 */
		void RemoveParentContainer(TWeakPtr<FConnectedDiContainer> DiContainer);

	private:
		// - FConnectedDiContainer
		virtual bool TryConnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer) override;
		virtual bool TryDisconnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer) override;
		virtual void NotifyInstanceBound(const DI::FBinding& NewBinding) const override;
		virtual void RetryAllPendingWaits() const override;
		virtual TSharedPtr<DI::FBinding> FindConnectedBinding(const DI::FBindingId& BindingId) const override;
		// --

		/**
		 * Higher priority containers will be checked first.
		 * mutable so we can use clear up dead parents in const methods
		 */
		mutable TArray<TPair<int32, TWeakPtr<FConnectedDiContainer>>, TInlineAllocator<4>> ParentContainers;

		// Mutable so we can clean up invalid children in getters
		mutable TArray<TWeakPtr<FConnectedDiContainer>, TInlineAllocator<1>> ChildrenContainers;
	};
}
