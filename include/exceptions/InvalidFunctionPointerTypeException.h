#ifndef INVALIDFUNCTIONPOINTERTYPEEXCEPTION_H
#define INVALIDFUNCTIONPOINTERTYPEEXCEPTION_H

#include "CompilerException.h"

class InvalidFunctionPointerTypeException : public CompilerException
{
	public:
		InvalidFunctionPointerTypeException(const TokenInfo &token, const std::string &name);

		std::string message() const override;

	private:
		std::string m_name;
};

#endif // INVALIDFUNCTIONPOINTERTYPEEXCEPTION_H
