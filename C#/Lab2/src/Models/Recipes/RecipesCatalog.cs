using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;

public class RecipesCatalog
{
    public static RecipeCatalog CreateRecipes()
    {
        Recipe[] firstLevel =
        [
            new Recipe(new Product("ReactiveMetals"), 40, 6000, [new Product("BaseMetals")]),
            new Recipe(new Product("PreciousMetals"), 40, 6000, [new Product("NobleMetals")]),
            new Recipe(new Product("Bacteria"), 40, 6000, [new Product("Microorganisms")]),
            new Recipe(new Product("IndustrialFibers"), 40, 6000, [new Product("Autotrophs")]),
            new Recipe(new Product("Proteins"), 40, 6000, [new Product("ComplexOrganisms")]),
            new Recipe(new Product("ToxicMetals"), 40, 6000, [new Product("HeavyMetals")]),
            new Recipe(new Product("Silicon"), 40, 6000, [new Product("FelsicMagma")]),
        ];
        Recipe[] secondLevel =
        [
            new Recipe(
                new Product("MechanicalParts"),
                5,
                40,
                [new Product("PreciousMetals"), new Product("ReactiveMetals")]),
            new Recipe(
                new Product("Nanites"),
                5,
                40,
                [new Product("ReactiveMetals"), new Product("Bacteria")]),
            new Recipe(
                new Product("Fertilizer"),
                5,
                40,
                [new Product("Proteins"), new Product("Bacteria")]),
            new Recipe(
                new Product("ConstructionBlocks"),
                5,
                40,
                [new Product("ToxicMetals"), new Product("ReactiveMetals")]),
            new Recipe(
                new Product("MicrofiberShielding"),
                5,
                40,
                [new Product("Silicon"), new Product("IndustrialFibers")]),
        ];
        return new RecipeCatalog(firstLevel, secondLevel);
    }
}