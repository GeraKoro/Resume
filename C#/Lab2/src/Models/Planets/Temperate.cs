using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;

public class Temperate : Planet
{
    public Temperate(string name) : base(name) { }

    public override string Type => nameof(Temperate);

    public override IReadOnlyList<RawExtractionNumber> Yield { get; } =
    [
        new RawExtractionNumber(new RawProduct("Microorganisms"), 1500),
        new RawExtractionNumber(new RawProduct("Autotrophs"), 1500),
        new RawExtractionNumber(new RawProduct("ComplexOrganisms"), 1000),
    ];
}