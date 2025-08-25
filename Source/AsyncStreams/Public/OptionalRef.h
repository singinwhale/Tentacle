#pragma once
#include "Misc/Optional.h"

template <typename OptionalType>
struct TOptional<OptionalType&>
{
public:
	using ElementType = OptionalType&;

	/** Construct an OptionalType with a valid value. */
	TOptional(OptionalType& InValue)
		: TOptional(InPlace, &InValue)
	{
	}

	explicit TOptional(EInPlace, OptionalType* OptionalValue)
	{
		OptionalRefPtr = OptionalValue;
	}

	/** Construct an OptionalType with an invalid value. */
	TOptional(FNullOpt)
		: TOptional()
	{
	}

	/** Construct an OptionalType with no value; i.e. unset */
	TOptional()
	{
		OptionalRefPtr = nullptr;
	}

	~TOptional()
	{
		Reset();
	}

	/** Copy/Move construction */
	TOptional(const TOptional& Other)
	{
		OptionalRefPtr = Other.OptionalRefPtr;
	}

	TOptional(TOptional&& Other)
	{
		Swap(OptionalRefPtr, Other.OptionalRefPtr);
	}

	TOptional& operator=(const TOptional& Other)
	{
		if (&Other != this)
		{
			OptionalRefPtr = Other.OptionalRefPtr;
		}
		return *this;
	}

	TOptional& operator=(TOptional&& Other)
	{
		if (&Other != this)
		{
			Swap(OptionalRefPtr, Other.OptionalRefPtr);
		}
		return *this;
	}

	TOptional& operator=(OptionalType& InValue)
	{
		OptionalRefPtr = &InValue;
		return *this;
	}

	TOptional& operator=(OptionalType&& InValue)
	{
		OptionalRefPtr = &InValue;
		return *this;
	}

	void Reset()
	{
		OptionalRefPtr = nullptr;
	}

	OptionalType& Emplace(OptionalType& InValue)
	{
		OptionalRefPtr = &InValue;
		return *OptionalRefPtr;
	}

	friend bool operator==(const TOptional& Lhs, const TOptional& Rhs)
	{
		return Lhs.OptionalRefPtr == Rhs.OptionalRefPtr;
	}

	friend bool operator!=(const TOptional& Lhs, const TOptional& Rhs)
	{
		return !(Lhs == Rhs);
	}

	void Serialize(FArchive& Ar)
	{
		Ar << OptionalRefPtr;
	}

	/** @return true when the value is meaningful; false if calling GetValue() is undefined. */
	bool IsSet() const
	{
		return OptionalRefPtr != nullptr;
	}

	FORCEINLINE explicit operator bool() const
	{
		return IsSet();
	}

	/** @return The optional value; undefined when IsSet() returns false. */
	OptionalType& GetValue()
	{
		checkf(IsSet(), TEXT("It is an error to call GetValue() on an unset TOptional. Please either check IsSet() or use Get(DefaultValue) instead."));
		return *OptionalRefPtr;
	}

	FORCEINLINE const OptionalType& GetValue() const
	{
		return const_cast<TOptional*>(this)->GetValue();
	}

	OptionalType* operator->()
	{
		return &GetValue();
	}

	FORCEINLINE const OptionalType* operator->() const
	{
		return const_cast<TOptional*>(this)->operator->();
	}

	OptionalType& operator*()
	{
		return GetValue();
	}

	FORCEINLINE const OptionalType& operator*() const
	{
		return const_cast<TOptional*>(this)->operator*();
	}

	/** @return The optional value when set; DefaultValue otherwise. */
	const OptionalType& Get(const OptionalType& DefaultValue UE_LIFETIMEBOUND) const UE_LIFETIMEBOUND
	{
		return IsSet() ? *(const OptionalType*)OptionalRefPtr : DefaultValue;
	}

	/** @return A pointer to the optional value when set, nullptr otherwise. */
	OptionalType* GetPtrOrNull()
	{
		return IsSet() ? (OptionalType*)OptionalRefPtr : nullptr;
	}

	FORCEINLINE const OptionalType* GetPtrOrNull() const
	{
		return const_cast<TOptional*>(this)->GetPtrOrNull();
	}

private:
	OptionalType* OptionalRefPtr;
};
