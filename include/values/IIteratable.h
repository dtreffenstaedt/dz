#ifndef IITERATABLE_H
#define IITERATABLE_H

#include "Node.h"

class EntryPoint;

class IIteratable
{
	public:
		virtual std::vector<DzResult> build(const EntryPoint &entryPoint) const = 0;
};

#endif // IITERATABLE_H
