#ifndef TYPE_H
#define TYPE_H

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/DerivedTypes.h>

class EntryPoint;

class Type
{
	public:
		virtual std::string name() const = 0;
		virtual std::string fullName() const
		{
			return name();
		}

		virtual llvm::Type *storageType(llvm::LLVMContext &context) const = 0;

		virtual bool is(const Type *type, const EntryPoint &entryPoint) const = 0;
		virtual bool equals(const Type *type, const EntryPoint &entryPoint) const = 0;
};

#endif // TYPE_H
