#ifndef DZARRAYELEMENT_H
#define DZARRAYELEMENT_H

#include "DzValue.h"

class IteratorType;

class DzArrayElement : public DzValue
{
	public:
		DzArrayElement(const IteratorType *iteratorType, size_t index, DzValue *next);

		std::vector<DzResult> build(const EntryPoint &entryPoint, Stack values) const override;

	private:
		const IteratorType *m_iteratorType;

		size_t m_index;

		DzValue *m_next;
};
#endif // DZARRAYELEMENT_H
