using Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;

public interface IBuildingFactory
{
    CommandCenter CreateCommandCenter(int level);

    Extractor CreateExtractor(RawProduct rawProduct, int workerCount);

    BaseFactory CreateBaseFactory(Recipe recipe);

    AdvancedFactory CreateAdvancedFactory(Recipe recipe);
}