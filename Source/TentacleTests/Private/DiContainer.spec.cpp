// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Container/DiContainer.h"
#include "Examples/ExampleComponent.h"
#include "Examples/ExampleNative.h"
#include "Misc/TypeContainer.h"
#include "Mocks/SimpleService.h"

BEGIN_DEFINE_SPEC(DiContainerSpec, "Tentacle.DiContainer",
                  EAutomationTestFlags::EngineFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext)

	DI::FDiContainer DiContainer;
END_DEFINE_SPEC(DiContainerSpec)

void DiContainerSpec::Define()
{
	BeforeEach([this]
	{
		DiContainer = DI::FDiContainer();
	});
	AfterEach([this]
	{
		DiContainer = DI::FDiContainer();
	});
	Describe("Constructor", [this]
	{
	});
	Describe("Bind", [this]
	{
		It("should bind UObjects", [this]
		{
			const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
			DiContainer.Bind().Instance<USimpleUService>(Service);
			TestEqual("DiContainer.Resolve().TryGet<USimpleUService>()", DiContainer.Resolve().TryGet<USimpleUService>(), Service);
		});
		It("should bind UInterfaces", [this]
		{
			const TObjectPtr<USimpleInterfaceImplementation> Service = NewObject<USimpleInterfaceImplementation>();
			DiContainer.Bind().Instance<ISimpleInterface>(Service);
			const TScriptInterface<ISimpleInterface>& ResolvedInterface = DiContainer.Resolve().TryGet<ISimpleInterface>();
			TestSame("DiContainer.Resolve().TryGet<ISimpleInterface>()", *ResolvedInterface, static_cast<ISimpleInterface&>(*Service));
		});
		It("should bind native classes", [this]
		{
			const TSharedRef<FSimpleNativeService> Service = MakeShared<FSimpleNativeService>(20);
			DiContainer.Bind().Instance<FSimpleNativeService>(Service);
			const TSharedPtr<FSimpleNativeService> Resolved = DiContainer.Resolve().TryGet<FSimpleNativeService>();
			TestEqual("Resolved->A", Resolved->A, 20);
		});
		It("should bind ustructs", [this]
		{
			FSimpleUStructService Service = FSimpleUStructService{20};
			DiContainer.Bind().Instance<FSimpleUStructService>(Service);
			TOptional<const FSimpleUStructService&> Resolved = DiContainer.Resolve().TryGet<FSimpleUStructService>();
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual("Resolved->A", Resolved->A, 20);
			}
		});
	});

	Describe("Resolve", [this]
	{
		Describe("TryGet", [this]
		{
			BeforeEach([this]
			{
				const char* InstanceName = "SomeName";

				TObjectPtr<USimpleUService> UService = NewObject<USimpleUService>();
				UService->A = 20;
				DiContainer.Bind().Instance<USimpleUService>(UService);
				UService = NewObject<USimpleUService>();
				UService->A = 22;
				DiContainer.Bind().NamedInstance<USimpleUService>(UService, InstanceName);

				TObjectPtr<USimpleInterfaceImplementation> SimpleInterfaceImplementation = NewObject<USimpleInterfaceImplementation>();
				SimpleInterfaceImplementation->A = 20;
				DiContainer.Bind().Instance<ISimpleInterface>(SimpleInterfaceImplementation);
				SimpleInterfaceImplementation = NewObject<USimpleInterfaceImplementation>();
				SimpleInterfaceImplementation->A = 22;
				DiContainer.Bind().NamedInstance<ISimpleInterface>(SimpleInterfaceImplementation, InstanceName);

				TSharedRef<FSimpleNativeService> NativeService = MakeShared<FSimpleNativeService>(20);
				DiContainer.Bind().Instance<FSimpleNativeService>(NativeService);
				NativeService = MakeShared<FSimpleNativeService>(22);
				DiContainer.Bind().NamedInstance<FSimpleNativeService>(NativeService, InstanceName);

				FSimpleUStructService StructService = FSimpleUStructService{20};
				DiContainer.Bind().Instance<FSimpleUStructService>(StructService);
				StructService = FSimpleUStructService{22};
				DiContainer.Bind().NamedInstance<FSimpleUStructService>(StructService, InstanceName);

				TSharedRef<FMockEngineType> MockEngineType = MakeShared<FMockEngineType>(20);
				DiContainer.Bind().Instance<FMockEngineType>(MockEngineType);
				MockEngineType = MakeShared<FMockEngineType>(22);
				DiContainer.Bind().NamedInstance<FMockEngineType>(MockEngineType, InstanceName);
			});
			
			It("should resolve UObjects", [this]
			{
				TestEqual("DiContainer.Resolve().TryGet<USimpleUService>()", DiContainer.Resolve().TryGet<USimpleUService>()->A, 20);
			});
			It("should resolve UInterfaces", [this]
			{
				TScriptInterface<ISimpleInterface> ResolvedInterface = DiContainer.Resolve().TryGet<ISimpleInterface>();
				TestEqual("DiContainer.Resolve().TryGet<ISimpleInterface>()->GetA()", ResolvedInterface->GetA(), 20);
			});
			It("should resolve native classes", [this]
			{
				const TSharedPtr<FSimpleNativeService> Resolved = DiContainer.Resolve().TryGet<FSimpleNativeService>();
				TestEqual("Resolved->A", Resolved->A, 20);
			});
			It("should resolve foreign native classes", [this]
			{
				const TSharedPtr<FMockEngineType> Resolved = DiContainer.Resolve().TryGet<FMockEngineType>();
				TestEqual("Resolved->A", Resolved->A, 20);
			});
			It("should resolve ustructs", [this]
			{
				TOptional<const FSimpleUStructService&> Resolved = DiContainer.Resolve().TryGet<FSimpleUStructService>();
				if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
				{
					TestEqual("Resolved->A", Resolved->A, 20);
				}
			});

			It("should resolve named UObjects", [this]
			{
				TestEqual("DiContainer.Resolve().TryGetNamed<USimpleUService>(Name)", DiContainer.Resolve().TryGetNamed<USimpleUService>("SomeName")->A, 22);
			});
			It("should resolve named UInterfaces", [this]
			{
				TestEqual("DiContainer.Resolve().TryGetNamed<USimpleUService>(Name)", DiContainer.Resolve().TryGetNamed<ISimpleInterface>("SomeName")->GetA(), 22);
			});
			It("should resolve named native classes", [this]
			{
				const TSharedPtr<FSimpleNativeService> Resolved = DiContainer.Resolve().TryGetNamed<FSimpleNativeService>("SomeName");
				TestEqual("Resolved->A", Resolved->A, 22);
			});
			It("should resolve named foreign native classes", [this]
			{
				const TSharedPtr<FMockEngineType> Resolved = DiContainer.Resolve().TryGetNamed<FMockEngineType>("SomeName");
				TestEqual("Resolved->A", Resolved->A, 22);
			});
			It("should resolve named UStructs", [this]
			{
				TOptional<const FSimpleUStructService&> Resolved = DiContainer.Resolve().TryGetNamed<FSimpleUStructService>("SomeName");
				if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
				{
					TestEqual("Resolved->A", Resolved->A, 22);
				}
			});

			It("should not resolve named UObjects with wrong name", [this]
			{
				TestFalse("DiContainer.Resolve().ResolveNamedInstance<USimpleUService>()",
						  bool(DiContainer.Resolve().TryGetNamed<USimpleUService>("SomeWrongName", DI::EResolveErrorBehavior::ReturnNull)));
			});
			It("should not resolve named UInterfaces with wrong name", [this]
			{
				TScriptInterface<ISimpleInterface> ResolvedInterface = DiContainer.Resolve().TryGetNamed<ISimpleInterface>("SomeWrongName", DI::EResolveErrorBehavior::ReturnNull);
				TestNull("DiContainer.Resolve().ResolveNamedInstance<ISimpleInterface>()", ResolvedInterface.GetObject());
			});
			It("should not resolve named native classes with wrong name", [this]
			{
				const TSharedPtr<FSimpleNativeService> Resolved = DiContainer.Resolve().TryGetNamed<FSimpleNativeService>("SomeWrongName", DI::EResolveErrorBehavior::ReturnNull);
				TestFalse("Resolved.IsValid()", Resolved.IsValid());
			});
			It("should not resolve named native foreign classes with wrong name", [this]
			{
				const TSharedPtr<FMockEngineType> Resolved = DiContainer.Resolve().TryGetNamed<FMockEngineType>("SomeWrongName", DI::EResolveErrorBehavior::ReturnNull);
				TestFalse("Resolved.IsValid()", Resolved.IsValid());
			});
			It("should not resolve named UStructs with wrong name", [this]
			{
				const TOptional<const FSimpleUStructService&> Resolved = DiContainer.Resolve().TryGetNamed<FSimpleUStructService>("SomeWrongName", DI::EResolveErrorBehavior::ReturnNull);
				TestFalse("Resolved.IsSet()", Resolved.IsSet());
			});
		});

		Describe("WaitFor", [this]
		{
			LatentIt("should resolve UObjects when already provided", [this](const FDoneDelegate& DoneDelegate)
			{
				TObjectPtr<USimpleUService> UService = NewObject<USimpleUService>();
				DiContainer.Bind().Instance<USimpleUService>(UService);
				DiContainer.Resolve().WaitFor<USimpleUService>().Next([DoneDelegate, this, UService](TOptional<TObjectPtr<USimpleUService>> Instance)
				{
					TestEqual("DiContainer.Resolve().ResolveFutureTypeInstance<USimpleUService>()", Instance->Get(), UService.Get());
					DoneDelegate.Execute();
				});
			});
			It("should resolve multiple UObjects when already provided", [this]
			{
				TObjectPtr<USimpleUService> UService = NewObject<USimpleUService>();
				TObjectPtr<USimpleInterfaceImplementation> UInterfaceService = NewObject<USimpleInterfaceImplementation>();
				FSimpleUStructService UStructService = {};
				TSharedRef<FSimpleNativeService> NativeServiceSharedPtr = MakeShared<FSimpleNativeService>();
				DiContainer.Bind().Instance<USimpleUService>(UService);
				DiContainer.Bind().Instance<ISimpleInterface>(UInterfaceService);
				DiContainer.Bind().Instance<FSimpleUStructService>(UStructService);
				DiContainer.Bind().Instance<FSimpleNativeService>(NativeServiceSharedPtr);
				auto [ResolvedUService, ResolvedUInterface, ResolvedUStruct, ResolvedNativeService] = DiContainer.Resolve().TryGetMany<USimpleUService, ISimpleInterface, FSimpleUStructService, FSimpleNativeService>();
				TestEqual("ObjectService", ResolvedUService.Get(), UService.Get());
				TestEqual<UObject*>("InterfaceService", ResolvedUInterface.GetObject(), UInterfaceService.Get());
				if (TestTrue("StructService.IsSet()", ResolvedUStruct.IsSet()))
				{
					TestEqual("StructService", *ResolvedUStruct, UStructService);
				}
				TestEqual("NativeService", ResolvedNativeService, NativeServiceSharedPtr.ToSharedPtr());
			});
		
			LatentIt("should resolve UObjects when provided later", [this](const FDoneDelegate& DoneDelegate)
			{
				TObjectPtr<USimpleUService> UService = NewObject<USimpleUService>();
				DiContainer.Resolve().WaitFor<USimpleUService>().Next([DoneDelegate, this, UService](TOptional<TObjectPtr<USimpleUService>> Instance)
				{
					if (TestTrue("Instance.IsSet()", Instance.IsSet()))
					{
						TestEqual("DiContainer.Resolve().ResolveFutureTypeInstance<USimpleUService>()", Instance->Get(), UService.Get());
					}
					DoneDelegate.Execute();
				});
				DiContainer.Bind().Instance<USimpleUService>(UService);
			});

			LatentIt("should resolve multiple UObjects when provided later", FTimespan::FromSeconds(1), [this](const FDoneDelegate& DoneDelegate)
			{
				TObjectPtr<USimpleUService> UService = NewObject<USimpleUService>();
				TObjectPtr<USimpleInterfaceImplementation> UInterfaceService = NewObject<USimpleInterfaceImplementation>();
				FSimpleUStructService UStructService = {999};
				TSharedRef<FSimpleNativeService> NativeServiceSharedPtr = MakeShared<FSimpleNativeService>();
				DiContainer
					.Resolve()
					.WaitForMany<USimpleUService, ISimpleInterface, FSimpleUStructService, FSimpleNativeService>()
					.AndThenExpand([DoneDelegate, this, UService, UInterfaceService, UStructService, NativeServiceSharedPtr](
						TObjectPtr<USimpleUService> ObjectService,
						TScriptInterface<ISimpleInterface> InterfaceService,
						const FSimpleUStructService& StructService,
						TSharedRef<FSimpleNativeService> NativeService)
						{
							TestEqual("ObjectService", ObjectService, UService);
							TestEqual<UObject*>("InterfaceService", InterfaceService.GetObject(), UInterfaceService);
							TestSame("&StructService", StructService, *DiContainer.Resolve().TryGet<FSimpleUStructService>());
							TestEqual("StructService", StructService, UStructService);
							TestEqual("StructService.A", StructService.A, UStructService.A);
							TestEqual("NativeService", NativeService, NativeServiceSharedPtr);
							DoneDelegate.Execute();
						}
					).OrElse([this, DoneDelegate]()
					{
						AddError("WaitForMany was canceled");
						DoneDelegate.Execute();
					});
				DiContainer.Bind().Instance<USimpleUService>(UService);
				DiContainer.Bind().Instance<ISimpleInterface>(UInterfaceService);
				DiContainer.Bind().Instance<FSimpleUStructService>(UStructService);
				DiContainer.Bind().Instance<FSimpleNativeService>(NativeServiceSharedPtr);
			});

			It("should invoke with unset optional when di container goes out of scope", [this]()
			{
				auto TempDiContainer = DI::FDiContainer();
				TempDiContainer.Resolve().WaitFor<USimpleUService>(nullptr, DI::EResolveErrorBehavior::ReturnNull).Next([this](TOptional<TObjectPtr<USimpleUService>> Instance)
				{
					TestFalse("Instance.IsSet()", Instance.IsSet());
				});
			});

			It("WaitFor should resolve structs via a reference to the binding storage", [this]()
			{
				DiContainer.Resolve().WaitFor<FSimpleUStructService>().Next([&, this](TOptional<const FSimpleUStructService&> Instance)
				{
					const FSimpleUStructService& Actual = *Instance;
					const FSimpleUStructService& Expected = *DiContainer.Resolve().TryGet<FSimpleUStructService>();
					TestSame<FSimpleUStructService>("Instance", Actual, Expected );
				});
				DiContainer.Bind().Instance<FSimpleUStructService>(FSimpleUStructService(999));
			});
		});
	});

	Describe("Inject", [this]
	{
		Describe("Sync", [this]
		{
			BeforeEach([this]
			{
				DiContainer.Bind().Instance<USimpleUService>(NewObject<USimpleUService>());
				DiContainer.Bind().Instance<FSimpleNativeService>(MakeShared<FSimpleNativeService>());
			});
			It("should inject into free functions", [this]
			{
				TestEqual("Service", *DiContainer.Inject().IntoFunctionByType(&DI::InjectTest::InjectDependencies), DiContainer.Resolve().TryGet<USimpleUService>());
			});
			It("should inject into member functions", [this]
			{
				FExampleNative Native = {};
				DiContainer.Inject().IntoFunctionByType(Native, &FExampleNative::Initialize);
				TestEqual("Native.SimpleNativeService", Native.SimpleNativeService, DiContainer.Resolve().TryGet<FSimpleNativeService>());
			});
			It("should inject into uobject member functions", [this]
			{
				UExampleComponent* ExampleComponent = NewObject<UExampleComponent>();
				DiContainer.Inject().IntoFunctionByType(*ExampleComponent, &UExampleComponent::InjectDependencies);
				TestEqual("ExampleComponent->SimpleUService", ExampleComponent->SimpleUService, DiContainer.Resolve().TryGet<USimpleUService>());
			});

			It("should inject into lambda functions with extra args", [this]
			{
				FExampleNative Native = {};
				FString ExtraString("test");
				DiContainer.Inject().IntoLambda([&](TSharedRef<FSimpleNativeService> NativeService)
				{
					Native.InitializeWithExtraArgs(NativeService, ExtraString);
				});
				TestEqual("Native.SimpleNativeService", Native.SimpleNativeService, DiContainer.Resolve().TryGet<FSimpleNativeService>());
			});

			It("should inject into uobject member functions with extra args", [this]
			{
				UExampleComponent* ExampleComponent = NewObject<UExampleComponent>();
				FString ExtraString("test");
				DiContainer.Inject()
					.IntoLambda([&](TObjectPtr<USimpleUService> SimpleUService)
						{
							ExampleComponent->InjectDependenciesWithExtraArgs(SimpleUService, ExtraString);
						}
					);
				TestEqual("ExampleComponent->SimpleUService", ExampleComponent->SimpleUService, DiContainer.Resolve().TryGet<USimpleUService>());
				TestEqual("ExampleComponent->ExtraString", ExampleComponent->ExtraString, ExtraString);
			});
		});

		Describe("Async", [this]
		{
			It("should async inject into free functions", [this]
			{
				DiContainer.Inject()
					.AsyncIntoStatic(&DI::InjectTest::InjectDependencies)
					.Next([this](TOptional<TObjectPtr<USimpleUService>> InjectedDependency)
					{
						if (TestTrue("InjectedService.IsSet()", InjectedDependency.IsSet()))
						{
							TestEqual("InjectedService", *InjectedDependency, DiContainer.Resolve().TryGet<USimpleUService>());
						}
					});
				DiContainer.Bind().Instance<USimpleUService>(NewObject<USimpleUService>());
			});
			It("should async inject into member functions", [this]
			{
				TSharedRef<FExampleNative> Native = MakeShared<FExampleNative>();
				DiContainer.Inject()
					.AsyncIntoSP<FExampleNative>(Native, &FExampleNative::Initialize)
					.AndThen([this](TSharedRef<FSimpleNativeService> InjectedDependency)
					{
						TestEqual("InjectedDependency", InjectedDependency, DiContainer.Resolve().TryGet<FSimpleNativeService>().ToSharedRef());
					});
				DiContainer.Bind().Instance<FSimpleNativeService>(MakeShared<FSimpleNativeService>());
			});
			It("should async inject into uobject member functions", [this]
			{
				UExampleComponent* ExampleComponent = NewObject<UExampleComponent>();
				DiContainer.Inject()
					.AsyncIntoUObject(*ExampleComponent, &UExampleComponent::InjectDependencies)
					.Next([this](TOptional<TObjectPtr<USimpleUService>> InjectedDependency)
					{
						if (TestTrue("InjectedDependency.IsSet()", InjectedDependency.IsSet()))
						{
							TestEqual("InjectedDependency", *InjectedDependency, DiContainer.Resolve().TryGet<USimpleUService>());
						}
					});
				DiContainer.Bind().Instance<USimpleUService>(NewObject<USimpleUService>());
			});
			LatentIt("should async inject into complex object member functions", [this](FDoneDelegate DoneDelegate)
			{
				UExampleComponent* ExampleComponent = NewObject<UExampleComponent>();
				DiContainer.Bind().Instance<USimpleUService>(NewObject<USimpleUService>());
				DiContainer.Bind().Instance<FSimpleNativeService>(MakeShared<FSimpleNativeService>());
				DiContainer.Bind().Instance<ISimpleInterface>(NewObject<USimpleInterfaceImplementation>());
				DiContainer.Bind().Instance<FSimpleUStructService>(FSimpleUStructService(999));
				DiContainer.Inject()
					.AsyncIntoUObject(*ExampleComponent, &UExampleComponent::ComplexInjectDependencies)
					.Next([this,DoneDelegate](TOptional<TTuple<TObjectPtr<USimpleUService>, TScriptInterface<ISimpleInterface>, TSharedRef<FSimpleNativeService>, const FSimpleUStructService*>> InjectedDependencies)
					{
						if (TestTrue("SimpleUService.IsSet()", InjectedDependencies.IsSet()))
						{
							auto [SimpleUService, InterfaceService, SimpleNativeService, SimpleStruct] = *InjectedDependencies;
							TestEqual("SimpleUService", SimpleUService, DiContainer.Resolve().TryGet<USimpleUService>());
							TestEqual("InterfaceService", InterfaceService, DiContainer.Resolve().TryGet<ISimpleInterface>());
							TestEqual("SimpleNativeService", SimpleNativeService, DiContainer.Resolve().TryGet<FSimpleNativeService>().ToSharedRef());
							TestEqual("SimpleStruct", *SimpleStruct, *DiContainer.Resolve().TryGet<FSimpleUStructService>());
							TestEqual("SimpleStruct->A", SimpleStruct->A, DiContainer.Resolve().TryGet<FSimpleUStructService>()->A);
							// TODO: fix the issue that references are not passed through properly through WeakFutures.
							//TestSame("&SimpleStruct", SimpleStruct, &*DiContainer.Resolve().TryGet<FSimpleUStructService>());
						}
						DoneDelegate.Execute();
					}).OrElse([this, DoneDelegate]
					{
						AddError("AsyncIntoUObject has been canceled.");
						DoneDelegate.Execute();
					});
			});
		});
	});
}
