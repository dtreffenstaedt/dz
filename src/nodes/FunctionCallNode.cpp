#include <iostream>
#include <numeric>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

#include "nodes/FunctionCallNode.h"
#include "EntryPoint.h"
#include "Type.h"
#include "IndexIterator.h"
#include "AllIterator.h"
#include "IRBuilderEx.h"
#include "ZipIterator.h"
#include "ValueHelper.h"
#include "FunctionNotFoundException.h"

#include "nodes/FunctionNode.h"

#include "values/ExpandedValue.h"
#include "values/ScalarValue.h"
#include "values/ReferenceValue.h"
#include "values/UserTypeValue.h"
#include "values/NamedValue.h"
#include "values/ArrayValue.h"
#include "values/ExpandableValue.h"
#include "values/IteratorValue.h"
#include "values/FunctionValue.h"

#include "types/IteratorType.h"

#include "exceptions/InvalidFunctionPointerTypeException.h"
#include "exceptions/AmbiguousFunctionException.h"

#include "iterators/ExtremitiesIterator.h"

FunctionCallNode::FunctionCallNode(antlr4::ParserRuleContext *context, const std::vector<std::string> &names)
	: m_token(TokenInfo::fromContext(context))
	, m_names(names)
{
}

int FunctionCallNode::order(const EntryPoint &entryPoint) const
{
	auto functions = entryPoint.functions();

	for (auto &name : m_names)
	{
		auto iterator = functions.find(name);

		if (iterator != functions.end())
		{
			if (iterator->second->attribute() == FunctionAttribute::Iterator)
			{
				return 1;
			}
		}
	}

	return -1;
}

std::vector<DzResult> FunctionCallNode::build(const EntryPoint &entryPoint, Stack values) const
{
	auto function = entryPoint.function();
	auto block = entryPoint.block();

	insertBlock(block, function);

	for (auto &name : m_names)
	{
		auto tailCallCandidate = entryPoint
			.byName(name);

		if (!tailCallCandidate)
		{
			continue;
		}

		auto targetValues = tailCallCandidate->values();
		auto inputValues = values;

		if (targetValues.size() != inputValues.size())
		{
			continue;
		}

		int8_t min = 0;
		int8_t max = 0;

		std::transform(targetValues.begin(), targetValues.end(), inputValues.begin(), extremities_iterator(min, max), [=](auto storage, auto value)
		{
			auto storageType = storage->type();
			auto valueType = value->type();

			return valueType->compatibility(storageType, entryPoint);
		});

		if (min < 0 || max > 0)
		{
			continue;
		}

		auto tailCallTarget = findTailCallTarget(tailCallCandidate, inputValues);

		if (!tailCallTarget)
		{
			continue;
		}

		auto zipped = zip(inputValues, targetValues);

		auto resultEntryPoint = std::accumulate(zipped.begin(), zipped.end(), entryPoint, [&](auto accumulatedEntryPoint, auto result)
		{
			auto [value, storage] = result;

			return ValueHelper::transferValue(accumulatedEntryPoint, value, storage);
		});

		linkBlocks(resultEntryPoint.block(), tailCallTarget->block());

		return std::vector<DzResult>();
	}

	return regularCall(entryPoint, values);
}

const CallableNode *FunctionCallNode::findFunction(const EntryPoint &entryPoint, Stack values) const
{
	auto functions = entryPoint.functions();
	auto locals = entryPoint.locals();

	for (auto &name : m_names)
	{
		auto local = locals.find(name);

		if (local != locals.end())
		{
			auto value = dynamic_cast<const FunctionValue *>(local->second);

			if (!value)
			{
				throw new InvalidFunctionPointerTypeException(m_token, name);
			}

			return value->function();
		}

		std::map<int8_t, CallableNode *> candidates;

		for (auto [i, end] = functions.equal_range(name); i != end; i++)
		{
			auto function = i->second;

			auto score = function->signatureCompatibility(entryPoint, values);

			if (score < 0)
			{
				continue;
			}

			auto candidate = candidates.find(score);

			if (candidate != candidates.end())
			{
				std::vector<CallableNode *> functions = { candidate->second, function };

				throw new AmbiguousFunctionException(m_token
					, functions
					, entryPoint
					);
			}

			candidates[score] = function;
		}

		if (candidates.size() > 0)
		{
			auto [_, function] = *candidates.begin();

			return function;
		}
	}

	return nullptr;
}

std::vector<DzResult> FunctionCallNode::regularCall(const EntryPoint &entryPoint, Stack values) const
{
	auto &context = entryPoint.context();

	auto functions = entryPoint.functions();
	auto locals = entryPoint.locals();

	auto block = entryPoint.block();

	auto function = findFunction(entryPoint, values);

	if (function)
	{
		auto functionBlock = llvm::BasicBlock::Create(*context);

		linkBlocks(block, functionBlock);

		auto functionEntryPoint = entryPoint
			.withBlock(functionBlock);

		if (function->attribute() == FunctionAttribute::Import)
		{
			return function->build(functionEntryPoint, values);
		}

		std::vector<DzResult> result;

		auto functionResults = function->build(functionEntryPoint, values);

		for (const auto &[lastEntryPoint, returnValue] : functionResults)
		{
			auto consumerBlock = llvm::BasicBlock::Create(*context);

			linkBlocks(lastEntryPoint.block(), consumerBlock);

			auto consumerEntryPoint = functionEntryPoint
				.withDepth(lastEntryPoint.depth())
				.withBlock(consumerBlock);

			result.push_back({ consumerEntryPoint, returnValue });
		}

		return result;
	}

	throw new FunctionNotFoundException(m_token, m_names[0], values);
}
