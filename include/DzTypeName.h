#ifndef DZTYPENAME_H
#define DZTYPENAME_H

#include "ITypeName.h"
#include "TokenInfo.h"

class EntryPoint;
class Type;

namespace antlr4
{
	class ParserRuleContext;
};

class DzTypeName : public ITypeName
{
	public:
		DzTypeName(antlr4::ParserRuleContext *context, const std::vector<std::string> &names);

		Type *resolve(const EntryPoint &entryPoint) const override;

		static DzTypeName *int32();
		static DzTypeName *int64();
		static DzTypeName *uint32();
		static DzTypeName *byte();
		static DzTypeName *boolean();
		static DzTypeName *string();
		static DzTypeName *without();

	private:
		TokenInfo m_token;

		std::vector<std::string> m_names;
};

#endif // DZTYPENAME_H
