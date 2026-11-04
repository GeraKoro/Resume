using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;

public class Lava : Planet
{
    public Lava(string name) : base(name) { }

    public override string Type => nameof(Lava);

    public override IReadOnlyList<RawExtractionNumber> Yield { get; } =
    [
        new RawExtractionNumber(new RawProduct("BaseMetals"), 1000),
        new RawExtractionNumber(new RawProduct("HeavyMetals"), 1500),
        new RawExtractionNumber(new RawProduct("FelsicMagma"), 1500),
    ];
}