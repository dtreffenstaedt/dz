#include "DzIteratorFunction.h"
#include "DzBaseArgument.h"
#include "AllIterator.h"
#include "DzFunction.h"

#include "values/LazyValue.h"

DzIteratorFunction::DzIteratorFunction(const IteratorTypeHandle &ith
	, const std::vector<DzBaseArgument *> &arguments
	, const DzFunction *subject
	)
	: m_ith(ith)
	, m_arguments(arguments)
	, m_subject(subject)
{
}

std::vector<DzResult> DzIteratorFunction::build(const EntryPoint &entryPoint, Stack values) const
{
	auto block = entryPoint.block();
	auto function = entryPoint.function();

	block->insertInto(function);

	std::vector<const BaseValue *> v;

	for (auto i = 0u; i < m_arguments.size(); i++)
	{
		v.insert(begin(v), values.pop());
	}

	auto lazy = new LazyValue(m_ith, m_subject, entryPoint, v);

	values.push(lazy);

	return {{ entryPoint, values }};
}

std::string DzIteratorFunction::name() const
{
	return m_subject->name();
}

FunctionAttribute DzIteratorFunction::attribute() const
{
	return FunctionAttribute::Iterator;
}

bool DzIteratorFunction::hasMatchingSignature(const EntryPoint &entryPoint, const Stack &values) const
{
	return m_subject->hasMatchingSignature(entryPoint, values);
}
