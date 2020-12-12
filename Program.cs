using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Antlr4.Runtime;
using Antlr4.Runtime.Misc;
using LLVMSharp.Interop;

namespace dz
{
    class DzValue
    {
    }

    class DzFunctionCall : DzValue
    {
        private readonly DzValue parent;
        private readonly List<DzValue> arguments;

		public string Name { get; }

        public DzFunctionCall(string name
			, DzValue parent
			)
        {
            this.parent = parent;
			this.arguments = new List<DzValue>();

			this.Name = name;
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

			return $"{this.Name}({string.Join(", ", arguments)})";
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
		public DzValue Value { get; set; }

        public override string ToString()
        {
            return $"return {this.Value}";
        }
    }

	class DzTerminator : DzValue
	{
		public DzValue Value { get; set; }

        public override string ToString()
        {
            return this.Value.ToString();
        }
    
	}

    class Visitor : dzBaseVisitor<(DzValue, DzValue)>
    {
		private readonly bool isDependency;
        private readonly DzValue parent;
		private readonly DzValue deepest;
		private readonly LLVMModuleRef module;

        public Visitor(bool isDependency
			, DzValue parent
			, DzValue deepest
			, LLVMModuleRef module
			)
        {
			this.isDependency = isDependency;
            this.parent = parent;
			this.deepest = deepest;
			this.module = module;
        }

        private (IEnumerable<DzValue>, DzValue) Fold((IEnumerable<DzValue> Arguments, DzValue Consumer) accumulator
            , dzParser.ExpressionContext expression
            )
        {
			var visitor = new Visitor(this.isDependency
				, accumulator.Consumer
				, accumulator.Consumer
				, this.module
				);
				
            var (argument, function) = visitor.Visit(expression);

            return (accumulator.Arguments.Append(argument), function);
        }

        public override (DzValue, DzValue) VisitCall(dzParser.CallContext context)
        {
			var ret = new DzReturn
			{
				Value = this.deepest
			};

			var closure = new DzClosure(ret);
			var call = new DzFunctionCall(context.ID().GetText(), closure);

			var (arguments, producer) = context.expression()
				.Aggregate((Enumerable.Empty<DzValue>(), (DzValue)call), this.Fold);

			foreach (var argument in arguments)
			{
				call.AddArgument(argument);
			}

			return (closure.Parameter, producer);
        }

		class TypeHarvester : dzBaseVisitor<LLVMTypeRef>
		{
            public override LLVMTypeRef VisitTypeName([NotNull] dzParser.TypeNameContext context)
            {
				return context.ID().GetText() switch
				{
					"int" => LLVMTypeRef.Int32,
					_ => throw new NotSupportedException()
				};
            }

            protected override LLVMTypeRef AggregateResult(LLVMTypeRef aggregate, LLVMTypeRef nextResult)
            {
                return base.AggregateResult(aggregate, nextResult);
            }
		}

		private IEnumerable<LLVMTypeRef> GetArgumentTypes(dzParser.FunctionContext context)
		{
			var typeHarvester = new TypeHarvester();

			foreach (var argument in context.argument())
			{
				yield return typeHarvester.Visit(argument);
			}

			if (this.isDependency)
			{
				yield return LLVMTypeRef.CreateFunction(LLVMTypeRef.Int32, new[] { LLVMTypeRef.Int32 }, false);	
			}
		}

		public override (DzValue, DzValue) VisitFunction(dzParser.FunctionContext context)
		{
			var name = context.ID().GetText();

			var (value, f) = this.Visit(context.block());

			var parameterTypes = this
				.GetArgumentTypes(context)
				.ToArray();

			var functionType = LLVMTypeRef.CreateFunction(LLVMTypeRef.Int32, parameterTypes, false);
			var prototype = module.AddFunction(name, functionType);

			Console.Write($"int {name}(");

			if (this.isDependency)
			{
				Console.Write("func<int, int> consumer");
			}

			Console.WriteLine(")");
			Console.WriteLine("{");
			Console.Write("\t");
			Console.WriteLine(value);
			Console.WriteLine("}");
			Console.WriteLine();

			return (value, f);
		}

        public override (DzValue, DzValue) VisitRet([NotNull] dzParser.RetContext context)
        {
			if (this.isDependency)
			{
				var call = new DzFunctionCall("consumer", null);
				var visitor = new Visitor(this.isDependency, call, call, this.module);

				var (value, f) = visitor.Visit(context.value);

				call.AddArgument(value);

				var ret = new DzReturn
				{
					Value = f					
				};

				return (ret, f);
			}
			else
			{
				var terminator = new DzTerminator();
				var visitor = new Visitor(this.isDependency, terminator, terminator, this.module);

				var (value, f) = visitor.Visit(context.value);

				terminator.Value = value;

				var ret = new DzReturn
				{
					Value = f
				};

				return (ret, f);
			}
        }

		public override (DzValue, DzValue) VisitBlock(dzParser.BlockContext context)
		{
			return this.Visit(context.ret());
		}

		public override (DzValue, DzValue) VisitProgram(dzParser.ProgramContext context)
		{
			var dependencyVisitor = new DependencyVisitor();

			var dependencies = dependencyVisitor
				.Visit(context)
				.ToHashSet();

			foreach (var function in context.function())
			{
				var name = function.ID().GetText();

				var isDependency = dependencies.Contains(name);

				var visitor = new Visitor(isDependency, null, null, this.module);

				var (f, q) = visitor.Visit(function);
			}

			return this.DefaultResult;
		}

		public override (DzValue, DzValue) VisitBinary(dzParser.BinaryContext context)
		{
			var binary = new DzBinary();

			var rightVisitor = new Visitor(this.isDependency, binary, this.deepest, this.module);
			var (right, f) = rightVisitor.Visit(context.right);

			var leftVisitor = new Visitor(this.isDependency, f, f, this.module);
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
			var module = LLVMModuleRef.CreateWithName("my cool jit");

            using (var stream = new FileStream("main.dz", FileMode.Open))
            {
                var input = new AntlrInputStream(stream);
                var lexer = new dzLexer(input);

                var tokens = new CommonTokenStream(lexer);
                var parser = new dzParser(tokens);

                var visitor = new Visitor(false, null, null, module);

				var program = parser.program();

				visitor.Visit(program);
			}
		}
	}
}
