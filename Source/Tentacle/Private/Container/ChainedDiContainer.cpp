// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Container/ChainedDiContainer.h"

void DI::FChainedDiContainer::SetParentContainer(TSharedPtr<FConnectedDiContainer> DiContainer)
{
	if (ParentContainer == DiContainer)
		return;

	if (TSharedPtr<FConnectedDiContainer> PinnedParent = ParentContainer.Pin())
	{
		if (!PinnedParent->TryDisconnectSubcontainer(AsShared()))
		{
			UE_LOG(LogDependencyInjection, Warning, TEXT("FChainedDiContainer::SetParentContainer: Failed to disconnect from parent container."));
		}
	}

	ParentContainer = DiContainer;

	if (DiContainer && !DiContainer->TryConnectSubcontainer(AsShared()))
	{
		UE_LOG(LogDependencyInjection, Error, TEXT("FChainedDiContainer::SetParentContainer: Failed to connect to new parent container."));
	}
}

void DI::FChainedDiContainer::AddReferencedObjects(FReferenceCollector& Collector)
{
	for (auto& [BindingId, Binding] : Bindings)
	{
		Binding->AddReferencedObjects(Collector);
	}
}

bool DI::FChainedDiContainer::TryConnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer)
{
	ChildrenContainers.AddUnique(ConnectedDiContainer);
	ConnectedDiContainer->RetryAllPendingWaits();
	return true;
}

bool DI::FChainedDiContainer::TryDisconnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer)
{
	return ChildrenContainers.Remove(ConnectedDiContainer) > 0;
}

void DI::FChainedDiContainer::NotifyInstanceBound(const DI::FBinding& NewBinding) const
{
	Subscriptions.NotifyInstanceBound(NewBinding);
	for (auto ChildrenContainerIt = ChildrenContainers.CreateIterator(); ChildrenContainerIt; ++ChildrenContainerIt)
	{
		TSharedPtr<FConnectedDiContainer> ChainedDiContainer = ChildrenContainerIt->Pin();
		if (!ChainedDiContainer.IsValid())
		{
			ChildrenContainerIt.RemoveCurrent();
			continue;
		}

		ChainedDiContainer->NotifyInstanceBound(NewBinding);
	}
}

void DI::FChainedDiContainer::RetryAllPendingWaits() const
{
	TArray<FBindingId> BindingIds = Subscriptions.GetAllPendingBindingIds();
	for (const FBindingId& BindingId : BindingIds)
	{
		if (TSharedPtr<DI::FBinding> Binding = FindBinding(BindingId))
		{
			Subscriptions.NotifyInstanceBound(*Binding);
		}
	}

	for (auto ChildrenContainerIt = ChildrenContainers.CreateIterator(); ChildrenContainerIt; ++ChildrenContainerIt)
	{
		TSharedPtr<FConnectedDiContainer> ChildContainer = ChildrenContainerIt->Pin();
		if (!ChildContainer.IsValid())
		{
			ChildrenContainerIt.RemoveCurrent();
			continue;
		}

		ChildContainer->RetryAllPendingWaits();
	}
}

TSharedPtr<DI::FBinding> DI::FChainedDiContainer::FindConnectedBinding(const DI::FBindingId& BindingId) const
{
	return FindBinding(BindingId);
}

DI::EBindResult DI::FChainedDiContainer::BindSpecific(TSharedRef<FBinding> SpecificBinding, EBindConflictBehavior ConflictBehavior)
{
	EBindResult OverallResult = EBindResult::Bound;
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
	NotifyInstanceBound(*SpecificBinding);
	return OverallResult;
}

TSharedPtr<DI::FBinding> DI::FChainedDiContainer::FindBinding(const FBindingId& BindingId) const
{
	if (const TSharedRef<FBinding>* DependencyBinding = Bindings.Find(BindingId))
	{
		if ((*DependencyBinding)->IsValid())
		{
			return *DependencyBinding;
		}
	}

	if (TSharedPtr<FConnectedDiContainer> ParentDiContainer = ParentContainer.Pin())
	{
		return ParentDiContainer->FindConnectedBinding(BindingId);
	}
	return {};
}

DI::FBindingSubscriptionList::FOnInstanceBound& DI::FChainedDiContainer::Subscribe(
	const FBindingId& BindingId) const
{
	return Subscriptions.SubscribeOnce(BindingId);
}

bool DI::FChainedDiContainer::Unsubscribe(const FBindingId& BindingId, FDelegateHandle DelegateHandle) const
{
	return Subscriptions.Unsubscribe(BindingId, DelegateHandle);
}

void FChainedDiContainerGCd::AddStructReferencedObjects(FReferenceCollector& Collector)
{
	DiContainer->AddReferencedObjects(Collector);
}