// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "Container/Binding.h"
#include "WeakFuture.h"
#include "DiContainerConcept.h"
#include "ResolveErrorBehavior.h"
#include "Tentacle.h"
#include "Blueprint/BlueprintExceptionInfo.h"

namespace DI
{
	/**
	 * DiContainer agnostic implementation of common resolving operations.
	 * This helps in keeping the number of functions to be implemented for a DiContainer type to be very minimal
	 * to allow variations without resorting to inheritance.
	 */
	template <class TDiContainer>
	class TResolveHelper
	{
	public:
		TResolveHelper(const TDiContainer& DiContainer) : DiContainer(DiContainer)
		{
		}

		/**
		 * Try to find a UObject derived binding from the given Class and Binding Name
		 * Primarily intended for blueprint internal use.
		 * @param ObjectType - the class that was used to register the UObject
		 * @param BindingName - optional name of the binding
		 * @param ErrorBehavior - specified what to do if the binding is not found.
		 * @return The UObject that is an instance of ObjectType, if it is bound.
		 */
		TObjectPtr<UObject> TryGetUObjectByClass(
			UClass* ObjectType,
			FName BindingName,
			EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			FBindingId BindingId = FBindingId(FTypeId(ObjectType), BindingName);
			return this->Get<UObject>(BindingId, ErrorBehavior);
		}

		/**
		 * Primarily for blueprint internal usage.
		 * @param StructType - struct class of the UStruct.
		 * @param OutStructMemory - buffer where the struct should be written.
		 * @param BindingName - (Optional) Name of the struct binding
		 * @param ErrorBehavior - specified what to do if the binding is not found.
		 * @return
		 */
		bool TryGetUStruct(
			UScriptStruct* StructType,
			void* OutStructMemory,
			FName BindingName,
			EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			FBindingId BindingId = FBindingId(FTypeId(StructType), BindingName);

			static_assert(
				TIsDerivedFrom<TBindingType<FHitResult>, DI::FRawDataBinding>::IsDerived,
				"This code assumes that UStruct bindings inherit from FRawDataBinding"
			);
			if (TSharedPtr<DI::FRawDataBinding> BindingInstance = StaticCastSharedPtr<DI::FRawDataBinding>(DiContainer.FindBinding(BindingId)))
			{
				BindingInstance->CopyRawData(OutStructMemory, StructType->GetStructureSize());
				return true;
			}
			else
			{
				HandleResolveError(BindingId, ErrorBehavior);
			}
			return false;
		}

		/**
		 * Try to resolve a single instance by type.
		 * @code
		 * ChildContainer->Resolve().TryGet<USimpleUService>(DI::EResolveErrorBehavior::AssertCheck)
		 * @endcode
		 * @tparam T type of the binding that it was bound with.
		 * @param ErrorBehavior - specified what to do if the binding is not found.
		 * @return
		 */
		template <class T>
		DI::TBindingInstPtr<T> TryGet(EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			FBindingId BindingId = MakeBindingId<T>();
			return this->Get<T>(BindingId, ErrorBehavior);
		}

		/**
		 * Try to resolve multiple instances based on their type only.
		 * @code
		 * auto [ResolvedUService, ResolvedUInterface] = DiContainer.Resolve().TryGetMany<USimpleUService, ISimpleInterface>();
		 * @endcode
		 * @tparam Ts - Types of the bindings that they were bound with. Only exact class matches can be resolved.
		 * @param ErrorBehavior - specified what to do if any of the bindings are not found.
		 * @return The bindings in the same order as the types. Failed lookups will have null values.
		 */
		template <class... Ts>
		TTuple<DI::TBindingInstPtr<Ts>...> TryGetMany(EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return MakeTuple(this->Get<Ts>(MakeBindingId<Ts>(), ErrorBehavior)...);
		}

		/**
		 * Try to resolve an instance based on its type only.
		 * @code
		 * DiContainer.Resolve().TryGetNamed<USimpleUService>("SomeName", DI::EResolveErrorBehavior::ReturnNull)
		 * @endcode
		 * @tparam T - Type of the binding that it was bound with. Only exact class matches can be resolved.
		 * @param ErrorBehavior - specified what to do if the binding is not found.
		 * @return The bindings in the same order as the types. Failed lookups will have null values.
		 */
		template <class T>
		DI::TBindingInstPtr<T> TryGetNamed(const FName& BindingName, EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			FBindingId BindingId = MakeBindingId<T>(BindingName);
			return this->Get<T>(BindingId, ErrorBehavior);
		}

		template <class T>
		using TSubscriptionDelegateType = TDelegate<void(TBindingInstRef<T>)>;

