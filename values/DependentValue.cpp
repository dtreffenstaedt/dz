#include "DependentValue.h"

#include "types/IteratorType.h"

DependentValue::DependentValue(const Type *iteratorType, const EntryPoint *provider)
	: m_iteratorType(iteratorType)
	, m_provider(provider)
{
}

const Type *DependentValue::type() const
{
	return m_iteratorType;
}

const EntryPoint *DependentValue::provider() const
{
	return m_provider;
}
