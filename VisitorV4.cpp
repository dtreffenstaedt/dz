#include <numeric>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>

#include "VisitorV4.h"
#include "EntryPoint.h"
#include "DzFunction.h"
#include "DzExportedFunctionTerminator.h"
#include "DzExportedFunction.h"
#include "DzTerminator.h"
#include "DzIntegralLiteral.h"
#include "DzBinary.h"
#include "DzFunctionCall.h"
#include "DzMemberAccess.h"
#include "DzArgument.h"
#include "DzTypeName.h"
#include "DzConditional.h"
#include "DzReturn.h"
#include "DzBooleanLiteral.h"
#include "DzStringLiteral.h"
#include "DzImportedFunction.h"
#include "DzInstantiation.h"
#include "DzGlobal.h"
#include "DzGlobalTerminator.h"
#include "IndexIterator.h"
#include "DefaultPrototypeProvider.h"
#include "WithPrototypeProvider.h"
#include "StackSegment.h"
#include "DzContinuation.h"
#include "DzExpansion.h"
#include "Indexed.h"
#include "DzArrayElement.h"
#include "DzArrayInit.h"
#include "BlockStackFrame.h"
#include "DzTupleArgument.h"

#include "types/Prototype.h"

#include "values/TypedValue.h"

VisitorV4::VisitorV4(DzValue *alpha, DzValue *beta)
	: m_alpha(alpha)
	, m_beta(beta)
{
}

antlrcpp::Any VisitorV4::visitProgram(dzParser::ProgramContext *context)
{
	auto llvmContext = std::make_unique<llvm::LLVMContext>();
	auto module = std::make_unique<llvm::Module>("dz", *llvmContext);

	std::vector<DzCallable *> roots;
	std::multimap<std::string, DzCallable *> functions;
	std::map<std::string, const BaseValue *> locals;
	std::map<std::string, Prototype *> types;

	for (auto function : context->function())
	{
		auto result = visit(function)
			.as<DzCallable *>();

		if (result->attribute() == FunctionAttribute::Export)
		{
			roots.push_back(result);
		}
		else
		{
			functions.insert({ result->name(), result });
		}
	}

	for (auto structure : context->structure())
	{
		auto result = visit(structure)
			.as<Prototype *>();

		types.insert({ result->tag(), result });
	}

	Stack values;

	EntryPoint entryPoint(0
		, nullptr
		, nullptr
		, nullptr
		, nullptr
		, nullptr
		, nullptr
		, &module
		, &llvmContext
		, "term"
		, functions
		, locals
		, types
		, values
		);

	for (auto global : context->global())
	{
		auto result = visit(global)
			.as<DzGlobal *>();

		for (auto &[_, globalValues] : result->build(entryPoint, values))
		{
			locals.insert({ result->name(), globalValues.require<TypedValue>() });
		}
	}

	for (auto root : roots)
	{
		auto ep = entryPoint
			.withLocals(locals);

		root->build(ep, values);
	}

	module->print(llvm::errs(), nullptr);

	if (verifyModule(*module, &llvm::errs()))
	{
		throw new std::exception();
	}

	return new ModuleInfo(std::move(module), std::move(llvmContext));
}

FunctionAttribute getAttribute(dzParser::FunctionContext *ctx)
{
	if (ctx->attribute)
	{
		auto attribute = ctx->attribute->getText();

		if (attribute == "import")
		{
			return FunctionAttribute::Import;
		}

		if (attribute == "export")
		{
			return FunctionAttribute::Export;
		}

		if (attribute == "recursive")
		{
			return FunctionAttribute::Recursive;
		}

		if (attribute == "iterator")
		{
			return FunctionAttribute::Iterator;
		}
	}

	return FunctionAttribute::None;
}

