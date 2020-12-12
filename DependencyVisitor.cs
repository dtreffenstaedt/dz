using System.Collections.Generic;
using System.Linq;
using Antlr4.Runtime.Misc;

namespace dz
{
    class DependencyVisitor : dzBaseVisitor<IEnumerable<string>>
    {
        protected override IEnumerable<string> DefaultResult => Enumerable.Empty<string>();

        public override IEnumerable<string> VisitCall([NotNull] dzParser.CallContext context)
        {
            yield return context.ID().GetText();

            foreach (var call in this.VisitChildren(context))
            {
                yield return call;
            }
        }

        protected override IEnumerable<string> AggregateResult(IEnumerable<string> aggregate, IEnumerable<string> nextResult)
        {
            return aggregate.Concat(nextResult);
        }
    }
}