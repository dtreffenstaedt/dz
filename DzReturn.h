#ifndef DZRETURN_H
#define DZRETURN_H

#include "DzValue.h"
#include "IteratorTypeHandle.h"

class DzReturn : public DzValue
{
	public:
		DzReturn(const IteratorType *iteratorType
			, DzValue *consumer
			, DzValue *chained
			);

		std::vector<DzResult> build(const EntryPoint &entryPoint, Stack values) const override;

	private:
		const IteratorType *m_iteratorType;

		DzValue *m_consumer;
		DzValue *m_chained;
};

#endif // DZRETURN_H
