using Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;

public abstract class BuildingFactory : IBuildingFactory
{
    protected abstract string PlanetType { get; }

    protected abstract IReadOnlyList<RawExtractionNumber> Yield { get; }

    public CommandCenter CreateCommandCenter(int level) => new CommandCenter(PlanetType, level);

    public Extractor CreateExtractor(RawProduct rawProduct, int workerCount)
    {
        if (Yield.All(yield => yield.RawProduct != rawProduct))
        {
            throw new ArgumentException("There is no any of this resource on this planet: " + rawProduct);
        }

        return new Extractor(PlanetType, rawProduct, workerCount);
    }

    public BaseFactory CreateBaseFactory(Recipe recipe) => new BaseFactory(PlanetType, recipe);

    public AdvancedFactory CreateAdvancedFactory(Recipe recipe) => new AdvancedFactory(PlanetType, recipe);
}