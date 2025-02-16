#ifndef WITHPROTOTYPE_H
#define WITHPROTOTYPE_H

#include "IPrototype.h"
#include "Node.h"

#include "values/UserTypeValue.h"

class ScalarValue;

class WithPrototype : public IPrototype
{
	public:
		WithPrototype(const UserTypeValue *value);

		const IPrototype *root() const override;

		std::string name() const override;
		std::vector<PrototypeField> fields(const EntryPoint &entryPoint) const override;

		llvm::Type *storageType(llvm::LLVMContext &context) const override;

		int8_t compatibility(const Type *type, const EntryPoint &entryPoint) const override;

	private:
		const UserTypeValue *m_value;
};

#endif // WITHPROTOTYPE_H
