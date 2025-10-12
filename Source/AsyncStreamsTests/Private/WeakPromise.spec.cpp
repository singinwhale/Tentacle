#include "WeakFuture.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(WeakPromiseSpec, "Tentacle.AsyncStreams.WeakPromise",
                  EAutomationTestFlags::EngineFilter | EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProgramContext)
END_DEFINE_SPEC(WeakPromiseSpec)

void WeakPromiseSpec::Define()
{
	Describe("SetValue", [this]
	{
		It("should call the follow-up event", [this]
		{
			bool bFollowUpEventWasCalled = false;
			TWeakPromise<bool> Promise;
			{
				TWeakFuture<bool> Future = Promise.GetWeakFuture();
				Future.Next([&bFollowUpEventWasCalled](TOptional<bool>)
				{
					bFollowUpEventWasCalled = true;
				});
			}
			Promise.SetValue(true);

			TestTrue("Follow-up event was called", bFollowUpEventWasCalled);
		});

		It("should transport the value properly", [this]
		{
			bool bFollowUpEventWasCalled = false;
			TWeakPromise<bool> Promise;
			TWeakFuture<bool> Future = Promise.GetWeakFuture();
			Promise.SetValue(true);
			TestTrue("Result", Future.Get().GetValue());
		});
	});
	Describe("Cancel", [this]
	{
		It("should call the follow-up event", [this]
		{
			bool bFollowUpEventWasCalled = false;
			TWeakPromise<bool> Promise;
			{
				TWeakFuture<bool> Future = Promise.GetWeakFuture();
				Future.Next([&bFollowUpEventWasCalled](TOptional<bool>)
				{
					bFollowUpEventWasCalled = true;
				});
			}
			Promise.Cancel();

			TestTrue("Follow-up event was called", bFollowUpEventWasCalled);
		});

		It("should transport no value", [this]
		{
			TWeakPromise<bool> Promise;
			TWeakFuture<bool> Future = Promise.GetWeakFuture();
			Promise.Cancel();
			TestFalse("Result is set", Future.Get().IsSet());
		});
	});

	Describe("void types", [this]
	{
		It("should call the follow-up event for values", [this]
		{
			bool bFollowUpEventWasCalled = false;
			TWeakPromise<void> Promise;
			{
				TWeakFuture<void> Future = Promise.GetWeakFuture();
				Future.Next([this, &bFollowUpEventWasCalled](bool bValueSet)
				{
					TestTrue("bValueSet", bValueSet);
					bFollowUpEventWasCalled = true;
				});
			}
			Promise.SetValue();

			TestTrue("Follow-up event was called", bFollowUpEventWasCalled);
		});
		It("should call the follow-up event for cancellations", [this]
		{
			bool bFollowUpEventWasCalled = false;
			TWeakPromise<void> Promise;
			{
				TWeakFuture<void> Future = Promise.GetWeakFuture();
				Future.Next([this, &bFollowUpEventWasCalled](bool bValueSet)
				{
					TestFalse("bValueSet", bValueSet);
					bFollowUpEventWasCalled = true;
				});
			}
			Promise.Cancel();

			TestTrue("Follow-up event was called", bFollowUpEventWasCalled);
		});
	});
}
