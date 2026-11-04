using Itmo.ObjectOrientedProgramming.Lab2.Calculations;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Colonies;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab2.Tests;

public class TestScenario
{
    private readonly RecipeCatalog _recipes = RecipesCatalog.CreateRecipes();
    private readonly Calculator _calculator = new();

    [Fact(DisplayName = "Сценарий 1 сборка колонии")]
    public void Scenario1_Build()
    {
        ColonyBuildResult result = BuildMechanicalParts(2);

        ColonyBuildResult.Success ok = Assert.IsType<ColonyBuildResult.Success>(result);
        Assert.Equal(2580, ok.Colony.Buildings.Sum(b => b.Cpu));
        Assert.Equal(12136, ok.Colony.CommandCenter.CpuProduced);
        Assert.Equal(11900, ok.Colony.Buildings.Sum(b => b.Pg));
        Assert.Equal(12000, ok.Colony.CommandCenter.PgProduced);
    }

    [Fact(DisplayName = "Сценарий 2 нехватка энергии")]
    public void Scenario2_NotEnoughPg()
    {
        Assert.IsType<ColonyBuildResult.Failure>(BuildMechanicalParts(1));
    }

    [Fact(DisplayName = "Сценарий 3 здание другого типа")]
    public void Scenario3_WrongPlanetType()
    {
        var barren = new BarrenBuildingFactory();
        var lava = new LavaBuildingFactory();

        ColonyBuildResult result = Colony.ColonyBuilder.Create()
            .WithPlanet(new Barren("Barren-I"))
            .WithCommandCenter(barren.CreateCommandCenter(2))
            .WithBuildings(
            [
                barren.CreateExtractor(new RawProduct("NobleMetals"), 4),
                barren.CreateExtractor(new RawProduct("BaseMetals"), 4),
                barren.CreateBaseFactory(_recipes.FirstLevel.First(r => r.Input[0].Name == "NobleMetals")),
                lava.CreateBaseFactory(_recipes.FirstLevel.First(r => r.Input[0].Name == "BaseMetals")),
                barren.CreateAdvancedFactory(_recipes.SecondLevel.First(r => r.Output.Name == "MechanicalParts")),
            ])
            .Build();

        Assert.IsType<ColonyBuildResult.Failure>(result);
    }

    [Fact(DisplayName = "Сценарий 4 сырьё не с планеты")]
    public void Scenario4_WrongResource()
    {
        Assert.Throws<ArgumentException>(() =>
            new LavaBuildingFactory().CreateExtractor(new RawProduct("NobleMetals"), 4));
    }

    [Fact(DisplayName = "Сценарий 5 производство за 24 часа")]
    public void Scenario5_Production()
    {
        Colony colony = Assert.IsType<ColonyBuildResult.Success>(BuildMechanicalParts(2)).Colony;

        IReadOnlyList<StockOneElement> stock = _calculator.Calculate(colony, 24);

        Assert.Single(stock);
        Assert.Equal(120, stock[0].Amount);
        Assert.Equal("MechanicalParts", stock[0].Product.Name);
    }

    [Fact(DisplayName = "Сценарий 6 копирование")]
    public void Scenario6_Copy()
    {
        Colony colony = Assert.IsType<ColonyBuildResult.Success>(BuildMechanicalParts(2)).Colony;

        ColonyCopyResult ok = colony.Copy(new Barren("Barren-II"));
        ColonyCopyResult fail = colony.Copy(new Lava("Lava-I"));

        Colony copy = Assert.IsType<ColonyCopyResult.Success>(ok).Colony;
        Assert.Equal(120, Assert.Single(_calculator.Calculate(copy, 24)).Amount);
        Assert.IsType<ColonyCopyResult.Failure>(fail);
    }

    private ColonyBuildResult BuildMechanicalParts(int level)
    {
        var f = new BarrenBuildingFactory();
        return Colony.ColonyBuilder.Create()
            .WithPlanet(new Barren("Barren-I"))
            .WithCommandCenter(f.CreateCommandCenter(level))
            .WithBuildings(
            [
                f.CreateExtractor(new RawProduct("NobleMetals"), 4),
                f.CreateExtractor(new RawProduct("BaseMetals"), 4),
                f.CreateBaseFactory(_recipes.FirstLevel.First(r => r.Input[0].Name == "NobleMetals")),
                f.CreateBaseFactory(_recipes.FirstLevel.First(r => r.Input[0].Name == "BaseMetals")),
                f.CreateAdvancedFactory(_recipes.SecondLevel.First(r => r.Output.Name == "MechanicalParts")),
            ])
            .Build();
    }
}