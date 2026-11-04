using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;

public class Water : Planet
{
    public Water(string name) : base(name) { }

    public override string Type => nameof(Water);

    public override IReadOnlyList<RawExtractionNumber> Yield { get; } =
    [
        new RawExtractionNumber(new RawProduct("Microorganisms"), 3000),
        new RawExtractionNumber(new RawProduct("ComplexOrganisms"), 1700),
        new RawExtractionNumber(new RawProduct("Water"), 5000),
    ];
}