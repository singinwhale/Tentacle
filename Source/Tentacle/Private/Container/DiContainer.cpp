// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Container/DiContainer.h"

namespace DI
{
	bool FDiContainer::Unsubscribe(const FBindingId& BindingId, FDelegateHandle DelegateHandle) const
	{
		return Subscriptions.Unsubscribe(BindingId, DelegateHandle);
	}

	void FDiContainer::AddReferencedObjects(FReferenceCollector& Collector)
	{
		for (auto& [BindingId, Binding] : Bindings)
		{
			Binding->AddReferencedObjects(Collector);
		}
	}

	TSharedPtr<DI::FBinding> FDiContainer::FindBinding(const FBindingId& BindingId) const
	{
		if (const TSharedRef<DI::FBinding>* DependencyBinding = Bindings.Find(BindingId))
		{
			if ((*DependencyBinding)->IsValid())
			{
				return *DependencyBinding;
			}
		}
		return nullptr;
	}

	FBindingSubscriptionList::FOnInstanceBound& FDiContainer::Subscribe(const FBindingId& BindingId) const
	{
		return Subscriptions.SubscribeOnce(BindingId);
	}

	TBindingHelper<FDiContainer> FDiContainer::Bind()
	{
		return TBindingHelper<FDiContainer>(*this);
	}

	TResolveHelper<FDiContainer> FDiContainer::Resolve() const
	{
		return TResolveHelper<FDiContainer>(*this);
	}

	TInjector<FDiContainer> FDiContainer::Inject()
	{
		return TInjector<FDiContainer>(*this);
	}

	EBindResult FDiContainer::BindSpecific(
		TSharedRef<DI::FBinding> SpecificBinding,
		EBindConflictBehavior ConflictBehavior)
	{
		FBindingId BindingId = SpecificBinding->GetId();
		if (TSharedRef<FBinding>* Binding = Bindings.Find(BindingId))
		{
			if ((*Binding)->IsValid())
			{
				HandleBindingConflict(BindingId, ConflictBehavior);
				return EBindResult::Conflict;
			}
		}
		Bindings.Emplace(BindingId, SpecificBinding);
		Subscriptions.NotifyInstanceBound(*SpecificBinding);
		return EBindResult::Bound;
	}
}
