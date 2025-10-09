// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.


#include "Container/ChainedDiContainer.h"
#include "Container/DiContainer.h"
#include "Container/ForkingDiContainer.h"
#include "Examples/ExampleComponent.h"
#include "Examples/ExampleNative.h"
#include "Misc/TypeContainer.h"
#include "Mocks/SimpleService.h"

BEGIN_DEFINE_SPEC(ConnectedDiContainerSpec, "Tentacle.ConnectedDiContainer",
                  EAutomationTestFlags::EngineFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext)

	TSharedPtr<DI::FChainedDiContainer> ChildContainer;
	TSharedPtr<DI::FChainedDiContainer> ParentContainer;
	TSharedPtr<DI::FChainedDiContainer> OtherParentContainer;
	TSharedPtr<DI::FForkingDiContainer> ForkingDiContainer;
	TObjectPtr<USimpleUService> Service;
END_DEFINE_SPEC(ConnectedDiContainerSpec)

void ConnectedDiContainerSpec::Define()
{
	BeforeEach([this]
	{
		ChildContainer = MakeShared<DI::FChainedDiContainer>();
		ParentContainer = MakeShared<DI::FChainedDiContainer>();
		OtherParentContainer = MakeShared<DI::FChainedDiContainer>();
		ForkingDiContainer = MakeShared<DI::FForkingDiContainer>();
		Service = NewObject<USimpleUService>();

		ChildContainer->SetParentContainer(ForkingDiContainer);
		ForkingDiContainer->AddParentContainer(OtherParentContainer.ToSharedRef(), 0);
		ForkingDiContainer->AddParentContainer(ParentContainer.ToSharedRef(), 1);
	});
	AfterEach([this]
	{
		ChildContainer.Reset();
		ParentContainer.Reset();
		OtherParentContainer.Reset();
		ForkingDiContainer.Reset();
	});
	Describe("FindBinding", [this]
	{
		It("should search parents", [this]
		{
			ParentContainer->Bind().Instance<USimpleUService>(Service);

			TestEqual("ChildContainer.Resolve().TryGet<USimpleUService>()", ChildContainer->Resolve().TryGet<USimpleUService>(), Service);
		});
		It("should search other parents", [this]
		{
			OtherParentContainer->Bind().Instance<USimpleUService>(Service);

			TestEqual("ChildContainer.Resolve().TryGet<USimpleUService>()", ChildContainer->Resolve().TryGet<USimpleUService>(), Service);
		});
		It("should respect priority of parents", [this]
		{
			USimpleUService* OtherService = NewObject<USimpleUService>();
			ParentContainer->Bind().Instance<USimpleUService>(Service);
			OtherParentContainer->Bind().Instance<USimpleUService>(OtherService);

			TestEqual("ChildContainer.Resolve().TryGet<USimpleUService>()", ChildContainer->Resolve().TryGet<USimpleUService>(), Service);
		});

	});
	Describe("BindSpecific", [this]
	{
		LatentIt("should notify children", FTimespan::FromSeconds(1),[this](FDoneDelegate Done)
		{
			ChildContainer->Resolve().WaitFor<USimpleUService>().Next([Done,this](TOptional<TObjectPtr<USimpleUService>> ResolvedService)
			{
				TestEqual("ResolvedService", *ResolvedService, Service);
				Done.Execute();
			});
			ParentContainer->Bind().Instance<USimpleUService>(Service);
		});
	});
}
