// Copyright singinwhale https://www.singinwhale.com and contributors. Distributed under the MIT license.

#pragma once

#include "CoreMinimal.h"
#include "ResolveErrorBehavior.h"
#include "TentacleTemplates.h"
#include "WeakFuture.h"
#include "OptionalVoid.h"

namespace DI
{
	template <class TFutureValueType, class TDiContainer>
	class TAfterAsyncInject : public TWeakFuture<TFutureValueType>
	{
		using Super = TWeakFuture<TFutureValueType>;

	public:
		TAfterAsyncInject(TDiContainer& InDiContainer, Super&& WrappedFuture)
			: Super(MoveTemp(WrappedFuture))
			  , DiContainer(InDiContainer)
		{
		}

		/**
		 * Binds an instance as its direct type
		 * Keep in mind that when resolving this type, that you need to use the same type as it has been bound with.
		 * Resolving via its parent class is not supported.
		 * If you need to resolve a binding by multiple types, you can bind it to all required types manually.
		 */
		template <class T>
		TEnableIf<!TIsConst<TDiContainer>::Value,
		TWeakFuture<EBindResult>>::Type
		ThenBindInstance(DI::TBindingInstRef<T> Instance, EBindConflictBehavior ConflictBehavior = GDefaultConflictBehavior)
		{
			return this->template ThenBindNamedInstance<T>(Instance, FName(NAME_None), ConflictBehavior);
		}

		/**
		 * Binds a named instance as a specific type
		 * Keep in mind that when resolving this type, that you need to use the same type as it has been bound with.
		 * Resolving via its parent class is not supported.
		 * If you need to resolve a binding by multiple types, you can bind it to all required types manually.
		 */
		template <class TBinding>
		TEnableIf<!TIsConst<TDiContainer>::Value,
		TWeakFuture<EBindResult>>::Type
		ThenBindNamedInstance(DI::TBindingInstRef<TBinding> Instance, const FName& InstanceName, EBindConflictBehavior ConflictBehavior = GDefaultConflictBehavior)
		{
			auto [BindPromise, Future] = MakeWeakPromisePair<EBindResult>();
			this->Then(
				[Promise = MoveTemp(BindPromise), WeakInstance = MakeWeakBindingInstPtr<TBinding>(Instance), DiContainer = DiContainer.AsShared(), InstanceName, ConflictBehavior](
				TWeakFuture<TFutureValueType> CompletedValue) mutable
				{
					if (CompletedValue.WasCanceled())
					{
						Promise.Cancel();
					}
					else
					{
						if (TOptional<TBindingInstRef<TBinding>> ValidInstance = ResolveWeakBindingInstPtr(WeakInstance))
						{
							Promise.EmplaceValue(DiContainer->Bind().template NamedInstance<TBinding>(*ValidInstance, InstanceName, ConflictBehavior));
						}
						else
						{
							Promise.Cancel();
						}
					}
				});
			return MoveTemp(Future);
		}

	private:
		TDiContainer& DiContainer;
	};

	/**
	 * DiContainer agnostic implementation of common injection operations.
	 * This helps in keeping the number of functions to be implemented for a DiContainer type to be very minimal
	 * to allow variations without resorting to inheritance.
	 */
	template <class TDiContainer>
	class TInjector
	{
	public:
		explicit TInjector(TDiContainer& InDiContainer)
			: DiContainer(InDiContainer)
		{
		}

		/**
		 * Calls the given function on instance with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * bool UExampleComponent::InjectDependencies(TObjectPtr<USimpleUService> InSimpleUService);
		 * bool bResult = DiContainer.Inject().IntoFunctionByType(*ExampleComponent, &UExampleComponent::InjectDependencies);
		 * @endcode
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @return whatever the passed function returns.
		 */
		template <class T, class TRetVal, class... TArgs>
		TOptional<TRetVal> IntoFunctionByType(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			auto MaybeResolvedTypes = this->template TryGetFromArgumentTypes<TArgs...>(ErrorBehavior);
			return TryApplyOptionalTuple(MemberFunction, MaybeResolvedTypes, Instance);
		}

