using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Antlr4.Runtime;
using Antlr4.Runtime.Misc;

namespace dz
{
    class DzValue
    {
    }

    class DzFunctionCall : DzValue
    {
		private readonly string name;
        private readonly DzValue parent;
        private readonly List<DzValue> arguments;

        public DzFunctionCall(string name
			, DzValue parent
			)
        {
			this.name = name;
            this.parent = parent;
			this.arguments = new List<DzValue>();
        }

		public void AddArgument(DzValue argument)
		{
			this.arguments.Add(argument);
		}

		public override string ToString()
		{
			var arguments = this.arguments
				.Append(this.parent)
				.Where(x => x != null)
				.Select(x => x.ToString());

			return $"{this.name}({string.Join(", ", arguments)})";
		}
    }

	class DzParameter : DzValue
	{
		private readonly string name;

		public DzParameter(string name)
		{
			this.name = name;
		}

		public override string ToString()
		{
			return this.name;
		}
	}

	class DzClosure : DzValue
	{
		private static int s_id;

		private readonly DzValue parent;

		public DzParameter Parameter { get; }

		public DzClosure(DzValue parent)
		{
			this.parent = parent;

			this.Parameter = new DzParameter($"_{s_id++}");
		}

		public override string ToString()
		{
			return $"{this.Parameter} => {this.parent}";
		}
	}

	class DzBinary : DzValue
	{
		public DzValue Left { get; set; }
		public DzValue Right { get; set; }

		public override string ToString()
		{
			return $"{this.Left} + {this.Right}";
		}
	}

	class DzMember : DzValue
	{
		private readonly string name;

		public DzMember(string name)
		{
			this.name = name;
		}

        public override string ToString()
        {
            return this.name;
        }
    }

	class DzConstant : DzValue
	{
		private readonly string value;

		public DzConstant(string value)
		{
			this.value = value;
		}

        public override string ToString()
        {
            return this.value;
        }
	}

	class DzReturn : DzValue
	{
		private readonly DzValue value;

		public DzReturn(DzValue value)
		{
			this.value = value;
		}

        public override string ToString()
        {
            return $"return {this.value};";
        }
    }

    class Visitor : dzBaseVisitor<(DzValue, DzValue)>
    {
        private readonly DzValue parent;
		private readonly DzValue deepest;

        public Visitor(DzValue parent, DzValue deepest)
        {
            this.parent = parent;
			this.deepest = deepest;
        }

        private static (IEnumerable<DzValue>, DzValue) Fold((IEnumerable<DzValue> Arguments, DzValue Consumer) accumulator
            , dzParser.ExpressionContext expression
            )
        {
			var visitor = new Visitor(accumulator.Consumer, accumulator.Consumer);
            var (argument, function) = visitor.Visit(expression);

            return (accumulator.Arguments.Append(argument), function);
        }

        public override (DzValue, DzValue) VisitCall(dzParser.CallContext context)
        {
			var closure = new DzClosure(this.deepest);
			var call = new DzFunctionCall(context.ID().GetText(), closure);

			var (arguments, producer) = context.expression()
				.Aggregate((Enumerable.Empty<DzValue>(), (DzValue)call), Fold);

			foreach (var argument in arguments)
			{
				call.AddArgument(argument);
			}

			return (closure.Parameter, producer);
        }

		public override (DzValue, DzValue) VisitFunction(dzParser.FunctionContext context)
		{
			return this.Visit(context.block());
		}

        public override (DzValue, DzValue) VisitRet([NotNull] dzParser.RetContext context)
        {
			var call = new DzFunctionCall("consumer", null);
			var visitor = new Visitor(call, call);

            var (value, f) = visitor.Visit(context.value);

			call.AddArgument(value);

			var ret = new DzReturn(f);

			return (ret, f);
        }

		public override (DzValue, DzValue) VisitBlock(dzParser.BlockContext context)
		{
			return this.Visit(context.ret());
		}

		public override (DzValue, DzValue) VisitProgram(dzParser.ProgramContext context)
		{
			foreach (var function in context.function())
			{
				var (f, q) = this.Visit(function);

				Console.WriteLine($"{function.ID().GetText()}:");
				Console.WriteLine(f);
				Console.WriteLine(q);
			}

			return this.DefaultResult;
		}

		public override (DzValue, DzValue) VisitBinary(dzParser.BinaryContext context)
		{
			var binary = new DzBinary();

			var rightVisitor = new Visitor(binary, this.deepest);
			var (right, f) = rightVisitor.Visit(context.right);

			var leftVisitor = new Visitor(f, f);
			var (left, k) = leftVisitor.Visit(context.left);

			binary.Left = left;
			binary.Right = right;

			return (binary, k);
		}

        public override (DzValue, DzValue) VisitMember([NotNull] dzParser.MemberContext context)
        {
			var member = new DzMember(context.ID().GetText());

            return (member, this.deepest);
        }

        public override (DzValue, DzValue) VisitConstant([NotNull] dzParser.ConstantContext context)
        {
			var constant = new DzConstant(context.INT().GetText());

			return (constant, this.deepest);
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            using (var stream = new FileStream("main.dz", FileMode.Open))
            {
                var input = new AntlrInputStream(stream);
                var lexer = new dzLexer(input);

                var tokens = new CommonTokenStream(lexer);
                var parser = new dzParser(tokens);

                var visitor = new Visitor(null, null);

				var program = parser.program();

				visitor.Visit(program);
			}
		}
	}
}
