#include "WeakFuture.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(WeakFutureSetSpec, "Tentacle.AsyncStreams.WeakFutureSet",
                  EAutomationTestFlags::EngineFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext)
END_DEFINE_SPEC(WeakFutureSetSpec)

void WeakFutureSetSpec::Define()
{	
	Describe("AndThenExpand", [this]
	{
		LatentIt("works with single void types", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
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
			Promise.SetValue(MakeTuple<TOptional<void>>(true));
		});
		
		LatentIt("works with mixed void types", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
		{
			constexpr int32 TestValue = 1234;
			TWeakPromiseSet<void, int32, void> Promise;
			{
				TWeakFutureSet<void, int32, void> Future = Promise.GetWeakFutureSet();
				Future
					.AndThenExpand([this, DoneDelegate](int32 TransportedValue)
					{
						TestEqual("TransportedValue", TransportedValue, TestValue);
						DoneDelegate.Execute();
					})
					.OrElse([this, DoneDelegate]
					{
						AddError("Future was canceled for no good reason.");
						DoneDelegate.Execute();
					});
			}
			Promise.SetValue(MakeTuple<TOptional<void>,TOptional<int32>,TOptional<void>>(true, TestValue, true));
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

		// TODO: fix the issue that references are not passed through properly through WeakFutures. This also prevents usage of non-copyable immovable types.
		xLatentIt("should transport references properly", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
		{
			const int32 TestValue = 1234;
			TWeakPromiseSet<const int32&> Promise;
			{
				TWeakFutureSet<const int32&> Future = Promise.GetWeakFutureSet();
				Future.AndThenExpand([this, DoneDelegate, TestValue](const int32& InTransportedValue)
				{
					TestSame("InTransportedValue", InTransportedValue, TestValue);
					DoneDelegate.Execute();
				});
			}
			Promise.SetValue(MakeTuple<TOptional<const int32&>>(TestValue));
		});

		LatentIt("should transport the returned value properly", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
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

		// TODO: fix the issue that references are not passed through properly through WeakFutures. This also prevents usage of non-copyable immovable types.
		/*LatentIt("should transport references to immovable and uncopyable types properly", FTimespan::FromSeconds(1), [this](FDoneDelegate DoneDelegate)
		{
			struct FImmovable
			{
				FImmovable(int32 InA):A(InA){};
				FImmovable(FImmovable&& Other) = delete;
				FImmovable& operator=(FImmovable&& Other) = delete;
				FImmovable(const FImmovable&) = delete;
				FImmovable& operator=(const FImmovable&) = delete;

				int32 A = -1;
			};
			const FImmovable TestValue = FImmovable(1234);
			TWeakPromiseSet<const FImmovable&> Promise;
			{
				TWeakFutureSet<const FImmovable&> Future = Promise.GetWeakFutureSet();
				Future.AndThenExpand([this, DoneDelegate, &TestValue](const FImmovable& InTransportedValue)
				{
					TestSame("InTransportedValue", InTransportedValue, TestValue);
					DoneDelegate.Execute();
				});
			}
			Promise.SetValue(MakeTuple<TOptional<const FImmovable&>>(TestValue));
		});*/
	});
}
