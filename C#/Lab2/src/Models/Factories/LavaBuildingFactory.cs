using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;

public class LavaBuildingFactory : BuildingFactory
{
    protected override string PlanetType => nameof(Lava);

    protected override IReadOnlyList<RawExtractionNumber> Yield { get; } =
    [
        new RawExtractionNumber(new RawProduct("BaseMetals"), 1000),
        new RawExtractionNumber(new RawProduct("HeavyMetals"), 1500),
        new RawExtractionNumber(new RawProduct("FelsicMagma"), 1500),
    ];
}