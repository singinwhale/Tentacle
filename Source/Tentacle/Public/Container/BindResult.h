// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"

namespace DI
{
	enum class EBindResult
	{
		// The binding has been created successfully.
		Bound,

		// The binding is in conflict with an already created binding and has been rejected.
		Conflict
	};
}
