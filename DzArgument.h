#ifndef DZARGUMENT_H
#define DZARGUMENT_H

#include <string>

#include <llvm/IR/Type.h>

class DzTypeName;
class EntryPoint;

class DzArgument
{
	public:
		DzArgument(const std::string &name, DzTypeName *type);

		std::string name() const;

		llvm::Type *type(const EntryPoint &entryPoint) const;

	private:
		std::string m_name;

		DzTypeName *m_type;
};

#endif // DZARGUMENT_H
