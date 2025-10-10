# Tentacle Dependency Injection Framework

## Introduction to the API

The `Tentacle` Module contains the core of the Tentacle plugin and
provides all infrastructure for setting up the dependency injection architecture.

### Hello World Example

The most basic usage example goes like this:

```C++
// Create a simple DI Container
DI::FDiContainer DiContainer = DI::FDiContainer();

// Bind some UObject
const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
DiContainer.Bind().Instance<USimpleUService>(Service);

// Resolve the Object via its type
TObjectPtr<USimpleService> ResolvedService = DiContainer.Resolve().TryGet<USimpleUService>();
check(ResolvedService == Service);

// Or call a function with its arguments populated from the container
DiContainer.Inject().AsyncIntoUObject(*ExampleComponent, &UExampleComponent::InjectDependencies);
```

You can find more examples in the [Examples Folder](../TentacleTests/Private/Examples).

### Implementing a DI Context

A DI Context is an owner of a DI container that other object can use to resolve their dependencies.

A good reference for implementing a context is the `UDiContextComponent`:

```c++
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), NotBlueprintable)
class TENTACLE_API UDiContextComponent : public UActorComponent, public IDiContextInterface
{
	GENERATED_BODY()

public:
	UDiContextComponent();

	// - UActorComponent
	virtual void InitializeComponent() override;
	// - UObject
	static void AddReferencedObjects(UObject* Self, FReferenceCollector& Collector);
	// - IDIContextInterface
	virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainer; }
	virtual const DI::FChainedDiContainer& GetDiContainer()  const override { return *DiContainer; }
	// --

	UFUNCTION(BlueprintCallable)
	void SetAsParentOnAllComponentsOf(AActor* Actor) const;


protected:
	UPROPERTY(EditAnywhere, meta=(EditCondition="GetDefault<UTentacleSettings>()->bEnableScopeSubsystems"))
	bool bRegisterWorldAsParent = false;

	TSharedRef<DI::FChainedDiContainer> DiContainer = MakeShared<DI::FChainedDiContainer>();
};
```

Let's go through it step by step.

First, a DI context needs to implement the `IDiContextInterface`.
```c++
class TENTACLE_API UDiContextComponent : public UActorComponent, public IDiContextInterface
```
This interface cannot be implemented in blueprints. To use DI Contexts from Blueprint, you have to create either
a `UDiContextComponent` or a `UDiContainerObject`.
`IDiContextInterface` requires you to implement `GetDiContainer()` in a const, and non-const variant.
```c++
// - IDIContextInterface
virtual DI::FChainedDiContainer& GetDiContainer() override { return *DiContainer; }
virtual const DI::FChainedDiContainer& GetDiContainer()  const override { return *DiContainer; }
// --
```

To implement these methods your class needs to have an instance of a `FChainedDiContainer` 
which should be a protected member of your class.
```c++
TSharedRef<DI::FChainedDiContainer> DiContainer = MakeShared<DI::FChainedDiContainer>();
```

### Using your context

#### Generally

Generally, you should have the owner of your object do the initialization directly. 
This is not always possible due to the DI context not knowing the exact type of your object.
There are also no installers (yet).

The general case would go something like this:

```c++
UExampleComponent* ExampleComponent = NewObject<UExampleComponent>();
DiContainer.Inject().AsyncIntoUObject(*ExampleComponent, &UExampleComponent::InjectDependencies);
```

#### Actor Components

A very, very frequent use case is to initialize components as they are added to the actor. 
This works well for owned components that are added to the actor at spawn time:

```c++
void AExampleActor::PostInitializeComponents()
{
    ForEachComponent(
        false,
        [this](UActorComponent* ActorComponent)
        {
            DI::TryAutoInject(this, ActorComponent);
        }
    );
}
```

but for runtime added components, this won't work. There we need cooperation from the components,
to ask to be initialized by their context:
```c++
void UExampleComponent::BeginPlay()
{
	Super::BeginPlay();

	DI::RequestAutoInject(this);
}
```
`DI::RequestAutoInject` tries to find a containing `IAutoInjector` by walking up the ownership hierarchy.

Alternatively, you can mod the engine to add an event for when a component registers. 
The Actor is already notified in `AActor.h`
which you can call from `AActor::HandleRegisterComponentWithWorld` to allow actors to be 
notified when a component is registered.

Add this to AActor and call it from `HandleRegisterComponentWithWorld`
```c++
ENGINE_API virtual void ComponentRegistered(UActorComponent* Component);
```
Actors can then always do auto dependency injection automatically:

```c++
void AExampleActor::ComponentRegistered(UActorComponent* Component)
{
    DI::TryAutoInject(this, Component);
}
```

### Scope Subsystems

By enabling `UTantacleSettings::bEnableScopeSubsystems` you can choose to enable
automatic scope DI containers for all engine systems that support the subsystem framework.

Additionally, you may enable `UTentacleSettings::bEnableDefaultChaining` to link the scope subsystems
together in their hierarchical order:

```
Engine <-- Game Instance <-- World
                       ^---- Local Player
```

It is recommended to link the Player Controller to the Local Player and to the World.
If your game never repossesses the Pawn then you can also parent the Pawn to the Player Controller. 
See the [ExamplePlayerController](../TentacleTests/Private/Examples/ExamplePlayerController.cpp) for a reference implementation.

With the structure implemented by the example player controller, it looks like so:
```
Engine <-- Game Instance <-- World          <-- Player Controller <-- Pawn
                       ^---- Local Player   <--'
```