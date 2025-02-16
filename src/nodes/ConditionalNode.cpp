#include <sstream>
#include <iostream>
#include <unordered_map>

#include <llvm/IR/IRBuilder.h>

#include "nodes/ConditionalNode.h"
#include "EntryPoint.h"
#include "IRBuilderEx.h"

#include "values/ScalarValue.h"
#include "values/UserTypeValue.h"
#include "values/ReferenceValue.h"

ConditionalNode::ConditionalNode(const Node *ifFalse, const Node *ifTrue)
	: m_ifTrue(ifTrue)
	, m_ifFalse(ifFalse)
{
}

std::vector<DzResult> ConditionalNode::build(const EntryPoint &entryPoint, Stack values) const
{
	struct SingleResult
	{
		const EntryPoint entryPoint;
		const ScalarValue *value;
	};

	auto &context = entryPoint.context();
	auto &module = entryPoint.module();

	auto function = entryPoint.function();
	auto block = entryPoint.block();

	auto dataLayout = module->getDataLayout();

	block->setName("condition");

	insertBlock(block, function);

	auto ifTrue = llvm::BasicBlock::Create(*context);
	auto ifFalse = llvm::BasicBlock::Create(*context);

	IRBuilderEx builder(entryPoint);

	builder.createCondBr(values.require<ScalarValue>(TokenInfo()), ifTrue, ifFalse);

	auto epIfFalse = entryPoint
		.withName("ifFalse")
		.withBlock(ifFalse);

	auto epIfTrue = entryPoint
		.withName("ifTrue")
		.withBlock(ifTrue);

	auto resultsIfTrue = m_ifTrue->build(epIfTrue, values);
	auto resultsIfFalse = m_ifFalse->build(epIfFalse, values);

	std::vector<DzResult> immediateResults;

	immediateResults.insert(end(immediateResults), begin(resultsIfTrue), end(resultsIfTrue));
	immediateResults.insert(end(immediateResults), begin(resultsIfFalse), end(resultsIfFalse));

	std::multimap<const Type *, SingleResult> groupedResults;

	for (auto &result : immediateResults)
	{
		auto [resultEntryPoint, resultValues] = result;

		if (resultValues.size() != 1)
		{
			continue;
		}

		auto value = resultValues.request<ScalarValue>();

		if (value)
		{
			groupedResults.insert({ value->type(), { resultEntryPoint, value } });
		}
		else
		{
			return immediateResults;
		}
	}

	std::vector<DzResult> mergedResults;

	for(auto it = begin(groupedResults)
		; it != end(groupedResults)
		; it = upper_bound(it, end(groupedResults), *it, &compareKey<const Type *, SingleResult>)
		)
	{
		auto [type, _] = *it;

		auto range = groupedResults.equal_range(type);

		auto alloc = entryPoint.alloc(type);

		auto mergeBlock = llvm::BasicBlock::Create(*context, "merge", function);

		for (auto i = range.first; i != range.second; i++)
		{
			auto [resultEntryPoint, value] = i->second;

			auto resultBlock = resultEntryPoint.block();

			IRBuilderEx resultBuilder(resultEntryPoint);

			resultBuilder.createStore(value, alloc);

			linkBlocks(resultBlock, mergeBlock);
		}

		auto mergeEntryPoint = entryPoint
			.withBlock(mergeBlock);

		IRBuilderEx mergeBuilder(mergeEntryPoint);

		auto mergeLoad = mergeBuilder.createLoad(alloc, "mergeLoad");

		auto mergeValues = values;

		mergeValues.push(mergeLoad);

		mergedResults.push_back({ mergeEntryPoint, mergeValues });
	}

	return mergedResults;
}
