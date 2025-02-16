#ifndef WITHOUTVALUE_H
#define WITHOUTVALUE_H

#include "BaseValue.h"

#include "metadata/WithoutValueMetadata.h"

class WithoutValue : public BaseValueWithMetadata<WithoutValueMetadata>
{
	public:
		static const WithoutValue *instance();

		const Type *type() const override;
		const BaseValue *clone(const EntryPoint &entryPoint) const override;
};

#endif // WITHOUTVALUE_H
