using Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab2.Tests;

public class GeneralTests
{
    [Fact(DisplayName = "Урожай Barren")]
    public void Barren_Yield()
    {
        var planet = new Barren("Barren-I");

        Assert.Equal("Barren", planet.Type);
        Assert.Equal(3, planet.Yield.Count);
    }

    [Fact(DisplayName = "Фабрика создаёт экстрактор")]
    public void Factory_CreateExtractor()
    {
        Extractor extractor = new BarrenBuildingFactory()
            .CreateExtractor(new RawProduct("BaseMetals"), 4);

        Assert.Equal("Barren", extractor.PlanetType);
        Assert.Equal(4, extractor.WorkerCount);
    }

    [Fact(DisplayName = "Каталог рецептов не пустой")]
    public void Recipes_Filled()
    {
        RecipeCatalog catalog = RecipesCatalog.CreateRecipes();

        Assert.Equal(7, catalog.FirstLevel.Count);
        Assert.Equal(5, catalog.SecondLevel.Count);
    }

    [Fact(DisplayName = "Урожай Water")]
    public void Water_Yield()
    {
        var planet = new Water("Water-I");

        Assert.Equal("Water", planet.Type);
        Assert.Equal(3, planet.Yield.Count);
    }
}