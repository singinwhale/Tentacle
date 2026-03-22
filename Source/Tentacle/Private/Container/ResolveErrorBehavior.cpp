// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#include "Container/ResolveErrorBehavior.h"

#include "Tentacle.h"
#include "Blueprint/BlueprintExceptionInfo.h"
#include "Container/BindingId.h"
#include "UObject/Stack.h"

void DI::HandleResolveError(const FBindingId& BindingId, EResolveErrorBehavior ErrorBehavior)
{
	if (ErrorBehavior == EResolveErrorBehavior::ReturnNull)
		return;

	FString ErrorMessage = FString::Printf(TEXT("Failed to resolve binding %s"), *BindingId.ToString());
	switch (ErrorBehavior)
	{
	case EResolveErrorBehavior::ReturnNull:
		break;
	case EResolveErrorBehavior::LogWarning:
		UE_LOG(LogDependencyInjection, Warning, TEXT("%s"), *ErrorMessage)
		break;
	case EResolveErrorBehavior::LogError:
		UE_LOG(LogDependencyInjection, Error, TEXT("%s"), *ErrorMessage)
		break;
	case EResolveErrorBehavior::EnsureAlways:
		ensureAlwaysMsgf(false, TEXT("%s"), *ErrorMessage);
		break;
	case EResolveErrorBehavior::BlueprintException:
		{
			FFrame* Frame = FFrame::GetThreadLocalTopStackFrame();
			ensureMsgf(Frame, TEXT("Error behavior is BlueprintException but we do not seem to be inside a blueprint context!"));
			if (Frame)
			{
				FBlueprintExceptionInfo ExceptionInfo(
					EBlueprintExceptionType::AbortExecution,
					FText::FromString(ErrorMessage)
				);

				FBlueprintCoreDelegates::ThrowScriptException(Frame->Object, *Frame, ExceptionInfo);
				// Put break in here so that if the frame is invalid we fall through to the AssertCheck version.
				break;
			}
		}
	case EResolveErrorBehavior::AssertCheck:
		checkf(false, TEXT("%s"), *ErrorMessage);
		break;
	}
}