		/**
		 * Asynchronously resolve a type instance.
		 * When calling next on the returned future keep in mind the types that are used for the different binding types (see TBindingInstRef<T>)
		 * Example usage:
		 * @code
		 *  ChildContainer->Resolve().WaitFor<USimpleUService>()
		 *  .Next([this](TOptional<TObjectPtr<USimpleUService>> ResolvedService)
		 *  {
		 *       check(ResolvedService.IsSet());
		 *       (*ResolvedService)->DoSomething();
		 *  });
		 * @endcode
		 * @see TBindingInstRef
		 * @tparam T - Type of the binding that it was bound with. Only exact class matches can be resolved.
		 * @param WaitingObject
		 * @param ErrorBehavior - specified what to do if the binding is not found.
		 * @return A Weak Future that completes once the dependency is bound or the container is dropped.
		 */
		template <class T>
		TWeakFuture<TBindingInstRef<T>> WaitFor(
			UObject* WaitingObject = nullptr,
			EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return this->WaitForNamed<T>(NAME_None, WaitingObject, ErrorBehavior);
		}

		/**
		 * Asynchronously resolve type instances.
		 * When calling AndThenExpand on the returned set keep in mind the different types that are used for the different binding types (see TBindingInstRef<T>)
		 * Example usage:
		 * @code
		 *  DiContainer.Resolve().WaitForMany<USimpleUService, ISimpleInterface,>()
		 *  .AndThenExpand([](TOptional<TObjectPtr<USimpleUService>> ObjectService, TOptional<TScriptInterface<ISimpleInterface>> InterfaceService)
		 *  {
		 *      check(ResolvedService.IsSet());
		 *      (*ResolvedService)->DoSomething();
		 *  });
		 * @endcode
		 *
		 * Full Example usage:
		 * @code
		 *  DiContainer.Resolve().WaitForMany<USimpleUService, ISimpleInterface, FSimpleUStructService, FSimpleNativeService>()
		 *	.AndThenExpand([](TOptional<TObjectPtr<USimpleUService>> ObjectService, TOptional<TScriptInterface<ISimpleInterface>> InterfaceService, TOptional<const FSimpleUStructService&> StructService, TOptional<TSharedRef<FSimpleNativeService>> NativeService)
		 *	{
		 *		check(ResolvedService.IsSet());
		 *  	(*ResolvedService)->DoSomething();
		 *  });
		 * @endcode
		 * @see TBindingInstRef
		 * @tparam Ts - Types of the bindings that they were bound with. Only exact class matches can be resolved.
		 * @param WaitingObject - (Optional) The UObject that is putting forward this request.
		 * Used for printing debug logs and valid checking in case the requesting object is deleted before the bindings are resolved.
		 * @param ErrorBehavior - specified what to do if any of the bindings are not found.
		 * @return A Weak Future Set that completes once all binding requests have been completed or once the container is dropped.
		 */
		template <class... Ts>
		TWeakFutureSet<TBindingInstRef<Ts>...> WaitForMany(
			UObject* WaitingObject = nullptr,
			EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return AwaitAllWeak(this->WaitFor<Ts>(WaitingObject, ErrorBehavior)...);
		}

		/**
		 * Asynchronously resolve named type instances.
		 * When calling AndThenExpand on the returned set keep in mind the different types that are used for the different binding types (see TBindingInstPtr<T>)
		 * Simple Example usage:
		 * @code
		 *  DiContainer.Resolve().WaitForManyNamed<USimpleUService, ISimpleInterface,>()
		 *  .AndThenExpand([](TObjectPtr<USimpleUService> ObjectService, TScriptInterface<ISimpleInterface> InterfaceService)
		 *  {
		 *      check(ResolvedService.IsSet());
		 *      (*ResolvedService)->DoSomething();
		 *  });
		 * @endcode
		 *
		 * Full Example usage:
		 * @code
		 *  DiContainer.Resolve().WaitForManyNamed<USimpleUService, ISimpleInterface, FSimpleUStructService, FSimpleNativeService>(this, "Binding1", "Binding2", "Binding3", "Binding4", DI::EResolveErrorBehavior::AssertCheck)
		 *	.AndThenExpand([](TObjectPtr<USimpleUService> ObjectService, TScriptInterface<ISimpleInterface> InterfaceService, TOptional<const FSimpleUStructService&> StructService, TSharedPtr<FSimpleNativeService> NativeService)
		 *	{
		 *		check(ResolvedService.IsSet());
		 *  	(*ResolvedService)->DoSomething();
		 *  });
		 * @endcode
		 * @see TBindingInstRef
		 * @tparam Ts - Types of the bindings that they were bound with. Only exact class matches can be resolved.
		 * @param WaitingObject - (Optional) The UObject that is putting forward this request.
		 * Used for printing debug logs and valid checking in case the requesting object is deleted before the bindings are resolved.
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return A Weak Future Set that completes once all binding requests have been completed or once the container is dropped.
		 */
		template <class... Ts, class... TNames>
		TWeakFutureSet<TBindingInstRef<Ts>...> WaitForManyNamed(UObject* WaitingObject, EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			TTuple<TWeakFuture<TBindingInstRef<Ts>>...> Futures = TTuple<TWeakFuture<TBindingInstRef<Ts>>...>(
				this->template WaitForNamed<Ts>(BindingNames, WaitingObject, ErrorBehavior)...
			);
			return AwaitAllInTuple(MoveTemp(Futures));
			/*
			TWeakFuture<TTuple<TBindingInstPtr<Ts>...>> FuturePointers =
				AwaitAllInTuple(MoveTemp(Futures))
				.Next([](TOptional<TTuple<TOptional<TBindingInstRef<Ts>>...>> Values) -> TTuple<TBindingInstPtr<Ts>...>
				{
					if (!Values.IsSet()) 
						return {};
					
					return TransformTuple(*Values, []<class TBindingRef>(const TOptional<TBindingRef>& Result)
					{
						using T = TBindingInstBaseType<TBindingRef>::Type;
						if (Result.IsSet())
						{
							return TBindingInstPtr<T>(*Result);
						}
						return TBindingInstPtr<T>();
					});
				});
			return TWeakFutureSet<TBindingInstPtr<Ts>...>(MoveTemp(FuturePointers));*/
		}

