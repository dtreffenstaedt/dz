#include "nodes/EmptyArrayNode.h"

#include "values/WithoutValue.h"

EmptyArrayNode::EmptyArrayNode(const Node *consumer)
	: m_consumer(consumer)
{
}

std::vector<DzResult> EmptyArrayNode::build(const EntryPoint &entryPoint, Stack values) const
{
	values.push(WithoutValue::instance());

	return m_consumer->build(entryPoint, values);
}
