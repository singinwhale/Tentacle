// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Container/DiContainer.h"
#include "Mocks/SimpleService.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

BEGIN_DEFINE_SPEC(FDiContainerResolverSpec, "Tentacle.DiContainer.Bind.Resolver",
                  EAutomationTestFlags::EngineFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext)

	DI::FDiContainer DiContainer;
END_DEFINE_SPEC(FDiContainerResolverSpec)

void FDiContainerResolverSpec::Define()
{
	BeforeEach([this]
	{
		DiContainer = DI::FDiContainer();
	});
	AfterEach([this]
	{
		DiContainer = DI::FDiContainer();
	});

	It("should bind Resolvers from a plain lambda", [this]
	{
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		DiContainer.Bind().Resolver<USimpleUService>().Lambda([&]()
		{
			return Service;
		});
		TestEqual("DiContainer.Resolve().TryGet<USimpleUService>()", DiContainer.Resolve().TryGet<USimpleUService>(), Service);
	});

	It("should bind Resolvers from a weak lambda", [this]
	{
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		USimpleFactory* FactoryObject = NewObject<USimpleFactory>();
		DiContainer.Bind().Resolver<USimpleUService>().WeakLambda(FactoryObject, [&, FactoryObject]()
		{
			return Service;
		});
		TestEqual("DiContainer.Resolve().TryGet<USimpleUService>()", DiContainer.Resolve().TryGet<USimpleUService>(), Service);
	});

	It("should bind Resolvers from a static function", [this]
	{
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		DiContainer.Bind().Resolver<USimpleUService>().Static(&USimpleUService::Identity, Service);
		TestEqual("DiContainer.Resolve().TryGet<USimpleUService>()", DiContainer.Resolve().TryGet<USimpleUService>(), Service);
	});

	It("should bind Resolvers from shared pointers", [this]
	{
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		TSharedRef<FNativeFactory> SharedRefFactory = MakeShared<FNativeFactory>();
		DiContainer.Bind().Resolver<USimpleUService>().Shared(SharedRefFactory, &FNativeFactory::MakeSimpleService, Service);
		TestEqual("DiContainer.Resolve().TryGet<USimpleUService>()", DiContainer.Resolve().TryGet<USimpleUService>(), Service);
	});

	It("should bind Resolvers from uobjects", [this]
	{
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		USimpleFactory* FactoryObject = NewObject<USimpleFactory>();
		DiContainer.Bind().Resolver<USimpleUService>().UObject(FactoryObject, &USimpleFactory::MakeSimpleService, Service);
		TestEqual("DiContainer.Resolve().TryGet<USimpleUService>()", DiContainer.Resolve().TryGet<USimpleUService>(), Service);
	});

	It("should allow async binding", [this]
	{
		bool bResolverCalled = false;
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		DiContainer.Bind().Resolver<USimpleUService>().AsyncLambda([&](TWeakPromise<TObjectPtr<USimpleUService>> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(Service);
		});

		DiContainer.Resolve().WaitFor<USimpleUService>().Next([&](TOptional<TObjectPtr<USimpleUService>> ResolvedService)
		{
			TestEqual("ResolvedService", ResolvedService.Get(nullptr), Service);
		});
		TestTrue("bResolverCalled", bResolverCalled);
	});

	LatentIt("should allow async bindings that take a while", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
	{
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		DiContainer.Bind().Resolver<USimpleUService>().AsyncLambda([StrongService = TStrongObjectPtr(Service.Get()), this, DoneDelegate](TWeakPromise<TObjectPtr<USimpleUService>> BindingPromise)
		{
			FTSTicker::GetCoreTicker().AddTicker(TEXT(__FILE__  ":" UE_STRINGIZE(__LINE__)) , 0, [DoneDelegate, StrongService, BindingPromise2 = MoveTemp(BindingPromise)](float) mutable -> bool
			{
				BindingPromise2.EmplaceValue(StrongService.Get());
				DoneDelegate.Execute();
				return false;
			});
		});

		DiContainer.Resolve().WaitFor<USimpleUService>().Next([this, Service](TOptional<TObjectPtr<USimpleUService>> ResolvedService)
		{
			TestEqual("ResolvedService", ResolvedService.Get(nullptr), Service);
		});
	});

	It("should allow async transient bindings", [this]
	{
		int32 ResolverCalledCount = 0;
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		DiContainer.Bind().Resolver<USimpleUService>().AsyncLambda([&](TWeakPromise<TObjectPtr<USimpleUService>> BindingPromise)
		{
			ResolverCalledCount++;
			BindingPromise.EmplaceValue(Service);
		})
		.AsTransient();

		DiContainer.Resolve().WaitFor<USimpleUService>().Next([&](TOptional<TObjectPtr<USimpleUService>> ResolvedService)
		{
			TestEqual("ResolvedService", ResolvedService.Get(nullptr), Service);
		});

		DiContainer.Resolve().WaitFor<USimpleUService>().Next([&](TOptional<TObjectPtr<USimpleUService>> ResolvedService)
		{
			TestEqual("ResolvedService", ResolvedService.Get(nullptr), Service);
		});
		TestEqual("ResolverCalledCount", ResolverCalledCount, 2);
	});

	It("should allow async to be resolved immediately", [this]
	{
		bool bResolverCalled = false;
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		DiContainer.Bind().Resolver<USimpleUService>().AsyncLambda([&](TWeakPromise<TObjectPtr<USimpleUService>> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(Service);
		}).ResolveEagerly();
		TestTrue("bResolverCalled", bResolverCalled);
	});

	It("should respect conflict behavior", [this]
	{
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();
		AddExpectedMessage(TEXT("^.*binding.*already registered.*$"), ELogVerbosity::Warning);
		DiContainer.Bind().Resolver<USimpleUService>().AsyncLambda([&](TWeakPromise<TObjectPtr<USimpleUService>> BindingPromise)
		{
			BindingPromise.EmplaceValue(Service);
		}).ResolveEagerly();
		DiContainer.Bind().Resolver<USimpleUService>(DI::EBindConflictBehavior::LogWarning).AsyncLambda([&](TWeakPromise<TObjectPtr<USimpleUService>> BindingPromise)
		{
			BindingPromise.EmplaceValue(Service);
		}).ResolveEagerly();
	});

	It("should not crash when the binding is destroyed before the async resolver completes", [this]
	{
		// Stash the inner promise outside the resolver so it survives binding destruction.
		TSharedRef<TOptional<TWeakPromise<TObjectPtr<USimpleUService>>>> Sink = MakeShared<TOptional<TWeakPromise<TObjectPtr<USimpleUService>>>>();
		const TObjectPtr<USimpleUService> Service = NewObject<USimpleUService>();

		DiContainer.Bind().Resolver<USimpleUService>().AsyncLambda([Sink](TWeakPromise<TObjectPtr<USimpleUService>> BindingPromise)
		{
			Sink->Emplace(MoveTemp(BindingPromise));
		});

		// Kick off Resolve so the resolver fires (stashing the inner promise in Sink) and the
		// binding's [MutableSelf] continuation is parked on the inner future.
		DiContainer.Resolve().WaitFor<USimpleUService>().Next([](TOptional<TObjectPtr<USimpleUService>>) {});

		// Tear down the container -> binding -> outer pending promise. The stashed inner promise
		// survives because Sink still holds it.
		DiContainer = DI::FDiContainer();

		// Fulfilling the surviving promise wakes the parked continuation. Before the fix this
		// dereferenced a dangling raw pointer to the destroyed TResolverBinding.
		Sink->GetValue().EmplaceValue(Service);

		TestTrue("Reached end without crash", true);
	});

	// --- UInterface ---

	It("should bind UInterface Resolvers from a plain lambda", [this]
	{
		USimpleInterfaceImplementation* Impl = NewObject<USimpleInterfaceImplementation>();
		DiContainer.Bind().Resolver<ISimpleInterface>().Lambda([Impl]()
		{
			return TScriptInterface<ISimpleInterface>(Impl);
		});
		TScriptInterface<ISimpleInterface> Resolved = DiContainer.Resolve().TryGet<ISimpleInterface>();
		TestEqual<UObject*>("Resolved.GetObject()", Resolved.GetObject(), Impl);
	});

	It("should allow UInterface async binding", [this]
	{
		bool bResolverCalled = false;
		USimpleInterfaceImplementation* Impl = NewObject<USimpleInterfaceImplementation>();
		DiContainer.Bind().Resolver<ISimpleInterface>().AsyncLambda([&bResolverCalled, Impl](TWeakPromise<TScriptInterface<ISimpleInterface>> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(TScriptInterface<ISimpleInterface>(Impl));
		});

		DiContainer.Resolve().WaitFor<ISimpleInterface>().Next([this, Impl](TOptional<TScriptInterface<ISimpleInterface>> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual<UObject*>("Resolved.GetObject()", Resolved->GetObject(), Impl);
			}
		});
		TestTrue("bResolverCalled", bResolverCalled);
	});

	It("should allow UInterface async transient bindings", [this]
	{
		int32 ResolverCalledCount = 0;
		USimpleInterfaceImplementation* Impl = NewObject<USimpleInterfaceImplementation>();
		DiContainer.Bind().Resolver<ISimpleInterface>().AsyncLambda([&ResolverCalledCount, Impl](TWeakPromise<TScriptInterface<ISimpleInterface>> BindingPromise)
		{
			ResolverCalledCount++;
			BindingPromise.EmplaceValue(TScriptInterface<ISimpleInterface>(Impl));
		})
		.AsTransient();

		DiContainer.Resolve().WaitFor<ISimpleInterface>().Next([this, Impl](TOptional<TScriptInterface<ISimpleInterface>> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual<UObject*>("Resolved.GetObject()", Resolved->GetObject(), Impl);
			}
		});
		DiContainer.Resolve().WaitFor<ISimpleInterface>().Next([this, Impl](TOptional<TScriptInterface<ISimpleInterface>> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual<UObject*>("Resolved.GetObject()", Resolved->GetObject(), Impl);
			}
		});
		TestEqual("ResolverCalledCount", ResolverCalledCount, 2);
	});

	It("should allow UInterface async to be resolved immediately", [this]
	{
		bool bResolverCalled = false;
		USimpleInterfaceImplementation* Impl = NewObject<USimpleInterfaceImplementation>();
		DiContainer.Bind().Resolver<ISimpleInterface>().AsyncLambda([&bResolverCalled, Impl](TWeakPromise<TScriptInterface<ISimpleInterface>> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(TScriptInterface<ISimpleInterface>(Impl));
		}).ResolveEagerly();
		TestTrue("bResolverCalled", bResolverCalled);
	});

	// --- UStruct ---

	It("should bind UStruct Resolvers from a plain lambda", [this]
	{
		FSimpleUStructService Source(42);
		DiContainer.Bind().Resolver<FSimpleUStructService>().Lambda([Source]() -> const FSimpleUStructService&
		{
			return Source;
		});
		TOptional<const FSimpleUStructService&> Resolved = DiContainer.Resolve().TryGet<FSimpleUStructService>();
		if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
		{
			TestEqual("Resolved->A", Resolved->A, 42);
		}
	});

	It("should allow UStruct async binding", [this]
	{
		bool bResolverCalled = false;
		FSimpleUStructService Source(42);
		DiContainer.Bind().Resolver<FSimpleUStructService>().AsyncLambda([&bResolverCalled, Source](TWeakPromise<const FSimpleUStructService&> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(Source);
		});

		DiContainer.Resolve().WaitFor<FSimpleUStructService>().Next([this](TOptional<const FSimpleUStructService&> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual("Resolved->A", Resolved->A, 42);
			}
		});
		TestTrue("bResolverCalled", bResolverCalled);
	});

	It("should allow UStruct async transient bindings", [this]
	{
		int32 ResolverCalledCount = 0;
		FSimpleUStructService Source(42);
		DiContainer.Bind().Resolver<FSimpleUStructService>().AsyncLambda([&ResolverCalledCount, Source](TWeakPromise<const FSimpleUStructService&> BindingPromise)
		{
			ResolverCalledCount++;
			BindingPromise.EmplaceValue(Source);
		})
		.AsTransient();

		DiContainer.Resolve().WaitFor<FSimpleUStructService>().Next([this](TOptional<const FSimpleUStructService&> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual("Resolved->A", Resolved->A, 42);
			}
		});
		DiContainer.Resolve().WaitFor<FSimpleUStructService>().Next([this](TOptional<const FSimpleUStructService&> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual("Resolved->A", Resolved->A, 42);
			}
		});
		TestEqual("ResolverCalledCount", ResolverCalledCount, 2);
	});

	It("should allow UStruct async to be resolved immediately", [this]
	{
		bool bResolverCalled = false;
		FSimpleUStructService Source(42);
		DiContainer.Bind().Resolver<FSimpleUStructService>().AsyncLambda([&bResolverCalled, Source](TWeakPromise<const FSimpleUStructService&> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(Source);
		}).ResolveEagerly();
		TestTrue("bResolverCalled", bResolverCalled);
	});

	// --- Native ---

	It("should bind Native Resolvers from a plain lambda", [this]
	{
		TSharedRef<FSimpleNativeService> Source = MakeShared<FSimpleNativeService>();
		Source->A = 42;
		DiContainer.Bind().Resolver<FSimpleNativeService>().Lambda([Source]()
		{
			return Source;
		});
		TSharedPtr<FSimpleNativeService> Resolved = DiContainer.Resolve().TryGet<FSimpleNativeService>();
		TestEqual("Resolved", Resolved, Source.ToSharedPtr());
	});

	It("should allow Native async binding", [this]
	{
		bool bResolverCalled = false;
		TSharedRef<FSimpleNativeService> Source = MakeShared<FSimpleNativeService>();
		DiContainer.Bind().Resolver<FSimpleNativeService>().AsyncLambda([&bResolverCalled, Source](TWeakPromise<TSharedRef<FSimpleNativeService>> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(Source);
		});

		DiContainer.Resolve().WaitFor<FSimpleNativeService>().Next([this, Source](TOptional<TSharedRef<FSimpleNativeService>> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual("Resolved", *Resolved, Source);
			}
		});
		TestTrue("bResolverCalled", bResolverCalled);
	});

	It("should allow Native async transient bindings", [this]
	{
		int32 ResolverCalledCount = 0;
		TSharedRef<FSimpleNativeService> Source = MakeShared<FSimpleNativeService>();
		DiContainer.Bind().Resolver<FSimpleNativeService>().AsyncLambda([&ResolverCalledCount, Source](TWeakPromise<TSharedRef<FSimpleNativeService>> BindingPromise)
		{
			ResolverCalledCount++;
			BindingPromise.EmplaceValue(Source);
		})
		.AsTransient();

		DiContainer.Resolve().WaitFor<FSimpleNativeService>().Next([this, Source](TOptional<TSharedRef<FSimpleNativeService>> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual("Resolved", *Resolved, Source);
			}
		});
		DiContainer.Resolve().WaitFor<FSimpleNativeService>().Next([this, Source](TOptional<TSharedRef<FSimpleNativeService>> Resolved)
		{
			if (TestTrue("Resolved.IsSet()", Resolved.IsSet()))
			{
				TestEqual("Resolved", *Resolved, Source);
			}
		});
		TestEqual("ResolverCalledCount", ResolverCalledCount, 2);
	});

	It("should allow Native async to be resolved immediately", [this]
	{
		bool bResolverCalled = false;
		TSharedRef<FSimpleNativeService> Source = MakeShared<FSimpleNativeService>();
		DiContainer.Bind().Resolver<FSimpleNativeService>().AsyncLambda([&bResolverCalled, Source](TWeakPromise<TSharedRef<FSimpleNativeService>> BindingPromise)
		{
			bResolverCalled = true;
			BindingPromise.EmplaceValue(Source);
		}).ResolveEagerly();
		TestTrue("bResolverCalled", bResolverCalled);
	});
}