		template <class... Ts, class... TNames>
		TWeakFutureSet<TBindingInstRef<Ts>...> WaitForManyNamed(UObject* WaitingObject, TNames... BindingNames) const
		{
			return this->WaitForManyNamed(WaitingObject, GDefaultResolveErrorBehavior, BindingNames...);
		}

		/**
		 * Asynchronously resolve a type instance.
		 * When calling AndThenExpand on the returned set keep in mind the different types that are used for the different binding types (see TBindingInstRef<T>)
		 * Example Usage:
		 * @code
		 *  ChildContainer->Resolve().WaitForNamed<FSimpleNativeService>("SimpleName")
		 *  .Next([this](TOptional<TSharedRef<FSimpleNativeService>> ResolvedService)
		 *  {
		 *     check(ResolvedService.IsSet());
		 *     (*ResolvedService)->DoSomething();
		 *  });
		 * @endcode
		 *
		 * @tparam TInstanceType - Type of the binding that it was bound with. Only exact class matches can be resolved.
		 * @param BindingName - Name of the binding.
		 * @param WaitingObject - (Optional) The UObject that is putting forward this request.
		 * Used for printing debug logs and valid checking in case the requesting object is deleted before the bindings are resolved.
		 * @param ErrorBehavior - specified what to do if the binding is not found.
		 * @return A Weak Future that completes once the dependency is bound or the container is dropped.
		 */
		template <class TInstanceType>
		TWeakFuture<TBindingInstRef<TInstanceType>> WaitForNamed(
			const FName& BindingName,
			UObject* WaitingObject = nullptr,
			EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			FBindingId BindingId = MakeBindingId<TInstanceType>(BindingName);
			auto [Promise, Future] = MakeWeakPromisePair<TBindingInstRef<TInstanceType>>();
			TBindingInstPtr<TInstanceType> MaybeInstance = this->Get<TInstanceType>(BindingId, EResolveErrorBehavior::ReturnNull);
			if (MaybeInstance)
			{
				Promise.EmplaceValue(ToRefType(MaybeInstance));
			}
			else
			{
				auto Callback = [Promise](const DI::FBinding& BindingInstance) mutable
				{
					const TBindingType<TInstanceType>& SpecificBinding = static_cast<const TBindingType<TInstanceType>&>(BindingInstance);
					TBindingInstRef<TInstanceType> Resolved = SpecificBinding.Resolve();
					Promise.EmplaceValue(Resolved);
				};
				if (WaitingObject)
				{
					DiContainer.Subscribe(BindingId).AddWeakLambda(WaitingObject, Callback);
				}
				else
				{
					DiContainer.Subscribe(BindingId).AddLambda(Callback);
				}
			}
			auto [NextPromise, NextFuture] = MakeWeakPromisePair<TBindingInstRef<TInstanceType>>();
			Future.Then([BindingId, ErrorBehavior, NextPromise](TWeakFuture<TBindingInstRef<TInstanceType>> FutureInstance) mutable
			{
				if (FutureInstance.WasCanceled())
				{
					HandleResolveError(BindingId, ErrorBehavior);
					NextPromise.Cancel();
					return;
				}
				NextPromise.EmplaceValue(*FutureInstance.Consume());
			});
			return MoveTemp(NextFuture);
		}

	private:
		/**
		 * Private so no one passes in a binding Id that does not match T
		 */
		template <class T>
		DI::TBindingInstPtr<T> Get(const FBindingId& BindingId, EResolveErrorBehavior ErrorBehavior) const
		{
			if (TSharedPtr<DI::FBinding> BindingInstance = DiContainer.FindBinding(BindingId))
			{
				return StaticCastSharedPtr<DI::TBindingType<T>>(BindingInstance)->Resolve();
			}
			HandleResolveError(BindingId, ErrorBehavior);
			return {};
		}

		const TDiContainer& DiContainer;
	};
}