antlrcpp::Any VisitorV4::visitFunction(dzParser::FunctionContext *context)
{
	std::vector<DzBaseArgument *> arguments;

	for (auto argument : context->argument())
	{
		arguments.push_back(visit(argument));
	}

	auto name = context->name->getText();
	auto block = context->block();

	auto attribute = getAttribute(context);

	if (attribute == FunctionAttribute::Import)
	{
		auto import = new DzImportedFunction(name
			, arguments
			, visit(context->typeName())
			);

		return static_cast<DzCallable *>(import);
	}

	if (attribute == FunctionAttribute::Export)
	{
		auto terminator = new DzExportedFunctionTerminator();

		VisitorV4 visitor(terminator, nullptr);

		auto entryPoint = new DzExportedFunction(name
			, visitor.visit(block)
			, visitor.visit(context->typeName())
			);

		return static_cast<DzCallable *>(entryPoint);
	}

	auto terminator = new DzTerminator(name, attribute);

	VisitorV4 visitor(terminator, nullptr);

	auto function = new DzFunction(attribute
		, name
		, arguments
		, visitor.visit(block)
		);

	return static_cast<DzCallable *>(function);
}

antlrcpp::Any VisitorV4::visitTypeName(dzParser::TypeNameContext *context)
{
	return new DzTypeName(context->getText());
}

antlrcpp::Any VisitorV4::visitStandardArgument(dzParser::StandardArgumentContext *context)
{
	auto argument = new DzArgument(context->ID()->getText()
		, visit(context->typeName())
		);

	return static_cast<DzBaseArgument *>(argument);
}

antlrcpp::Any VisitorV4::visitTupleArgument(dzParser::TupleArgumentContext *context)
{
	auto arguments = context->argument();

	std::vector<DzBaseArgument *> tupleArguments;

	std::transform(begin(arguments), end(arguments), std::back_insert_iterator(tupleArguments), [this](dzParser::ArgumentContext *argument) -> DzBaseArgument *
	{
		return visit(argument);
	});

	auto argument = new DzTupleArgument(tupleArguments);

	return static_cast<DzBaseArgument *>(argument);
}

antlrcpp::Any VisitorV4::visitRet(dzParser::RetContext *context)
{
	if (context->chained)
	{
		auto continuation = visit(context->chained);

		auto ret = new DzReturn(m_alpha, continuation);

		VisitorV4 visitor(ret, nullptr);

		return visitor
			.visit(context->value);
	}

	auto ret = new DzReturn(m_alpha, nullptr);

	VisitorV4 visitor(ret, nullptr);

	return visitor
		.visit(context->value);
}

antlrcpp::Any VisitorV4::visitBlock(dzParser::BlockContext *context)
{
	auto expressions = context->expression();

	auto result = std::accumulate(rbegin(expressions), rend(expressions), visit(context->ret()), [this](DzValue *consumer, dzParser::ExpressionContext *expression)
	{
		auto stackFrame = new BlockStackFrame(consumer);

		VisitorV4 visitor(stackFrame, m_alpha);

		return visitor
			.visit(expression)
			.as<DzValue *>();
	});

	return static_cast<DzValue *>(result);
}

antlrcpp::Any VisitorV4::visitBinary(dzParser::BinaryContext *context)
{
	auto binary = new DzBinary(m_alpha
		, context->OP()->getText()
		);

	VisitorV4 leftVisitor(binary, nullptr);

	auto left = leftVisitor
		.visit(context->left)
		.as<DzValue *>();

	VisitorV4 rightVisitor(left, nullptr);

	auto right = rightVisitor.visit(context->right);

	return static_cast<DzValue *>(right);
}

antlrcpp::Any VisitorV4::visitCall(dzParser::CallContext *context)
{
	auto expression = context->expression();

	auto call = new DzFunctionCall(context->ID()->getText());

	std::vector<DzValue *> values;

	std::transform(begin(expression), end(expression), std::back_insert_iterator(values), [](dzParser::ExpressionContext *parameter)
	{
		VisitorV4 visitor(DzTerminator::instance(), nullptr);

		return visitor
			.visit(parameter)
			.as<DzValue *>();
	});

	auto segment = new StackSegment(values, call, m_alpha);

	return static_cast<DzValue *>(segment);
}

