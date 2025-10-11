#pragma once
#include "Misc/Optional.h"

template <>
struct TOptional<void>
{
public:
	using ElementType = void;

	/** Construct an void with a valid value. */
	TOptional(bool InIsSet)
		: bIsSet(InIsSet)
	{
	}

	/** Construct an OptionalType with an invalid value. */
	TOptional(FNullOpt)
		: TOptional()
	{
	}

	/** Construct an OptionalType with no value; i.e. unset */
	TOptional()
		:bIsSet(false)
	{
	}

	~TOptional()
	{
		Reset();
	}

	/** Copy/Move construction */
	TOptional(const TOptional& Other)
	{
		bIsSet = Other.bIsSet;
	}

	TOptional(TOptional&& Other)
	{
		bIsSet = Other.bIsSet;
	}

	TOptional& operator=(const TOptional& Other)
	{
		bIsSet = Other.bIsSet;
		return *this;
	}

	TOptional& operator=(TOptional&& Other)
	{
		bIsSet = Other.bIsSet;
		return *this;
	}

	template <class T>
	TOptional& operator=(T&)
	{
		bIsSet = true;
		return *this;
	}
	
	template <class T>
	TOptional& operator=(T&&)
	{
		bIsSet = true;
		return *this;
	}
	
	TOptional& operator=(bool bShouldBeSet)
	{
		bIsSet = bShouldBeSet;
		return *this;
	}

	void Reset()
	{
		bIsSet = false;
	}

	void Emplace(void)
	{
	}

	friend bool operator==(const TOptional& Lhs, const TOptional& Rhs)
	{
		return Lhs.bIsSet == Rhs.bIsSet;
	}

	friend bool operator!=(const TOptional& Lhs, const TOptional& Rhs)
	{
		return !(Lhs == Rhs);
	}

	void Serialize(FArchive& Ar)
	{
		Ar << bIsSet;
	}

	/** @return true when the value is meaningful; false if calling GetValue() is undefined. */
	bool IsSet() const
	{
		return bIsSet;
	}

	FORCEINLINE explicit operator bool() const
	{
		return IsSet();
	}

	/** @return The optional value; undefined when IsSet() returns false. */
	void GetValue()
	{
		checkf(IsSet(), TEXT("It is an error to call GetValue() on an unset TOptional. Please either check IsSet() or use Get(DefaultValue) instead."));
	}

	const void GetValue() const
	{
	}


	void operator*() const
	{
		
	}

	/** @return The optional value when set; DefaultValue otherwise. */
	const bool& Get(const bool& DefaultValue UE_LIFETIMEBOUND) const UE_LIFETIMEBOUND
	{
		return bIsSet;
	}

	/** @return A pointer to the optional value when set, nullptr otherwise. */
	bool* GetPtrOrNull()
	{
		return &bIsSet;
	}

	FORCEINLINE const bool* GetPtrOrNull() const
	{
		return const_cast<TOptional*>(this)->GetPtrOrNull();
	}

private:
	bool bIsSet = false;
};
