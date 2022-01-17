#ifndef DZITERATORFUNCTION_H
#define DZITERATORFUNCTION_H

#include "DzCallable.h"
#include "IteratorTypeHandle.h"

class DzBaseArgument;
class DzFunction;

class DzIteratorFunction : public DzCallable
{
	public:
		DzIteratorFunction(const IteratorTypeHandle &ith
			, const std::vector<DzBaseArgument *> &arguments
			, const DzFunction *subject
			);

		std::vector<DzResult> build(const EntryPoint &entryPoint, Stack values) const override;

		std::string name() const override;

		FunctionAttribute attribute() const override;

		bool hasMatchingSignature(const EntryPoint &entryPoint, const Stack &values) const override;

	private:
		IteratorTypeHandle m_ith;

		std::vector<DzBaseArgument *> m_arguments;

		const DzFunction *m_subject;
};

#endif // DZITERATORFUNCTION_H
