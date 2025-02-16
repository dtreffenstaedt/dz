#ifndef RETURNNODE_H
#define RETURNNODE_H

#include "Node.h"

class Type;

class ReturnNode : public Node
{
	public:
		ReturnNode(const Type *iteratorType
			, const Node *consumer
			, const Node *chained
			);

		std::vector<DzResult> build(const EntryPoint &entryPoint, Stack values) const override;

	private:
		const Type *m_iteratorType;

		const Node *m_consumer;
		const Node *m_chained;
};

#endif // RETURNNODE_H
