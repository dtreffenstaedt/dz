#ifndef USERTYPEVALUE_H
#define USERTYPEVALUE_H

#include "BaseValue.h"

class NamedValue;
class IPrototype;

class UserTypeValue : public BaseValue
{
	public:
		UserTypeValue(const IPrototype *type, const std::vector<const NamedValue *> &values);

		const Type *type() const override;

		const IPrototype *prototype() const;

		std::vector<const NamedValue *> fields() const;

	private:
		const IPrototype *m_type;

		std::vector<const NamedValue *> m_values;
};

#endif // USERTYPEVALUE_H