		/**
		 * Calls the given function with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * TObjectPtr<USimpleUService> InjectDependencies(TObjectPtr<USimpleUService> SimpleUService){
		 *     return SimpleUService.
		 * };
		 * TObjectPtr<USimpleUService> Result = DiContainer.Inject().IntoFunctionByType(&InjectDependencies);
		 * @endcode
		 * @param FreeFunction - function pointer to a function
		 * @return whatever the passed function returns.
		 */
		template <class TRetVal, class... TArgs>
		TOptional<TRetVal> IntoFunctionByType(TRetVal (*FreeFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			auto MaybeResolvedTypes = this->template TryGetFromArgumentTypes<TArgs...>(ErrorBehavior);
			return TryApplyOptionalTuple(*FreeFunction, MaybeResolvedTypes);
		}

		/**
		 * Calls the given lambda with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * DiContainer.Inject().IntoLambda(
		 *   [&](TObjectPtr<USimpleUService> SimpleUService)
		 *   {
		 *     ExampleComponent->InjectDependenciesWithExtraArgs(SimpleUService, ExtraString);
		 *   }
		 *);
		 * @endcode
		 * @param Callable - callable to call with the resolved arguments
		 * @return whatever the passed function returns.
		 */
		template <class TCallable>
		auto IntoLambda(TCallable&& Callable, EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			auto MaybeResolvedTypes = this->template ResolveFromArgumentTypesTuple<typename FunctionTraits::TFunctionTraits<TCallable>::ArgsTuple>(ErrorBehavior);
			return TryApplyOptionalTuple(MoveTemp(Callable), MaybeResolvedTypes);
		}

		/**
		 * Calls the given lambda with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * DiContainer.Inject().IntoLambda(
		 *   [&](TObjectPtr<USimpleUService> SimpleUService)
		 *   {
		 *     ExampleComponent->InjectDependenciesWithExtraArgs(SimpleUService, ExtraString);
		 *   }
		 *);
		 * @endcode
		 * @param Callable - callable to call with the resolved arguments
		 * @return whatever the passed function returns.
		 */
		template <class TCallable, class... TNames>
		auto IntoLambdaWithNames(TCallable&& Callable, EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			auto MaybeResolvedTypes = this->template ResolveFromArgumentTypesTupleNamed<typename FunctionTraits::TFunctionTraits<TCallable>::ArgsTuple>(ErrorBehavior, BindingNames...);
			return TryApplyOptionalTuple(MoveTemp(Callable), MaybeResolvedTypes);
		}

		/** Version without explicit ErrorBehavior */
		template <class TCallable, class... TNames>
		auto IntoLambdaWithNames(TCallable&& Callable, TNames... BindingNames) const
		{
			return this->IntoLambdaWithNames(MoveTemp(Callable), GDefaultResolveErrorBehavior, BindingNames...);
		}

		/**
		 * Calls the given function on instance with the resolved named types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * bool UExampleComponent::InjectDependencies(TObjectPtr<USimpleUService> InSimpleUService);
		 * bool bResult = DiContainer.Inject().IntoFunctionWithNames(*ExampleComponent, &UExampleComponent::InjectDependencies, "SimpleService");
		 * @endcode
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return whatever the passed function returns.
		 */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoFunctionWithNames(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames&&... BindingNames) const
		{
			auto ResolvedTypes = this->template TryGetFromArgumentsNamed<TArgs...>(ErrorBehavior, Forward<TNames>(BindingNames)...);

			
			
			return ResolvedTypes.ApplyAfter(MemberFunction, &Instance);
		}

		/** Version without explicit ErrorBehavior */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoFunctionWithNames(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), TNames&&... BindingNames) const
		{
			return this->template IntoFunctionWithNames<T, TRetVal, TArgs...>(Instance, MemberFunction, GDefaultResolveErrorBehavior, Forward<TNames>(BindingNames)...);
		}

