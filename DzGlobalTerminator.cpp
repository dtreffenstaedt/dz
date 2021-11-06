#include <llvm/IR/GlobalVariable.h>

#include "DzGlobalTerminator.h"
#include "Type.h"

DzGlobalTerminator::DzGlobalTerminator(const std::string &name)
	: m_name(name)
{
}

int DzGlobalTerminator::compare(DzValue *other, const EntryPoint &entryPoint) const
{
	UNUSED(other);
	UNUSED(entryPoint);

	return -1;
}

std::vector<DzResult> DzGlobalTerminator::build(const EntryPoint &entryPoint, Stack values) const
{
	auto &context = entryPoint.context();
	auto &module = entryPoint.module();

	auto value = values.pop();
	auto valueType = value.type();

	auto storageType = valueType->storageType(*context);

	auto global = module->getOrInsertGlobal(m_name, storageType, [&]
	{
		return new llvm::GlobalVariable(*module, storageType, false, llvm::GlobalValue::InternalLinkage, value.constant(), m_name);
	});

	values.push({ valueType, global });

	return {{ entryPoint, values }};
}
