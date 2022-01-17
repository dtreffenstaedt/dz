#include "UserTypeValue.h"
#include "NamedValue.h"

#include "types/UserType.h"
#include "types/Prototype.h"

UserTypeValue::UserTypeValue(const IPrototype *type, const std::vector<const NamedValue *> &values)
	: m_type(type)
	, m_values(values)
{
}

const Type *UserTypeValue::type() const
{
	std::vector<const Type *> elementTypes;

	std::transform(begin(m_values), end(m_values), std::back_insert_iterator(elementTypes), [](auto value)
	{
		return value->type();
	});

	return UserType::get(m_type->root(), elementTypes);
}

const IPrototype *UserTypeValue::prototype() const
{
	return m_type->root();
}

std::vector<const NamedValue *> UserTypeValue::fields() const
{
	return m_values;
}
