#include "Misc/AutomationTest.h"
#include "Mocks/SimpleService.h"


BEGIN_DEFINE_SPEC(TentacleTemplatesSpec, "Tentacle.Templates",
                  EAutomationTestFlags::EngineFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext)

END_DEFINE_SPEC(TentacleTemplatesSpec)

void TentacleTemplatesSpec::Define()
{
	Describe("TIsBindingPtrValid", [this]
	{
		It("Should consider references as valid", [this]
		{
			static_assert(!DI::CConvertibleToBool<const FSimpleUStructService&>);
			static_assert(!DI::CConvertibleToBool<FSimpleUStructService>);

			const TOptional Struct = FSimpleUStructService();
			bool bStructIsValid = DI::TIsBindingPtrValid<decltype(*Struct)>::Check(*Struct);
			TestTrue("&Struct", bStructIsValid);
		});
		It("Should check TObjectPtr", [this]
		{
			static_assert(DI::CConvertibleToBool<const TObjectPtr<USimpleUService>&>);
			static_assert(DI::CConvertibleToBool<TObjectPtr<USimpleUService>>);

			const TObjectPtr SimpleUService = TObjectPtr<USimpleUService>{};
			bool bSimpleUServiceIsValid = DI::TIsBindingPtrValid<TObjectPtr<USimpleUService>>::Check(SimpleUService);
			TestFalse("*SimpleUService", bSimpleUServiceIsValid);
		});
		It("Should check TSharedPtr", [this]
		{
			static_assert(DI::CConvertibleToBool<const TSharedPtr<FSimpleNativeService>&>);
			static_assert(DI::CConvertibleToBool<TSharedPtr<FSimpleNativeService>>);

			const TSharedPtr<FSimpleNativeService> SimpleNativeService = {};
			bool bSimpleUServiceIsValid = DI::TIsBindingPtrValid<TSharedPtr<FSimpleNativeService>>::Check(SimpleNativeService);
			TestFalse("*SimpleUService", bSimpleUServiceIsValid);
		});
		It("Should check TScriptInterface", [this]
		{
			static_assert(DI::CConvertibleToBool<const TScriptInterface<ISimpleInterface>&>);
			static_assert(DI::CConvertibleToBool<TScriptInterface<ISimpleInterface>>);

			const TScriptInterface<ISimpleInterface> SimpleIService = {};
			bool bSimpleUServiceIsValid = DI::TIsBindingPtrValid<TScriptInterface<ISimpleInterface>>::Check(SimpleIService);
			TestFalse("*SimpleUService", bSimpleUServiceIsValid);
		});
	});
}
