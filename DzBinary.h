#ifndef DZBINARY_H
#define DZBINARY_H

#include "DzValue.h"

class DzBinary : public DzValue
{
	public:
		DzBinary(DzValue *consumer, const std::string &op);

		std::vector<DzResult> build(const EntryPoint &entryPoint, Stack values) const override;

	private:
		llvm::Value *resolveOp(const EntryPoint &entryPoint, llvm::Value *left, llvm::Value *right) const;

		DzValue * m_consumer;

		std::string m_op;
};

#endif // DZBINARY_H
