// Copyright 2025 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"

namespace DI
{
	class FBindingId;

	/**
	 * Used to indicate how to handle a binding conflict when trying to bind a dependency in a DiContainer.
	 */
	enum class EBindConflictBehavior
	{
		None,
		LogWarning,
		LogError,
		EnsureAlways,
		AssertCheck,
		BlueprintException,
	};

	constexpr EBindConflictBehavior GDefaultConflictBehavior = EBindConflictBehavior::LogError;

	/** Default implementation for reacting to EBindConflictBehavior */
	TENTACLE_API void HandleBindingConflict(const FBindingId& BindingId, EBindConflictBehavior ConflictBehavior);
}
