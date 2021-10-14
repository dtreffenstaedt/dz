#include <sstream>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/IRBuilder.h>

#include "DzReturn.h"
#include "EntryPoint.h"
#include "Type.h"
#include "DzExportedFunctionTerminator.h"

DzReturn::DzReturn(DzValue *consumer, DzValue *chained)
	: m_consumer(consumer)
	, m_chained(chained)
{
}

std::vector<DzResult> DzReturn::build(const EntryPoint &entryPoint, Stack values) const
{
	auto &context = entryPoint.context();

	auto name = entryPoint.name();
	auto function = entryPoint.function();
	auto block = entryPoint.block();

	block->setName(name);
	block->insertInto(function);

	auto value = values.pop();

	auto type = value.type();
	auto storageType = type->storageType(*context);

	auto alloc = entryPoint.alloc(storageType);

	llvm::IRBuilder<> builder(block);

	builder.CreateStore(value, alloc);

	auto load = builder.CreateLoad(storageType, alloc);

	values.push({ type, load });

	auto ep = entryPoint
		.withReturnValueAddress(alloc);

	return m_consumer->build(ep, values);
}
