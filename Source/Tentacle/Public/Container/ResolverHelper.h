// Copyright 2026 singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "BindConflictBehavior.h"
#include "BindResult.h"
#include "Binding.h"
#include "ResolverBinding.h"
#include "BindingId.h"
#include "Tentacle.h"
#include "WeakFuture.h"
#include "TentacleTemplates.h"

namespace DI
{
	/**
	 * Fluent resolver binding builder. Returned by TBindingHelper::Resolver<T>().
	 *
	 * Sync binders (Lambda / WeakLambda / Static / Shared / UObject) accept a callable that returns
	 * TBindingInstRef<T> and is internally wrapped into an immediately-fulfilled TWeakFuture. The
	 * weak-context variants (WeakLambda / Shared / UObject) cancel the resolve if the context
	 * disappears before the resolver runs.
	 *
	 * Async binders (currently AsyncLambda) accept a callable that takes a TWeakPromise<TInstanceRef>
	 * by value. The callable is responsible for fulfilling or canceling the promise.
	 *
	 * Modifiers (AsTransient / ResolveEagerly) can be chained before or after the binder.
	 *
	 * Move-only. Registers a single TResolverBinding<T> on destruction (end of the build
	 * expression). If ResolveEagerly() was chained, the resolver is also invoked at that point
	 * rather than waiting for the first Resolve() call.
	 */
	template <class T, class TDiContainer>
	class TResolverBindingBuilder
	{
	public:
		using TInstanceRef = TBindingInstRef<T>;
		using FAsyncReturn = TWeakFuture<TInstanceRef>;
		using FResolverDelegate = TDelegate<FAsyncReturn()>;

		TResolverBindingBuilder(TDiContainer& InDiContainer, FBindingId InBindingId, EBindConflictBehavior InConflictBehavior)
			: DiContainer(&InDiContainer)
			, BindingId(MoveTemp(InBindingId))
			, ConflictBehavior(InConflictBehavior)
		{
		}

		TResolverBindingBuilder(const TResolverBindingBuilder&) = delete;
		TResolverBindingBuilder& operator=(const TResolverBindingBuilder&) = delete;
		TResolverBindingBuilder(TResolverBindingBuilder&& Other) noexcept
			: DiContainer(Other.DiContainer)
			, BindingId(MoveTemp(Other.BindingId))
			, ConflictBehavior(Other.ConflictBehavior)
			, Resolver(MoveTemp(Other.Resolver))
			, bTransient(Other.bTransient)
			, bResolveEagerly(Other.bResolveEagerly)
			, bFinalized(Other.bFinalized)
		{
			Other.bFinalized = true;
		}
		TResolverBindingBuilder& operator=(TResolverBindingBuilder&&) = delete;

		~TResolverBindingBuilder()
		{
			Finalize();
		}

		/** Bind a synchronous resolver lambda returning TBindingInstRef<T>. */
		template <class TLambda>
			requires std::is_invocable_r_v<TInstanceRef, TLambda>
		TResolverBindingBuilder& Lambda(TLambda&& InLambda)
		{
			Resolver.BindLambda([SyncLambda = Forward<TLambda>(InLambda)]() mutable -> FAsyncReturn
			{
				return MakeFulfilledWeakPromise<TInstanceRef>(SyncLambda()).GetWeakFuture();
			});
			return *this;
		}

		/** Bind a synchronous resolver lambda weakly tied to a UObject context. Cancels resolve if the context is gone. */
		template <class UObjectType, class TLambda>
			requires std::is_invocable_r_v<TInstanceRef, TLambda>
		TResolverBindingBuilder& WeakLambda(UObjectType* Context, TLambda&& InLambda)
		{
			using FInnerDelegate = TDelegate<TInstanceRef()>;
			FInnerDelegate Inner = FInnerDelegate::CreateWeakLambda(Context, Forward<TLambda>(InLambda));
			Resolver.BindLambda([InnerCapture = MoveTemp(Inner)]() mutable -> FAsyncReturn
			{
				return WrapInnerExecute(InnerCapture);
			});
			return *this;
		}

		/** Bind a static / free function with optional pre-bound arguments. */
		template <class FuncType, class... VarTypes>
		TResolverBindingBuilder& Static(FuncType* StaticFunc, VarTypes... Args)
		{
			Resolver.BindLambda([StaticFunc, Args...]() mutable -> FAsyncReturn
			{
				return MakeFulfilledWeakPromise<TInstanceRef>(StaticFunc(Args...)).GetWeakFuture();
			});
			return *this;
		}