antlrcpp::Any VisitorV4::visitWith(dzParser::WithContext *context)
{
	auto assignments = context->assignment();

	std::unordered_map<std::string, DzValue *> fields;

	std::transform(begin(assignments), end(assignments), std::insert_iterator(fields, std::begin(fields)), [](dzParser::AssignmentContext *assignment)
	{
		VisitorV4 visitor(DzTerminator::instance(), nullptr);

		auto value = visitor
			.visit(assignment->expression())
			.as<DzValue *>();

		return std::make_pair(assignment->field()->ID()->getText(), value);
	});

	auto instantiation = new DzInstantiation(m_alpha
		, WithPrototypeProvider::instance()
		, fields
		);

	return static_cast<DzValue *>(instantiation);

//	auto value = std::accumulate(begin(assignments), end(assignments), static_cast<DzValue *>(instantiation), [](auto consumer, dzParser::AssignmentContext *assignment)
//	{
//		VisitorV4 visitor(consumer, nullptr);

//		return visitor
//			.visit(assignment->expression())
//			.as<DzValue *>();
//	});

//	return static_cast<DzValue *>(value);
}

antlrcpp::Any VisitorV4::visitMember(dzParser::MemberContext *context)
{
	auto access = context->ID();

	auto path = std::accumulate(begin(access) + 1, end(access), (*begin(access))->getText(), [](auto name, antlr4::tree::TerminalNode *node)
	{
		std::stringstream ss;
		ss << name << "." << node->getText();

		return ss.str();
	});

	auto with = context->with();

	if (with)
	{
		auto member = new DzMemberAccess(visit(with), path);

		return static_cast<DzValue *>(member);
	}

	auto member = new DzMemberAccess(m_alpha, path);

	return static_cast<DzValue *>(member);
}

antlrcpp::Any VisitorV4::visitInt32Literal(dzParser::Int32LiteralContext *context)
{
	auto constant = new DzIntegralLiteral(m_alpha
		, DzTypeName::int32()
		, context->INT()->getText()
		);

	return static_cast<DzValue *>(constant);
}

antlrcpp::Any VisitorV4::visitInt64Literal(dzParser::Int64LiteralContext *context)
{
	auto constant = new DzIntegralLiteral(m_alpha
		, DzTypeName::int64()
		, context->INT()->getText()
		);

	return static_cast<DzValue *>(constant);
}

antlrcpp::Any VisitorV4::visitBoolLiteral(dzParser::BoolLiteralContext *context)
{
	auto constant = new DzBooleanLiteral(m_alpha
		, context->BOOL()->getText()
		);

	return static_cast<DzValue *>(constant);
}

antlrcpp::Any VisitorV4::visitStringLiteral(dzParser::StringLiteralContext *context)
{
	auto constant = new DzStringLiteral(m_alpha
		, context->STRING()->getText()
		);

	return static_cast<DzValue *>(constant);
}

antlrcpp::Any VisitorV4::visitUint32Literal(dzParser::Uint32LiteralContext *context)
{
	auto constant = new DzIntegralLiteral(m_alpha
		, DzTypeName::uint32()
		, context->INT()->getText()
		);

	return static_cast<DzValue *>(constant);
}

antlrcpp::Any VisitorV4::visitStructure(dzParser::StructureContext *context)
{
	auto name = context->name->getText();
	auto inputFields = context->field();

	std::vector<PrototypeField> fields;

	std::transform(begin(inputFields), end(inputFields), std::back_insert_iterator(fields), [](dzParser::FieldContext *field) -> PrototypeField
	{
		auto name = field->ID()->getText();

		if (field->expression())
		{
			VisitorV4 visitor(DzTerminator::instance(), nullptr);

			auto defaultValue = visitor
				.visit(field->expression())
				.as<DzValue *>();

			return { name, defaultValue };
		}

		return { name, nullptr };
	});

	std::vector<DzTypeName *> parentTypes;

	auto parentTypeNames = context->typeName();

	std::transform(begin(parentTypeNames), end(parentTypeNames), std::back_insert_iterator(parentTypes), [this](dzParser::TypeNameContext *typeName) -> DzTypeName *
	{
		return visit(typeName);
	});

	return new Prototype(name, fields, parentTypes);
}

