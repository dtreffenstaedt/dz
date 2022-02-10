#include "LazySink.h"

#include "values/LazyValue.h"

LazySink::LazySink(const Type *iteratorType
	, const DzValue *consumer
	, const DzValue *subject
	)
	: m_iteratorType(iteratorType)
	, m_consumer(consumer)
	, m_subject(subject)
{
}

std::vector<DzResult> LazySink::build(const EntryPoint &entryPoint, Stack values) const
{
	auto lazy = new LazyValue(Stack()
		, entryPoint
		, m_iteratorType
		, m_subject
		);

	values.push(lazy);

	return m_consumer->build(entryPoint, values);
}
