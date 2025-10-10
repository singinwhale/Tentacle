// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Container/ChainedDiContainer.h"
#include "Container/ForkingDiContainer.h"
#include "Contexts/DiContextComponent.h"
#include "Contexts/DIContextInterface.h"
#include "GameFramework/PlayerController.h"
#include "ExamplePlayerController.generated.h"

class UDiContextComponent;

/**
 * Example player controller that demonstrates a split parent relationship
 */
UCLASS(HideDropdown, NotBlueprintable)
class TENTACLETESTS_API AExamplePlayerController : public APlayerController, public IDiContextInterface
{
	GENERATED_BODY()

public:
	AExamplePlayerController();

	// - IDiContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return DiContextComponent->GetDiContainer(); }
	virtual const DI::FChainedDiContainer& GetDiContainer() const override { return DiContextComponent->GetDiContainer(); }
	// - APlayerController
	virtual void ReceivedPlayer() override;
	virtual void OnNetCleanup(class UNetConnection* Connection) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetPawn(APawn* InPawn) override;
	// - AActor
	virtual void PreInitializeComponents() override;
	// --
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dependency Injection")
	TObjectPtr<UDiContextComponent> DiContextComponent;

private:
	void TryRemoveDiChildFromLocalPlayerContainer();

	/**
	 * Virtual container that only delegates to other containers.
	 * The container that actually saves bindings on this class lies on the DiContextComponent.
	 * This one is used to fork to the World and LocalPlayer once it is set.
	 */
	TSharedRef<DI::FForkingDiContainer> ForkingDiContainer = MakeShared<DI::FForkingDiContainer>();
};