antlrcpp::Any VisitorV4::visitInstantiation(dzParser::InstantiationContext *context)
{
	auto assignments = context->assignment();

	std::unordered_map<std::string, DzValue *> fields;

	std::transform(begin(assignments), end(assignments), std::insert_iterator(fields, begin(fields)), [](dzParser::AssignmentContext *assignment)
	{
		VisitorV4 visitor(DzTerminator::instance(), nullptr);

		auto value = visitor
			.visit(assignment->expression())
			.as<DzValue *>();

		return std::make_pair(assignment->field()->ID()->getText(), value);
	});

	auto typeName = visit(context->typeName())
		.as<DzTypeName *>();

	auto prototypeProvider = new DefaultPrototypeProvider(typeName);
	auto instantiation = new DzInstantiation(m_alpha
		, prototypeProvider
		, fields
		);

	return static_cast<DzValue *>(instantiation);

//	auto value = std::accumulate(begin(assignments), end(assignments), static_cast<DzValue *>(instantiation), [](auto consumer, dzParser::AssignmentContext *assignment)
//	{
//		VisitorV4 visitor(consumer, nullptr);

//		return visitor
//			.visit(assignment->expression())
//			.as<DzValue *>();
//	});

//	return static_cast<DzValue *>(value);
}

antlrcpp::Any VisitorV4::visitConditional(dzParser::ConditionalContext *context)
{
	VisitorV4 blockVisitor(m_beta, nullptr);

	auto block = blockVisitor
		.visit(context->block())
		.as<DzValue *>();

	auto conditional = new DzConditional(m_alpha, block);

	VisitorV4 expressionVisitor(conditional, nullptr);

	auto condition = expressionVisitor
		.visit(context->expression())
		.as<DzValue *>();

	return condition;
}

antlrcpp::Any VisitorV4::visitGlobal(dzParser::GlobalContext *context)
{
	auto name = context->ID()->getText();

	auto terminator = new DzGlobalTerminator(name);

	VisitorV4 visitor(terminator, nullptr);

	auto literal = visitor
		.visit(context->literal())
		.as<DzValue *>();

	return new DzGlobal(literal, name);
}

antlrcpp::Any VisitorV4::visitNothing(dzParser::NothingContext *context)
{
	UNUSED(context);

	auto constant = new DzIntegralLiteral(m_alpha
		, DzTypeName::without()
		, "0"
		);

	return static_cast<DzValue *>(constant);
}

antlrcpp::Any VisitorV4::visitGroup(dzParser::GroupContext *context)
{
	return visit(context->expression());
}

antlrcpp::Any VisitorV4::visitExpansion(dzParser::ExpansionContext *context)
{
	auto expansion = new DzExpansion(m_alpha);

	VisitorV4 visitor(expansion, nullptr);

	return visitor
		.visit(context->expression())
		.as<DzValue *>();
}

antlrcpp::Any VisitorV4::visitContinuation(dzParser::ContinuationContext *context)
{
	auto continuation = new DzContinuation();

	auto expressions = context->expression();

	auto value = std::accumulate(begin(expressions), end(expressions), static_cast<DzValue *>(continuation), [](DzValue *consumer, dzParser::ExpressionContext *parameter)
	{
		VisitorV4 visitor(consumer, nullptr);

		auto result = visitor
			.visit(parameter)
			.as<DzValue *>();

		return result;
	});

	return value;
}

antlrcpp::Any VisitorV4::visitArray(dzParser::ArrayContext *context)
{
	auto expressions = context->expression();

	std::vector<Indexed<dzParser::ExpressionContext *>> indexed;

	std::transform(rbegin(expressions), rend(expressions), index_iterator(1u), std::back_insert_iterator(indexed), [=](auto x, auto y) -> Indexed<dzParser::ExpressionContext *>
	{
		return { expressions.size() - y, x };
	});

	auto firstElement = std::accumulate(begin(indexed), end(indexed), (DzValue *)nullptr, [&](auto next, Indexed<dzParser::ExpressionContext *> expression)
	{
		auto element = new DzArrayElement(expression.index, m_alpha, next);

		VisitorV4 visitor(element, nullptr);

		return visitor
			.visit(expression.value)
			.as<DzValue *>();
	});

	if (firstElement)
	{
		auto init = new DzArrayInit(firstElement);

		return static_cast<DzValue *>(init);
	}

	return m_alpha;
}
