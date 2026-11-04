using Itmo.ObjectOrientedProgramming.Lab2.Models.Colonies;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab2.Tests;

public class ColonyTests
{
    [Fact(DisplayName = "Пустая колония с командным центром собирается")]
    public void Build_Empty()
    {
        var f = new BarrenBuildingFactory();

        ColonyBuildResult result = Colony.ColonyBuilder.Create()
            .WithPlanet(new Barren("Barren-I"))
            .WithCommandCenter(f.CreateCommandCenter(0))
            .WithBuildings([])
            .Build();

        Assert.IsType<ColonyBuildResult.Success>(result);
    }

    [Fact(DisplayName = "Copy на тот же тип планеты")]
    public void Copy_SameType()
    {
        var f = new BarrenBuildingFactory();
        Colony colony = Assert.IsType<ColonyBuildResult.Success>(
                Colony.ColonyBuilder.Create()
                    .WithPlanet(new Barren("Barren-I"))
                    .WithCommandCenter(f.CreateCommandCenter(0))
                    .WithBuildings([])
                    .Build())
            .Colony;
        Assert.IsType<ColonyCopyResult.Success>(colony.Copy(new Barren("Barren-II")));
    }

    [Fact(DisplayName = "Copy на другой тип планеты")]
    public void Copy_OtherType()
    {
        var f = new BarrenBuildingFactory();
        Colony colony = Assert.IsType<ColonyBuildResult.Success>(
            Colony.ColonyBuilder.Create()
                .WithPlanet(new Barren("Barren-I"))
                .WithCommandCenter(f.CreateCommandCenter(0))
                .WithBuildings([])
                .Build()).Colony;

        Assert.IsType<ColonyCopyResult.Failure>(colony.Copy(new Lava("Lava-I")));
    }
}