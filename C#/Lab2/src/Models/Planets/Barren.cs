using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;

public class Barren : Planet
{
    public Barren(string name) : base(name) { }

    public override string Type => nameof(Barren);

    public override IReadOnlyList<RawExtractionNumber> Yield { get; } =
    [
        new RawExtractionNumber(new RawProduct("BaseMetals"), 1500),
        new RawExtractionNumber(new RawProduct("NobleMetals"), 1500),
        new RawExtractionNumber(new RawProduct("Microorganisms"), 1000),
    ];
}