#include "TupleValue.h"

TupleValue::TupleValue(const IteratorType *iteratorType, const std::vector<const BaseValue *> &values)
	: m_iteratorType(iteratorType)
	, m_values(values)
{
}

size_t TupleValue::size() const
{
	return m_values.size();
}

const Type *TupleValue::type() const
{
	std::vector<const Type *> types;

	std::transform(begin(m_values), end(m_values), std::back_insert_iterator(types), [](auto value)
	{
		return value->type();
	});

	return TupleType::get(m_iteratorType, types);
}

const IteratorType *TupleValue::iteratorType() const
{
	return m_iteratorType;
}

Stack TupleValue::values() const
{
	return m_values;
}
