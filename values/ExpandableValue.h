#ifndef EXPANDABLEVALUE_H
#define EXPANDABLEVALUE_H

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>

#include "values/BaseValue.h"

class Type;
class EntryPoint;
class DzValue;
class IteratorType;

class ExpandableValue : public BaseValue
{
	public:
		ExpandableValue(const IteratorType *iteratorType, const EntryPoint &provider, const DzValue *chain);

		const Type *type() const override;

		const IteratorType *iteratorType() const;
		const EntryPoint *provider() const;
		const DzValue *chain() const;

	private:
		const IteratorType *m_iteratorType;
		const EntryPoint *m_provider;
		const DzValue *m_chain;
};

#endif // EXPANDABLEVALUE_H
