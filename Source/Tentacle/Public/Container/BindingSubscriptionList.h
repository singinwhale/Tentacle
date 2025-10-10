// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Binding.h"
#include "BindingId.h"

namespace DI
{
	/**
	 * Keeps the list of pending subscribers per binding ID.
	 */
	class TENTACLE_API FBindingSubscriptionList
	{
	public:
		using FOnInstanceBound = TMulticastDelegate<void(const DI::FBinding&)>;
		using FOnInstanceBoundUnicast = FOnInstanceBound::FDelegate;

		bool Unsubscribe(const FBindingId& BindingId, FDelegateHandle DelegateHandle);

		void NotifyInstanceBound(const DI::FBinding& Binding);

		FOnInstanceBound& SubscribeOnce(const FBindingId& BindingId);
		TArray<FBindingId> GetAllPendingBindingIds() const;

	private:
		TMap<FBindingId, FOnInstanceBound> BindingToSubscriptions = {};
	};
}
