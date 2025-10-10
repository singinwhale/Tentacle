// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Container/BindingSubscriptionList.h"

namespace DI
{
	void FBindingSubscriptionList::NotifyInstanceBound(const DI::FBinding& Binding)
	{
		FOnInstanceBound Subscriptions;
		if (!BindingToSubscriptions.RemoveAndCopyValue(Binding.GetId(), Subscriptions))
			return;

		Subscriptions.Broadcast(Binding);
	}

	auto FBindingSubscriptionList::SubscribeOnce(const FBindingId& BindingId) -> FOnInstanceBound&
	{
		return BindingToSubscriptions.FindOrAdd(BindingId);
	}

	TArray<FBindingId> FBindingSubscriptionList::GetAllPendingBindingIds() const
	{
		TArray<FBindingId> OutIds;
		BindingToSubscriptions.GetKeys(OutIds);
		return OutIds;
	}

	bool FBindingSubscriptionList::Unsubscribe(const FBindingId& BindingId, FDelegateHandle DelegateHandle)
	{
		FOnInstanceBound* Subscriptions = BindingToSubscriptions.Find(BindingId);
		if (!Subscriptions)
			return false;

		return Subscriptions->Remove(DelegateHandle);
	}
}
