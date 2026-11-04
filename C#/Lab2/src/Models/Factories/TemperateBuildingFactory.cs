using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;

public class TemperateBuildingFactory : BuildingFactory
{
    protected override string PlanetType => nameof(Temperate);

    protected override IReadOnlyList<RawExtractionNumber> Yield { get; } =
    [
        new RawExtractionNumber(new RawProduct("Microorganisms"), 1500),
        new RawExtractionNumber(new RawProduct("Autotrophs"), 1500),
        new RawExtractionNumber(new RawProduct("ComplexOrganisms"), 1000),
    ];
}