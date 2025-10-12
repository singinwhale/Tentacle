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
		////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Calls the given function on instance with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		   bool UExampleComponent::InjectDependencies(TObjectPtr<USimpleUService> InSimpleUService);
		   TOptional<bool> bResult = DiContainer.Inject().IntoInstance(*ExampleComponent, &UExampleComponent::InjectDependencies);
		 * @endcode
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @return whatever the passed function returns or unset optional if there was an error.
		 */
		template <class T, class TRetVal, class... TArgs>
		TOptional<TRetVal> IntoInstance(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			auto MaybeResolvedTypes = this->template TryGetFromArgumentTypes<TArgs...>(ErrorBehavior);
			return TryApplyOptionalTuple(MemberFunction, MaybeResolvedTypes, Instance);
		}

		/**
		 * Calls the given function with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		   USimpleUService* InjectDependencies(TObjectPtr<USimpleUService> SimpleUService){
		       return SimpleUService.
		   };
		   TOptional<USimpleUService*> Result = *DiContainer.Inject().IntoStatic(&InjectDependencies);
		 * @endcode
		 * @param StaticFunction - function pointer to a function
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @return whatever the passed function returns or unset optional if there was an error.
		 */
		template <class TRetVal, class... TArgs>
		TOptional<TRetVal> IntoStatic(TRetVal (*StaticFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			auto MaybeResolvedTypes = this->template TryGetFromArgumentTypes<TArgs...>(ErrorBehavior);
			return TryApplyOptionalTuple(*StaticFunction, MaybeResolvedTypes);
		}

		/**
		 * Calls the given lambda with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		   TOptional<void> Ok = DiContainer.Inject().IntoLambda(
		     [&](TObjectPtr<USimpleUService> SimpleUService)
		     {
		       ExampleComponent->InjectDependenciesWithExtraArgs(SimpleUService, ExtraString);
		     }
		  );
		 * @endcode
		 * @param Callable - callable to call with the resolved arguments
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @return whatever the passed function returns or unset optional if there was an error.
		 */
		template <class TCallable>
		auto IntoLambda(TCallable&& Callable, EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			auto MaybeResolvedTypes = this->template ResolveFromArgumentTypesTuple<typename FunctionTraits::TFunctionTraits<TCallable>::ArgsTuple>(ErrorBehavior);
			return TryApplyOptionalTuple(MoveTemp(Callable), MaybeResolvedTypes);
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Calls the given lambda with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		   TOptional<void> Ok = DiContainer.Inject().IntoLambdaNamed(
		     [&](TObjectPtr<USimpleUService> SimpleUService)
		     {
		       ExampleComponent->InjectDependenciesWithExtraArgs(SimpleUService, ExtraString);
		     },
		     DI::EResolveErrorBehavior::AssertCheck,
		     "Service"
		   );
		 * @endcode
		 * @param Callable - callable to call with the resolved arguments
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return TOptional<TRetVal>. i.e. Whatever the passed function returns or unset optional if there was an error.
		 */
		template <class TCallable, class... TNames>
		auto IntoLambdaNamed(TCallable&& Callable, EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			auto MaybeResolvedTypes = this->template ResolveFromArgumentTypesTupleNamed<typename FunctionTraits::TFunctionTraits<TCallable>::ArgsTuple>(ErrorBehavior, BindingNames...);
			return TryApplyOptionalTuple(MoveTemp(Callable), MaybeResolvedTypes);
		}

		/**
		 * Calls the given lambda with the resolved types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * If some of the bindings are not found GDefaultResolveErrorBehavior is used.
		 * Example:
		 * @code
		   TOptional<void> Ok = DiContainer.Inject().IntoLambdaNamed(
		     [&](TObjectPtr<USimpleUService> SimpleUService)
		     {
		       ExampleComponent->InjectDependenciesWithExtraArgs(SimpleUService, ExtraString);
		     },
		     "Service"
		   );
		 * @endcode
		 * @param Callable - callable to call with the resolved arguments
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return TOptional<TRetVal>. i.e. Whatever the passed function returns or unset optional if there was an error.
		 */
		template <class TCallable, class... TNames>
		auto IntoLambdaNamed(TCallable&& Callable, TNames... BindingNames) const
		{
			return this->IntoLambdaNamed(MoveTemp(Callable), GDefaultResolveErrorBehavior, BindingNames...);
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Calls the given function on instance with the resolved named types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		   bool UExampleComponent::InjectDependencies(TObjectPtr<USimpleUService> InSimpleUService);
		   TOptional<bool> Result = DiContainer.Inject().IntoInstanceNamed(*ExampleComponent, &UExampleComponent::InjectDependencies, "SimpleService");
		 * @endcode
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return whatever the passed function returns or unset optional if there was an error.
		 */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoInstanceNamed(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames&&... BindingNames) const
		{
			auto ResolvedTypes = this->template TryGetFromArgumentsNamed<TArgs...>(ErrorBehavior, Forward<TNames>(BindingNames)...);
			return ResolvedTypes.ApplyAfter(MemberFunction, &Instance);
		}

		/**
		 * Calls the given function on instance with the resolved named types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * If some of the bindings are not found GDefaultResolveErrorBehavior is used.
		 * Example:
		 * @code
		   bool UExampleComponent::InjectDependencies(TObjectPtr<USimpleUService> InSimpleUService);
		   TOptional<bool> bResult = DiContainer.Inject().IntoInstanceNamed(*ExampleComponent, &UExampleComponent::InjectDependencies, "SimpleService");
		 * @endcode
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return whatever the passed function returns or unset optional if there was an error.
		 */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoInstanceNamed(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), TNames&&... BindingNames) const
		{
			return this->template IntoInstanceNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, GDefaultResolveErrorBehavior, Forward<TNames>(BindingNames)...);
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Calls the given function with the resolved named types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * Example:
		 * @code
		   TObjectPtr<USimpleUService> InjectDependencies(TObjectPtr<USimpleUService> SimpleUService);
		   TOptional<TObjectPtr<USimpleUService>> Result = DiContainer.Inject().IntoStaticNamed(&InjectDependencies, DI::EResolveErrorBehavior::AssertCheck, "SimpleService");
		 * @endcode
		 * @param StaticFunction - function pointer to a function
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return whatever the passed function returns or unset optional if there was an error.
		 */
		template <class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoStaticNamed(TRetVal (*StaticFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames&&... BindingNames) const
		{
			auto ResolvedTypes = this->template TryGetFromArgumentsNamed<TArgs...>(ErrorBehavior, Forward<TNames>(BindingNames)...);
			return ResolvedTypes.ApplyAfter(StaticFunction);
		}

		/**
		 * Calls the given function with the resolved named types.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 * If some of the bindings are not found GDefaultResolveErrorBehavior is used.
		 * Example:
		 * @code
		   TObjectPtr<USimpleUService> InjectDependencies(TObjectPtr<USimpleUService> SimpleUService);
		   TOptional<TObjectPtr<USimpleUService>> Result = DiContainer.Inject().IntoStaticNamed(&InjectDependencies, "SimpleService");
		 * @endcode
		 * @param StaticFunction - function pointer to a function
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return whatever the passed function returns or unset optional if there was an error.
		 */
		template <class TRetVal, class... TArgs, class... TNames>
		TOptional<TRetVal> IntoStaticNamed(TRetVal (*StaticFunction)(TArgs...), TNames&&... BindingNames) const
		{
			return this->template IntoStaticNamed<TRetVal, TArgs...>(StaticFunction, GDefaultResolveErrorBehavior, Forward<TNames>(BindingNames)...);
		}

		////////////////////////////////////////////////////////////////////////////////////////////


		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool UExampleComponent::Initialize(TObjectPtr<USimpleService> InService);
		   UExampleComponent* Instance;
		   DiContainer.Inject().AsyncIntoUObjectNamed(*Instance, &UExampleComponent::Initialize, DI::EResolveErrorBehavior::AssertCheck, "NameOfDep")
		  		.ThenBindInstance(Instance)
		  		.OrElse([]{
		  			// Handle error
		  		});
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
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
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool UExampleComponent::Initialize(TObjectPtr<USimpleService> InService);
		   UExampleComponent* Instance;
		   DiContainer.Inject().AsyncIntoUObjectNamed(*Instance, &UExampleComponent::Initialize, DI::EResolveErrorBehavior::AssertCheck, "NameOfDep")
		  		.ThenBindInstance(Instance)
		  		.OrElse([]{
		  			// Handle error
		  		});
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoUObjectNamed(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames&&... BindingNames) const
		{
			static_assert(TIsDerivedFrom<T, UObject>::IsDerived, "The AsyncIntoUObject family only works with UObjects. Use the AsyncIntoSP family if you need async execution for native types. Stability cannot be guaranteed without a WeakPtr mechanism");
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoUObjectInternal<T, TRetVal, TArgs...>(Instance, MemberFunction, ErrorBehavior, BindingNames...));
		}

		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool FExampleNative::Initialize(TSharedPtr<FSimpleNativeService> SimpleNative);
		   TSharedRef<FExampleNative> Instance = MakeShared<FExampleNative>()
		   DiContainer.Inject().AsyncIntoUObjectNamed(MakeShared<FExampleNative>(), &FExampleNative::Initialize, "NameOfDep")
		  		.ThenBindInstance(Instance)
		  		.OrElse([]{
		  			// Handle error
		  		});
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoUObjectNamed(T& Instance, TRetVal (T::*MemberFunction)(TArgs...), TNames&&... BindingNames) const
		{
			static_assert(TIsDerivedFrom<T, UObject>::IsDerived, "The AsyncIntoUObject family only works with UObjects. Use the AsyncIntoSP family if you need async execution for native types. Stability cannot be guaranteed without a WeakPtr mechanism");
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoUObjectNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, GDefaultResolveErrorBehavior, BindingNames...));
		}

		////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool FExampleNative::Initialize(TSharedPtr<FSimpleNativeService> SimpleNative);
		   TSharedRef<FExampleNative> Instance = MakeShared<FExampleNative>()
		   DiContainer.Inject().AsyncIntoSP(MakeShared<FExampleNative>(), &FExampleNative::Initialize, DI::EResolveErrorBehavior::AssertCheck, "NameOfDep")
		  		.ThenBindInstance(Instance)
		  		.OrElse([]{
		  			// Handle error
		  		});
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class T, class TRetVal, class... TArgs>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoSP(TSharedRef<T> Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return this->template AsyncIntoSPNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, ErrorBehavior, (TVoid<TArgs>(), NAME_None)...);
		}

		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool FExampleNative::Initialize(TSharedPtr<FSimpleNativeService> SimpleNative);
		   TSharedRef<FExampleNative> Instance = MakeShared<FExampleNative>()
		   DiContainer.Inject().AsyncIntoSPNamed(MakeShared<FExampleNative>(), &FExampleNative::Initialize, DI::EResolveErrorBehavior::AssertCheck, "NameOfDep")
				  .ThenBindInstance(Instance)
				  .OrElse([]{
					  // Handle error
				  });
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoSPNamed(TSharedRef<T> Instance, TRetVal (T::*MemberFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoSPInternal<T, TRetVal, TArgs...>(Instance, MemberFunction, ErrorBehavior, BindingNames...));
		}

		/**
		 * Calls the given function on the instance with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool FExampleNative::Initialize(TSharedPtr<FSimpleNativeService> SimpleNative);
		   TSharedRef<FExampleNative> Instance = MakeShared<FExampleNative>()
		   DiContainer.Inject().AsyncIntoSPNamed(MakeShared<FExampleNative>(), &FExampleNative::Initialize, "NameOfDep")
				  .ThenBindInstance(Instance)
				  .OrElse([]{
					  // Handle error
				  });
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param Instance - the object on which to call MemberFunction
		 * @param MemberFunction - member function pointer to a member function of Instance
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class T, class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoSPNamed(TSharedRef<T> Instance, TRetVal (T::*MemberFunction)(TArgs...), TNames... BindingNames) const
		{
			return this->template AsyncIntoSPNamed<T, TRetVal, TArgs...>(Instance, MemberFunction, GDefaultResolveErrorBehavior, BindingNames...);
		}
		//-------------------------------

		////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Calls the given function with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool InitializeSomething(TSharedPtr<FSimpleNativeService> SimpleNative);
		   DiContainer.Inject().AsyncIntoStatic(&InitializeSomething, DI::EResolveErrorBehavior::AssertCheck, "NameOfDep")
		  		.ThenBindInstance(Instance)
		  		.OrElse([]{
		  			// Handle error
		  		});
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param StaticFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class TRetVal, class... TArgs>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoStatic(TRetVal (*StaticFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior = GDefaultResolveErrorBehavior) const
		{
			return this->template AsyncIntoStaticNamed<TRetVal, TArgs...>(StaticFunction, ErrorBehavior, (TVoid<TArgs>(), NAME_None)...);
		}

		/**
		 * Calls the given function with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool InitializeSomething(TSharedPtr<FSimpleNativeService> SimpleNative);
		   DiContainer.Inject().AsyncIntoStaticNamed(&InitializeSomething, DI::EResolveErrorBehavior::AssertCheck, "NameOfDep")
		  		.ThenBindInstance(Instance)
		  		.OrElse([]{
		  			// Handle error
		  		});
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param StaticFunction - member function pointer to a member function of Instance
		 * @param ErrorBehavior - specifies what to do if any of the bindings are not found.
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoStaticNamed(TRetVal (*StaticFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			return TAfterAsyncInject<TRetVal, TDiContainer>(DiContainer, this->template AsyncIntoStaticInternal<TRetVal, TArgs...>(StaticFunction, ErrorBehavior, BindingNames...));
		}

		/**
		 * Calls the given function with the resolved types when they are fully resolvable.
		 * Keep in mind the different types for the different bindings. For reference see TBindingInstPtr.
		 *
		 * The function will not be called at all if the instance goes out of scope or any argument cannot be resolved.
		 * Use .OrElse(...) to handle such errors.
		 * Example:
		 * @code
		   bool InitializeSomething(TSharedPtr<FSimpleNativeService> SimpleNative);
		   DiContainer.Inject().AsyncIntoStaticNamed(&InitializeSomething, DI::EResolveErrorBehavior::AssertCheck, "NameOfDep")
		  		.ThenBindInstance(Instance)
		  		.OrElse([]{
		  			// Handle error
		  		});
		 * @endcode
		 * @warning If not all the bindings can be resolved, the function will not be called at all!
		 * @param StaticFunction - member function pointer to a member function of Instance
		 * @param BindingNames - List the names of the bindings to resolve. Use NAME_None for type-only bindings.
		 * @return a future for whatever the passed function returns. The future will be canceled if there is an error.
		 */
		template <class TRetVal, class... TArgs, class... TNames>
		TAfterAsyncInject<TRetVal, TDiContainer>
		AsyncIntoStaticNamed(TRetVal (*StaticFunction)(TArgs...), TNames... BindingNames) const
		{
			return this->template AsyncIntoStaticNamed<TRetVal, TArgs...>(StaticFunction, GDefaultResolveErrorBehavior, BindingNames...);
		}

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
		AsyncIntoStaticInternal(TRetVal (*StaticFunction)(TArgs...), EResolveErrorBehavior ErrorBehavior, TNames... BindingNames) const
		{
			auto [OutPromise, OutFuture] = MakeWeakPromisePair<TRetVal>();
			this->DiContainer
				.Resolve()
				.template WaitForManyNamed<typename TBindingInstBaseType<TArgs>::Type...>(nullptr, ErrorBehavior, BindingNames...)
				.AndThenExpand([StaticFunction, OutPromise](TArgs... ResolvedTypes) mutable
				{
					const bool bAllIsResolvedAndValid = (TIsBindingPtrValid<TArgs>::Check(ResolvedTypes) && ... && true);
					if (bAllIsResolvedAndValid)
					{
						OutPromise.EmplaceValue((*StaticFunction)(MoveTempIfPossible(ResolvedTypes)...));
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
