// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.
#pragma once

#include "CoreMinimal.h"
#include "Binding.h"
#include "BindingId.h"
#include "Tentacle.h"
#include "WeakFuture.h"
#include "TentacleTemplates.h"

namespace DI
{
	/**
	 * Resolver-backed binding for any TBindingType<T> category. Inherits from the corresponding
	 * TBindingType<T> so ResolveHelper's StaticCast still works; overrides the (within-hierarchy)
	 * virtual Resolve to invoke the resolver delegate.
	 *
	 * The binding is always valid (the resolver is bound at construction). Pending-resolve state is
	 * handled internally: concurrent Resolve() calls before the first async resolver completes share
	 * the same eventual result rather than invoking the resolver multiple times.
	 */
	template <class T>
	class TResolverBinding final : public TBindingType<T>
	{
	public:
		using Super = TBindingType<T>;
		using TInstanceRef = TBindingInstRef<T>;
		using FResolverDelegate = TDelegate<TWeakFuture<TInstanceRef>()>;

		TResolverBinding(FBindingId InBindingId, FResolverDelegate InResolverDelegate, bool bInTransient)
			: Super(MoveTemp(InBindingId))
			  , ResolverDelegate(MoveTemp(InResolverDelegate))
			  , bTransient(bInTransient)
		{
			checkf(ResolverDelegate.IsBound(), TEXT("TResolverBinding requires a bound resolver delegate."));
		}

		virtual bool IsValid() const override
		{
			return true;
		}

		virtual TWeakFuture<TInstanceRef> Resolve() const override
		{
			if (bTransient)
			{
				return ResolverDelegate.Execute();
			}

			{
				FRWScopeLock ScopeLock(ResolverLock, SLT_ReadOnly);
				// Serve from cache if the resolver has populated it AND the cached value is still
				// live. Super::IsValid() guards against e.g. a cached UObject being GCd, in which
				// case we fall through and re-run the resolver.
				if (Super::IsValid())
				{
					return Super::Resolve();
				}
			}


			TResolverBinding* MutableSelf = const_cast<TResolverBinding*>(this);
			TWeakFuture<TInstanceRef> PendingResolveFuture;
			bool bShouldLaunchResolver = false;
			{
				FRWScopeLock ScopeLock(ResolverLock, SLT_Write);

				// Re-Check if we have been resolved for some reason between swapping locks.
				if (Super::IsValid())
				{
					return Super::Resolve();
				}

				if (!bResolverInFlight)
				{
					MutableSelf->bResolverInFlight = true;
					bShouldLaunchResolver = true;
				}
				TWeakPromise<TInstanceRef> PendingResolvePromise = TWeakPromise<TInstanceRef>();
				PendingResolveFuture = PendingResolvePromise.GetWeakFuture();
				MutableSelf->PendingPromises.Add(MoveTemp(PendingResolvePromise));
			}

			if (bShouldLaunchResolver)
			{
				TWeakFuture<TInstanceRef> ResolvedInstanceFuture = ResolverDelegate.Execute();
				checkf(ResolvedInstanceFuture.IsValid(), TEXT("A Resolver MUST return a valid future!"));

				TWeakPtr<TResolverBinding> WeakSelf = StaticCastWeakPtr<TResolverBinding>(MutableSelf->AsWeak());
				ResolvedInstanceFuture.Next([WeakSelf](TOptional<TInstanceRef> Result) -> void
				{
					TSharedPtr<TResolverBinding> Self = WeakSelf.Pin();
					if (!Self.IsValid())
						return;

					TArray<TWeakPromise<TInstanceRef>> MovedPromises;
					TOptional<TInstanceRef> CachedInstance;
					{
						FRWScopeLock ScopeLock(Self->ResolverLock, SLT_Write);
						MovedPromises = MoveTemp(Self->PendingPromises);
						if (Result.IsSet())
						{
							Self->CacheInstance(Result.GetValue());
							// Read the cache back while we still hold the lock. GC can null a cached
							// UObject through AddReferencedObjects as soon as we release it, which
							// would fulfill the waiters with null instead of canceling them.
							CachedInstance = Self->ResolveFromCache();
						}
						Self->bResolverInFlight = false;
					}

					if (CachedInstance.IsSet())
					{
						for (auto& P : MovedPromises)
						{
							P.EmplaceValue(CachedInstance.GetValue());
						}
					}
					// Calls destructors on the promises -> cancels if not already fulfilled.
					// Would happen when Promises goes out of scope anyway, but we want to be explicit.
					MovedPromises.Empty();
				});
			}
			return MoveTemp(PendingResolveFuture);
		}


		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			FRWScopeLock ScopeLock(ResolverLock, SLT_Write);
			Super::AddReferencedObjects(Collector);
		}

	private:
		/** Copy the freshly produced instance into the corresponding base storage. */
		void CacheInstance(TInstanceRef Instance)
		{
			using FDecayed = std::decay_t<T>;
			if constexpr (THasUStruct<FDecayed>::Value && !THasUClass<FDecayed>::Value)
			{
				// UStruct
				this->StructData.template InitializeAs<T>(Instance);
			}
			else if constexpr (THasUClass<FDecayed>::Value && TIsIInterface<FDecayed>::Value)
			{
				// UInterface
				this->InterfaceDependency = static_cast<const FScriptInterface&>(Instance);
			}
			else if constexpr (THasUClass<FDecayed>::Value)
			{
				// UObject
				this->UObjectDependency = Instance;
			}
			else
			{
				// Native shared
				this->SharedNativeDependency = MoveTemp(Instance);
			}
		}

		/** Return the already-cached instance in the matching TInstanceRef shape. */
		TInstanceRef ResolveFromCache() const
		{
			using FDecayed = std::decay_t<T>;
			if constexpr (THasUStruct<FDecayed>::Value && !THasUClass<FDecayed>::Value)
			{
				return this->StructData.template Get<T>();
			}
			else if constexpr (THasUClass<FDecayed>::Value && TIsIInterface<FDecayed>::Value)
			{
				return TScriptInterface<T>(this->InterfaceDependency.GetObject());
			}
			else if constexpr (THasUClass<FDecayed>::Value)
			{
				return this->UObjectDependency;
			}
			else
			{
				return this->SharedNativeDependency.ToSharedRef();
			}
		}

		FResolverDelegate ResolverDelegate;
		bool bTransient = false;
		mutable bool bResolverInFlight = false;
		mutable FRWLock ResolverLock;
		mutable TArray<TWeakPromise<TInstanceRef>> PendingPromises;
	};
}