		/**
		 * Calls the given function with the resolved named types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * TObjectPtr<USimpleUService> InjectDependencies(TObjectPtr<USimpleUService> SimpleUService){
		 *     return SimpleUService.
		 * };
		 * TObjectPtr<USimpleUService> Result = DiContainer.Inject().IntoFunctionByType(&InjectDependencies, DI::GDefaultResolveErrorBehavior, "SimpleService");
		 * @endcode
		 * @param FreeFunction - function pointer to a function
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return whatever the passed function returns.
		 */
		template <class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoFunctionWithNames(TRetVal (*FreeFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames&&... BindingNames) const
		{
			auto ResolvedTypes = this->template TryGetFromArgumentsNamed<TArgs...>(ErrorBehavior, Forward<TNames>(BindingNames)...);
			return ResolvedTypes.ApplyAfter(FreeFunction);
		}

		/** Version without explicit ErrorBehavior */
		template <class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoFunctionWithNames(TRetVal (*FreeFunction)(TArgs...), TNames&&... BindingNames) const
		{
			return this->template IntoFunctionWithNames<TRetVal, TArgs...>(FreeFunction, GDefaultResolveErrorBehavior, Forward<TNames>(BindingNames)...);
		}

		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * bool UExampleComponent::InjectDependencies(TObjectPtr<USimpleUService> InSimpleUService);
		 * bool bResult = DiContainer.Inject().AsyncIntoUObject(*ExampleComponent, &UExampleComponent::InjectDependencies);
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @return whatever the passed function returns.
		 */
		template <class T, class TRetVal, class... TArgs>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoUObject(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return this->template AsyncIntoUObjectNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, ErrorBehavior, (TVoid<TArgs>(), NAME_None)...);
		}

		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * bool FExampleNative::Initialize(TSharedPtr<FSimpleNativeService> SimpleNative);
		 * DiContainer.Inject().AsyncIntoSP(MakeShared<FExampleNative>(), &FExampleNative::Initialize);
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @return whatever the passed function returns.
		 */
		template <class T, class TRetVal, class... TArgs>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoSP(TSharedRef<T> Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return this->template AsyncIntoSPNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, ErrorBehavior, (TVoid<TArgs>(), NAME_None)...);
		}

