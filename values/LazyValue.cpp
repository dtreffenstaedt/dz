#include "LazyValue.h"
#include "EntryPoint.h"
#include "DzTypeName.h"

#include "types/IteratorType.h"

LazyValue::LazyValue(const IteratorTypeHandle &handle, const DzValue *subject, const EntryPoint &entryPoint, const Stack &values)
	: m_handle(handle)
	, m_subject(subject)
	, m_entryPoint(new EntryPoint(entryPoint))
	, m_values(values)
{
}

std::vector<DzResult> LazyValue::build(llvm::BasicBlock *block, const Stack &values) const
{
	auto entryPoint = m_entryPoint->withBlock(block);

	return m_subject->build(entryPoint, m_values);
}

const Type *LazyValue::type() const
{
	return m_handle.type();
}
