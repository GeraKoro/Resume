using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;

public class BarrenBuildingFactory : BuildingFactory
{
    protected override string PlanetType => nameof(Barren);

    protected override IReadOnlyList<RawExtractionNumber> Yield { get; } =
    [
        new RawExtractionNumber(new RawProduct("BaseMetals"), 1500),
        new RawExtractionNumber(new RawProduct("NobleMetals"), 1500),
        new RawExtractionNumber(new RawProduct("Microorganisms"), 1000),
    ];
}