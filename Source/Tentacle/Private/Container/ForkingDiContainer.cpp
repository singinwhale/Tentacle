// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Container/ForkingDiContainer.h"

void DI::FForkingDiContainer::AddParentContainer(TSharedRef<FConnectedDiContainer> DiContainer, int32 Priority)
{
	// Remove all existing instances disregarding priority.
	// This will cause the priority to be "overwritten" if you add the same DiContainer with a different priority.
	ParentContainers.RemoveAll([DiContainer](const auto& PrioritizedParent)
	{
		return PrioritizedParent.Value == DiContainer;
	});

	ParentContainers.Add({Priority, MoveTemp(DiContainer)});
	ParentContainers.StableSort([](const auto& Lhs, const auto& Rhs)
	{
		return Lhs.Key >= Rhs.Key;
	});
	
	if (!DiContainer->TryConnectSubcontainer(AsShared()))
	{
		UE_LOG(LogDependencyInjection, Error, TEXT("FForkingDiContainer::AddParentContainer: Failed to connect to parent container."));
	}
}

void DI::FForkingDiContainer::RemoveParentContainer(TWeakPtr<FConnectedDiContainer> DiContainer)
{
	for (auto It = ParentContainers.CreateIterator(); It; ++It)
	{
		if (It->Value != DiContainer)
			continue;

		It.RemoveCurrent();

		TSharedPtr<FConnectedDiContainer> PinnedParent = DiContainer.Pin();
		if (!PinnedParent)
			continue;

		if (!PinnedParent->TryDisconnectSubcontainer(AsShared()))
		{
			UE_LOG(LogDependencyInjection, Warning, TEXT("FForkingDiContainer::AddParentContainer: Failed to disconnect from parent container."));
		}
	}
}

bool DI::FForkingDiContainer::TryConnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer)
{
	ChildrenContainers.AddUnique(ConnectedDiContainer);
	ConnectedDiContainer->RetryAllPendingWaits();
	return true;
}

bool DI::FForkingDiContainer::TryDisconnectSubcontainer(TSharedRef<FConnectedDiContainer> ConnectedDiContainer)
{
	return ChildrenContainers.Remove(ConnectedDiContainer) > 0;
}

void DI::FForkingDiContainer::NotifyInstanceBound(const DI::FBinding& NewBinding) const
{
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

void DI::FForkingDiContainer::RetryAllPendingWaits() const
{
	for (auto ChildrenContainerIt = ChildrenContainers.CreateIterator(); ChildrenContainerIt; ++ChildrenContainerIt)
	{
		TSharedPtr<FConnectedDiContainer> ChainedDiContainer = ChildrenContainerIt->Pin();
		if (!ChainedDiContainer.IsValid())
		{
			ChildrenContainerIt.RemoveCurrent();
			continue;
		}

		ChainedDiContainer->RetryAllPendingWaits();
	}
}

TSharedPtr<DI::FBinding> DI::FForkingDiContainer::FindConnectedBinding(const FBindingId& BindingId) const
{
	for (auto It = ParentContainers.CreateIterator(); It; ++It)
	{
		TSharedPtr<FConnectedDiContainer> ParentDiContainer = It->Value.Pin();
		if (!ParentDiContainer.IsValid())
		{
			It.RemoveCurrent();
			continue;
		}

		if (TSharedPtr<DI::FBinding> Binding = ParentDiContainer->FindConnectedBinding(BindingId))
		{
			return Binding;
		}
	}
	return {};
}
