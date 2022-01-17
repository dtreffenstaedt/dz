#ifndef TUPLEVALUE_H
#define TUPLEVALUE_H

#include <sstream>
#include <numeric>

#include "BaseValue.h"
#include "Utility.h"
#include "Stack.h"

#include "types/TupleType.h"

class IteratorType;

class TupleValue : public BaseValue
{
	public:
		TupleValue(const IteratorType *iteratorType, const std::vector<const BaseValue *> &values);

		size_t size() const;

		const Type *type() const override;
		const IteratorType *iteratorType() const;

		Stack values() const;

	private:
		const IteratorType *m_iteratorType;

		std::vector<const BaseValue *> m_values;
};

#endif // TUPLEVALUE_H
