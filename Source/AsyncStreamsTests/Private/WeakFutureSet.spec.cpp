#include "WeakFuture.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(WeakFutureSetSpec, "Tentacle.AsyncStreams.WeakFutureSet",
                  EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ProgramContext)
END_DEFINE_SPEC(WeakFutureSetSpec)

void WeakFutureSetSpec::Define()
{
	Describe("AndThenExpand", [this]
	{
		LatentIt("works with void functions", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
		{
			TWeakPromiseSet<void> Promise;
			{
				TWeakFutureSet<void> Future = Promise.GetWeakFutureSet();
				Future
					.AndThenExpand([this, DoneDelegate]
					{
						DoneDelegate.Execute();
					})
					.OrElse([this, DoneDelegate]
					{
						AddError("Future was canceled for no good reason.");
						DoneDelegate.Execute();
					});
			}
			Promise.SetValue(TTuple<TOptional<void>>{});
		});

		LatentIt("should transport the value properly", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
		{
			const int32 TestValue = 1234;
			TWeakPromiseSet<int32> Promise;
			{
				TWeakFutureSet<int32> Future = Promise.GetWeakFutureSet();
				Future.AndThenExpand([this, DoneDelegate, TestValue](int32 InTransportedValue)
				{
					TestEqual("InTransportedValue", InTransportedValue, TestValue);
					DoneDelegate.Execute();
				});
			}
			Promise.SetValue(MakeTuple<TOptional<int32>>(TestValue));
		});

		LatentIt("should transport the value properly", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
		{
			const int32 TestValue = 1234;
			TWeakPromiseSet<int32> Promise;
			{
				TWeakFutureSet<int32> Future = Promise.GetWeakFutureSet();
				Future
					.AndThenExpand([](int32 InTransportedValue)
					{
						return InTransportedValue;
					})
					.AndThen([this, DoneDelegate, TestValue](int32 InPrevReturnValue)
					{
						TestEqual("InPrevReturnValue", InPrevReturnValue, TestValue);
						DoneDelegate.Execute();
					});
			}
			Promise.SetValue(MakeTuple<TOptional<int32>>(TestValue));
		});
	});
}