		/** Bind a member function on a shared pointer with optional pre-bound arguments. Cancels resolve if the shared object is gone. */
		template <class TObjType, class TMethod, class... VarTypes>
		TResolverBindingBuilder& Shared(TSharedRef<TObjType> Obj, TMethod Method, VarTypes... Args)
		{
			using FInnerDelegate = TDelegate<TInstanceRef()>;
			FInnerDelegate Inner = FInnerDelegate::CreateSP(Obj, Method, Args...);
			Resolver.BindLambda([InnerCapture = MoveTemp(Inner)]() mutable -> FAsyncReturn
			{
				return WrapInnerExecute(InnerCapture);
			});
			return *this;
		}

		/** Bind a member function on a UObject with optional pre-bound arguments. Cancels resolve if the UObject is gone. */
		template <class UObjType, class TMethod, class... VarTypes>
		TResolverBindingBuilder& UObject(UObjType* Obj, TMethod Method, VarTypes... Args)
		{
			using FInnerDelegate = TDelegate<TInstanceRef()>;
			FInnerDelegate Inner = FInnerDelegate::CreateUObject(Obj, Method, Args...);
			Resolver.BindLambda([InnerCapture = MoveTemp(Inner)]() mutable -> FAsyncReturn
			{
				return WrapInnerExecute(InnerCapture);
			});
			return *this;
		}

		/**
		 * Bind an asynchronous resolver lambda. The lambda receives a TWeakPromise<TInstanceRef>
		 * by value and is responsible for fulfilling or canceling it.
		 */
		template <class TLambda>
			requires std::is_invocable_v<TLambda, TWeakPromise<TInstanceRef>>
		TResolverBindingBuilder& AsyncLambda(TLambda&& InLambda)
		{
			Resolver.BindLambda([UserLambda = Forward<TLambda>(InLambda)]() mutable -> FAsyncReturn
			{
				auto [Promise, Future] = MakeWeakPromisePair<TInstanceRef>();
				UserLambda(MoveTemp(Promise));
				return MoveTemp(Future);
			});
			return *this;
		}

		/** Mark the binding as transient so the resolver runs on every resolve and nothing is cached. */
		TResolverBindingBuilder& AsTransient()
		{
			bTransient = true;
			return *this;
		}

		/**
		 * Fire the resolver as part of finalization, with no external consumer, so the result is
		 * already cached (or in-flight) by the time anyone calls Resolve() on the container. The
		 * resolver runs when the builder is destroyed -- not when this method is called -- so any
		 * further chained modifiers still take effect.
		 */
		TResolverBindingBuilder& ResolveEagerly()
		{
			bResolveEagerly = true;
			return *this;
		}

	private:
		/** Common helper: execute the inner sync delegate if its weak context is still valid; otherwise produce a canceled future. */
		static FAsyncReturn WrapInnerExecute(TDelegate<TInstanceRef()>& Inner)
		{
			if (!Inner.IsBound())
			{
				return MakeCanceledWeakFuture<TInstanceRef>();
			}
			return MakeFulfilledWeakPromise<TInstanceRef>(Inner.Execute()).GetWeakFuture();
		}

		void Finalize()
		{
			if (bFinalized || !Resolver.IsBound())
			{
				return;
			}
			bFinalized = true;
			if (bResolveEagerly && bTransient)
			{
				UE_LOG(LogDependencyInjection, Warning,
					TEXT("Resolver binding %s combines ResolveEagerly() with AsTransient(); the eager resolve discards its result and the next Resolve() will still invoke the factory. Sidestep this warning by manually resolving once after binding."),
					*BindingId.ToString());
			}
			TSharedRef<TResolverBinding<T>> Binding = MakeShared<TResolverBinding<T>>(MoveTemp(BindingId), MoveTemp(Resolver), bTransient);
			DiContainer->BindSpecific(Binding, ConflictBehavior);
			if (bResolveEagerly)
			{
				(void)Binding->Resolve();
			}
		}

		TDiContainer* DiContainer;
		FBindingId BindingId;
		EBindConflictBehavior ConflictBehavior;
		FResolverDelegate Resolver;
		bool bTransient = false;
		bool bResolveEagerly = false;
		bool bFinalized = false;
	};
}
