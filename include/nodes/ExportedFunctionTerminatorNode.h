#ifndef EXPORTEDFUNCTIONTERMINATORNODE_H
#define EXPORTEDFUNCTIONTERMINATORNODE_H

#include "Node.h"

class ExportedFunctionTerminatorNode : public Node
{
	public:
		std::vector<DzResult> build(const EntryPoint &entryPoint, Stack values) const override;
};

#endif // EXPORTEDFUNCTIONTERMINATORNODE_H