		/** Version for free functions  */
		template <class TRetVal, class... TArgs>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoStatic(TRetVal (*FreeFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return this->template AsyncIntoStaticNamed<TRetVal, TArgs...>(FreeFunction, ErrorBehavior, (TVoid<TArgs>(), NAME_None)...);
		}

		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		 * bool FExampleNative::Initialize(TSharedPtr<FSimpleNativeService> SimpleNative);
		 * DiContainer.Inject().AsyncIntoUObjectNamed(MakeShared<FExampleNative>(), &FExampleNative::Initialize, "NameOfDep");
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return whatever the passed function returns.
		 */
		//-------------------------------
		template <class T, class TRetVal, class... TArgs, class... TNames>
		typename TEnableIf<!TIsConst<TDiContainer>::Value, TAfterAsyncInject<TRetVal, TDiContainer>>::Type
		AsyncIntoUObjectNamed(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames&&... BindingNames) const
		{
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoUObjectInternal<T, TRetVal, TArgs...>(Instance, MemberFunction, ErrorBehavior, BindingNames...));
		}
		template <class T, class TRetVal, class... TArgs, class... TNames>
		typename TEnableIf<!TIsConst<TDiContainer>::Value, TAfterAsyncInject<TRetVal, TDiContainer>>::Type
		AsyncIntoUObjectNamed(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), TNames&&... BindingNames) const
		{
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoUObjectNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, GDefaultResolveErrorBehavior, BindingNames...));
		}
		//-------------------------------
		
		/** Version for native types referenced via SharedPtr */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoSPNamed(TSharedRef<T> Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoSPInternal<T, TRetVal, TArgs...>(Instance, MemberFunction, ErrorBehavior, BindingNames...));
		}
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoSPNamed(TSharedRef<T> Instance, TRetVal (T::*MemberFunction)(TArgs...), TNames... BindingNames) const
		{
			return this->template AsyncIntoSPNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, GDefaultResolveErrorBehavior, BindingNames...);
		}
		//-------------------------------
		/** Version for free functions */
		template <class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoStaticNamed(TRetVal (*FreeFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoStaticInternal<TRetVal, TArgs...>(FreeFunction, ErrorBehavior, BindingNames...));
		}
		template <class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoStaticNamed(TRetVal (*FreeFunction)(TArgs...), TNames... BindingNames) const
		{
			return this->template AsyncIntoStaticNamed<TRetVal, TArgs...>(FreeFunction, GDefaultResolveErrorBehavior, BindingNames...);
		}
		//-------------------------------

	private:
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TWeakFuture<TRetVal>
		AsyncIntoUObjectInternal(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames&&... BindingNames) const
		{
			auto [RetValPromise, OutFuture] = MakeWeakPromisePair<TRetVal>();
			static_assert((std::convertible_to<TBindingInstRef<typename TBindingInstBaseType<TArgs>::Type>, TArgs> && ...));
			this->DiContainer
				.Resolve()
				.template WaitForManyNamed<typename TBindingInstBaseType<TArgs>::Type...>(&Instance, ErrorBehavior, BindingNames...)
				.AndThenExpand([WeakInstance = MakeWeakObjectPtr(&Instance), MemberFunction, RetValPromise](TArgs... ResolvedBindings) mutable
				{
					T* ValidInstance = WeakInstance.Get();
					const bool bAllIsResolvedAndValid = (TIsBindingPtrValid<TArgs>::Check(ResolvedBindings) && ... && true);
					if (ValidInstance && bAllIsResolvedAndValid)
					{
						FutureDetail::SetPromiseValueFromContinuationResult(RetValPromise,[&](auto&&... Bindings)
						{
							return (*ValidInstance.*MemberFunction)(Bindings...);
						}, MoveTempIfPossible(ResolvedBindings)...);
					}
					else
					{
						RetValPromise.Cancel();
					}
				})
				.OrElse([RetValPromise]() mutable 
				{
					RetValPromise.Cancel();
				});
			return MoveTemp(OutFuture);
		}

		/** Version for native types referenced via SharedPtr */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TWeakFuture<TRetVal>
		AsyncIntoSPInternal(TSharedRef<T> Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			auto [OutPromise, OutFuture] = MakeWeakPromisePair<TRetVal>();
			this->DiContainer
				.Resolve()
				.template WaitForManyNamed<typename TBindingInstBaseType<TArgs>::Type...>(nullptr, ErrorBehavior, BindingNames...)
				.AndThenExpand([WeakInstance = Instance.ToWeakPtr(), MemberFunction, OutPromise](TArgs... ResolvedTypes) mutable
				{
					const bool bAllIsResolvedAndValid = (TIsBindingPtrValid<TArgs>::Check(ResolvedTypes) && ... && true);
					TSharedPtr<T> ValidInstance = WeakInstance.Pin();
					if (ValidInstance && bAllIsResolvedAndValid)
					{
						OutPromise.EmplaceValue(((*ValidInstance).*MemberFunction)(MoveTempIfPossible(ResolvedTypes)...));
					}
					else
					{
						OutPromise.Cancel();
					}
				});
			return MoveTemp(OutFuture);
		}

		/** Version for callables */
		template <class TRetVal, class... TArgs, class... TNames>
		TWeakFuture<TRetVal>
		AsyncIntoStaticInternal(TRetVal (*FreeFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			auto [OutPromise, OutFuture] = MakeWeakPromisePair<TRetVal>();
			this->DiContainer
				.Resolve()
				.template WaitForManyNamed<typename TBindingInstBaseType<TArgs>::Type...>(nullptr, ErrorBehavior, BindingNames...)
				.AndThenExpand([FreeFunction, OutPromise](TArgs... ResolvedTypes) mutable
				{
					const bool bAllIsResolvedAndValid = (TIsBindingPtrValid<TArgs>::Check(ResolvedTypes) && ... && true);
					if (bAllIsResolvedAndValid)
					{
						OutPromise.EmplaceValue((*FreeFunction)(MoveTempIfPossible(ResolvedTypes)...));
					}
					else
					{
						OutPromise.Cancel();
					}
				});
			return MoveTemp(OutFuture);
		}

		template <class... TArgumentTypes>
		auto TryGetFromArgumentTypes(EResolveErrorBehavior ErrorBehavior) const
		{
			return this->template TryGetFromArgumentsNamed<TArgumentTypes...>(ErrorBehavior, (TVoid<TArgumentTypes>(), NAME_None)...);
		}

		template <class TTupleWithArgsType>
		struct TResolveTuple
		{
		};

		template <class... TTupleWithArgsTypes>
		struct TResolveTuple<TTuple<TTupleWithArgsTypes...>>
		{
			static TOptional<TTuple<TTupleWithArgsTypes...>> Resolve(const TInjector& Injector, EResolveErrorBehavior ErrorBehavior)
			{
				return Injector.TryGetFromArgumentTypes<TTupleWithArgsTypes...>(ErrorBehavior);
			}

			static TOptional<TTuple<TTupleWithArgsTypes...>> ResolveNamed(const TInjector& Injector, EResolveErrorBehavior ErrorBehavior, FName BindingNames...)
			{
				return Injector.TryGetFromArgumentsNamed<TTupleWithArgsTypes...>(ErrorBehavior, BindingNames);
			}
		};

		template <class TTupleWithArgsType>
		auto ResolveFromArgumentTypesTuple(EResolveErrorBehavior ErrorBehavior) const
		{
			return TResolveTuple<TTupleWithArgsType>::Resolve(*this, ErrorBehavior);
		}

		template <class TTupleWithArgsType, class... TNames>
		auto ResolveFromArgumentTypesTupleNamed(EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			return TResolveTuple<TTupleWithArgsType>::ResolveNamed(*this, ErrorBehavior, BindingNames...);
		}

		template <class... TArgumentTypes, class... TNames>
		auto TryGetFromArgumentsNamed(EResolveErrorBehavior ErrorBehavior, TNames... Names) const
		{
			auto ResolvedPointers = MakeTuple(DiContainer.Resolve().template TryGetNamed<typename TBindingInstBaseType<TArgumentTypes>::Type>(Names, ErrorBehavior)...);
			return TryDerefAllInstances(ResolvedPointers);
		}

		template<class TCallable, class TOptionalTuple, class... TExtraArgs>
		static auto TryApplyOptionalTuple(TCallable Callable, TOptionalTuple OptionalTuple, TExtraArgs&&... ExtraArgs) // -> TOptional<TRetVal>
		{
			using TRetVal = typename FunctionTraits::TFunctionTraits<TCallable>::ResultType;
			if (OptionalTuple.IsSet())
			{
				if constexpr (!std::is_same_v<TRetVal, void>)
				{
					return TOptional<TRetVal>(OptionalTuple->ApplyAfter(MoveTemp(Callable), MoveTemp(ExtraArgs)...));
				}
				else if constexpr (std::is_same_v<TRetVal, void>)
				{
					OptionalTuple->ApplyAfter(MoveTemp(Callable), MoveTemp(ExtraArgs)...);
					return TOptional<TRetVal>(true);
				}
			}
			return TOptional<TRetVal>();
		}

		TDiContainer& DiContainer;
	};
}
