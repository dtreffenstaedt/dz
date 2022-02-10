#ifndef DZRETURN_H
#define DZRETURN_H

#include "DzValue.h"

class Type;

class DzReturn : public DzValue
{
	public:
		DzReturn(const Type *iteratorType
			, const DzValue *consumer
			, const DzValue *chained
			);

		std::vector<DzResult> build(const EntryPoint &entryPoint, Stack values) const override;

	private:
		const Type *m_iteratorType;

		const DzValue *m_consumer;
		const DzValue *m_chained;
};

#endif // DZRETURN_H
