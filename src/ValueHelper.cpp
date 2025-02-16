#include <numeric>

#include "ValueHelper.h"
#include "IRBuilderEx.h"
#include "ZipIterator.h"

#include "values/ReferenceValue.h"
#include "values/UserTypeValue.h"
#include "values/ArrayValue.h"
#include "values/LazyValue.h"
#include "values/NamedValue.h"
#include "values/ScalarValue.h"
#include "values/ExpandedValue.h"
#include "values/TupleValue.h"
#include "values/StringValue.h"

EntryPoint ValueHelper::transferValue(const EntryPoint &entryPoint
	, const BaseValue *value
	, const BaseValue *storage
	)
{
	if (auto scalarValue = dynamic_cast<const ScalarValue *>(value))
	{
		auto referenceStorage = dynamic_cast<const ReferenceValue *>(storage);

		if (!referenceStorage)
		{
			// TODO: This will be fixed when array transfer is O(N).
			// Right now, we will always create branches for all possible values.
			// This *can* include "without", for example.

			return entryPoint;
		}

		IRBuilderEx builder(entryPoint);

		builder.createStore(scalarValue, referenceStorage);
	}
	else if (auto reference = dynamic_cast<const ReferenceValue *>(value))
	{
		auto referenceStorage = dynamic_cast<const ReferenceValue *>(storage);

		if (!referenceStorage)
		{
			// TODO: This will be fixed when array transfer is O(N)

			return entryPoint;
		}

		IRBuilderEx builder(entryPoint);

		auto load = builder.createLoad(reference);

		builder.createStore(load, referenceStorage);
	}
	else if (auto userTypeValue = dynamic_cast<const UserTypeValue *>(value))
	{
		auto userTypeStorage = dynamic_cast<const UserTypeValue *>(storage);

		auto valueFields = userTypeValue->fields();
		auto storageFields = userTypeStorage->fields();

		auto zipped = zip(valueFields, storageFields);

		return std::accumulate(zipped.begin(), zipped.end(), entryPoint, [&](auto accumulatedEntryPoint, auto fields)
		{
			auto [valueField, storageField] = fields;

			return transferValue(accumulatedEntryPoint
				, valueField->value()
				, storageField->value()
				);
		});
	}
	else if (auto lazyValue = dynamic_cast<const LazyValue *>(value))
	{
		auto lazyStorage = dynamic_cast<const LazyValue *>(storage);

		return lazyStorage->assignFrom(entryPoint, lazyValue);
	}
	else if (auto tupleValue = dynamic_cast<const TupleValue *>(value))
	{
		auto tupleStorage = dynamic_cast<const TupleValue *>(storage);

		auto valueElements = tupleValue->values();
		auto storageElements = tupleStorage->values();

		auto zipped = zip(valueElements, storageElements);

		return std::accumulate(zipped.begin(), zipped.end(), entryPoint, [&](auto accumulatedEntryPoint, auto elements)
		{
			auto [valueElement, storageElement] = elements;

			return transferValue(accumulatedEntryPoint
				, valueElement
				, storageElement
				);
		});
	}
	else if(auto stringValue = dynamic_cast<const StringValue *>(value))
	{
		auto stringStorage = dynamic_cast<const StringValue *>(stringValue);

		auto addressOf = new ScalarValue(stringValue->type()
			, *stringValue->reference()
			);

		IRBuilderEx builder(entryPoint);

		builder.createStore(addressOf, stringStorage->reference());
	}

	return entryPoint;
}
