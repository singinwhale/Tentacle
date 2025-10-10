// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "BindConflictBehavior.h"
#include "BindResult.h"
#include "DiContainerConcept.h"

namespace DI
{
	/**
	 * Virtual base so we can abstract over DiContainers
	 */
	class TENTACLE_API FDiContainerBase
	{
	public:
		FDiContainerBase() = default;
		virtual ~FDiContainerBase() = default;


		// - DiContainerConcept
		/** Bind a specific binding. */
		virtual EBindResult BindSpecific(TSharedRef<DI::FBinding> SpecificBinding, EBindConflictBehavior ConflictBehavior) = 0;
		/** Find a binding by its ID. */
		virtual TSharedPtr<DI::FBinding> FindBinding(const FBindingId& BindingId) const = 0;

		/**
		 * Get the delegate that will be invoked a single time when the binding with the given ID is bound.
		 * If the binding is already bound the event will never fire.
		 * @param BindingId the ID of the binding to be notified about.
		 */
		virtual FBindingSubscriptionList::FOnInstanceBound& Subscribe(const FBindingId& BindingId) const = 0;
		// --
	};

	/**
	 * Virtual base so we can abstract over DiContainers
	 */
	class TENTACLE_API FConnectedDiContainer
	{
	public:
		FConnectedDiContainer() = default;
		virtual ~FConnectedDiContainer() = default;

		/**
		 * @return true if the connection has been established successfully, false otherwise.
		 * @note Implementers should log an error with further information.
		 * @note The parent should call RetryAllPendingWaits if there are already any instances bound. 
		 */
		virtual bool TryConnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer) = 0;

		/**
		 * @return true if the connection has been removed successfully, false otherwise.
		 * @note Implementers should log an error with further information.
		 */
		virtual bool TryDisconnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer) = 0;
		/**
		 * Notifies this connected container that a new binding has been bound in the parent container.
		 * @param NewBinding - the new binding
		 */
		virtual void NotifyInstanceBound(const DI::FBinding& NewBinding) const = 0;

		/**
		 * Requests this container to reevaluate all pending bindings in case they have become available through adding a parent container.
		 * After this operation the container and its children should not have any more pending waits for already bound bindings.
		 */
		virtual void RetryAllPendingWaits() const = 0;

		/**
		 * Try to find a binding in this container.
		 * @param BindingId - the binding to look for
		 * @return the binding if it has been found, nullptr otherwise.
		 */
		virtual TSharedPtr<DI::FBinding> FindConnectedBinding(const DI::FBindingId& BindingId) const = 0;
	};
}
