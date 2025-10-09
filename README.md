# Tentacle

Inversion of Control (Dependency Injection) framework for Unreal Engine roughly inspired by [Zenject](https://github.com/modesttree/Zenject)

## Examples

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
DiContainer.Inject().AsyncIntoFunctionByType(*ExampleComponent, &UExampleComponent::InjectDependencies);
```

For more info see [full Readme](Source/Tentacle/README.md)
