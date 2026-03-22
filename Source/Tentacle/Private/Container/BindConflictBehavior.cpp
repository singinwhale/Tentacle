// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#include "Container/BindConflictBehavior.h"

#include "Tentacle.h"
#include "Blueprint/BlueprintExceptionInfo.h"
#include "Container/BindingId.h"
#include "UObject/Stack.h"


void DI::HandleBindingConflict(const FBindingId& BindingId, EBindConflictBehavior ConflictBehavior)
{
	if (ConflictBehavior != EBindConflictBehavior::None)
	{
		const FString ErrorMessage = FString::Printf(
			TEXT("An instance for binding %s is already registered!"),
			*BindingId.ToString()
		);
		switch (ConflictBehavior)
		{
		case EBindConflictBehavior::LogWarning:
			UE_LOG(LogDependencyInjection, Warning, TEXT("%s"), *ErrorMessage)
			break;
		case EBindConflictBehavior::LogError:
			UE_LOG(LogDependencyInjection, Error, TEXT("%s"), *ErrorMessage)
			break;
		case EBindConflictBehavior::EnsureAlways:
			ensureAlwaysMsgf(false, TEXT("%s"), *ErrorMessage);
			break;
		case EBindConflictBehavior::BlueprintException:
			{
				FFrame* Frame = FFrame::GetThreadLocalTopStackFrame();
				if (ensureMsgf(Frame, TEXT("")))
				{
					FBlueprintExceptionInfo ExceptionInfo(
						EBlueprintExceptionType::FatalError,
						INVTEXT("Binding conflict for binding %s")
					);

					FBlueprintCoreDelegates::ThrowScriptException(Frame->Object, *Frame, ExceptionInfo);
					// Put break here so we fall through to AssertCheck if we don't have a valid blueprint stack frame.
					break;
				}
			}
		default:
		case EBindConflictBehavior::AssertCheck:
			checkf(false, TEXT("%s"), *ErrorMessage);
			break;
		}
	}
}
