#pragma once

#include <assert.h>
#include <functional>

template <typename T>
class Delegate {};

template <typename R, typename... Args>
class Delegate<R(Args...)>
{
	typedef R(*ProxyFunction)(void*, Args...);

	template <R(*Function)(Args...)>
	static inline R FunctionProxy(void*, Args... args)
	{
		return Function(std::forward<Args>(args)...);
	}

	template <class C, R(C::*Function)(Args...)>
	static inline R MethodProxy(void* instance, Args... args)
	{
		return (static_cast<C*>(instance)->*Function)(std::forward<Args>(args)...);
	}

	template <class C, R(C::*Function)(Args...) const>
	static inline R ConstMethodProxy(void* instance, Args... args)
	{
		return (static_cast<const C*>(instance)->*Function)(std::forward<Args>(args)...);
	}

public:
	Delegate()
		: m_instance(nullptr)
		, m_proxy(nullptr)
	{
	}

	template <R(*Function)(Args...)>
	void Bind(void)
	{
		m_instance = nullptr;
		m_proxy = &FunctionProxy<Function>;
	}

	template <class C, R(C::*Function)(Args...)>
	void Bind(C* instance)
	{
		m_instance = instance;
		m_proxy = &MethodProxy<C, Function>;
	}

	template <class C, R(C::*Function)(Args...) const>
	void Bind(const C* instance)
	{
		m_instance = const_cast<C*>(instance);
		m_proxy = &ConstMethodProxy<C, Function>;
	}

	R Invoke(Args... args) const
	{
		assert((m_proxy != nullptr) && "Cannot invoke unbound Delegate. Call Bind() first.");
		return m_proxy(m_instance, std::forward<Args>(args)...);
	}

private:
	void* m_instance;
	ProxyFunction m_proxy;
};
