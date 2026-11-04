using Itmo.ObjectOrientedProgramming.Lab2.Calculations;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Colonies;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Factories;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab2.Tests;

public class ValidTests
{
    [Fact(DisplayName = "Уровень командного центра")]
    public void CommandCenter_Level()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => new CommandCenter("Barren", -1));
        Assert.Throws<ArgumentOutOfRangeException>(() => new CommandCenter("Barren", 6));
    }

    [Fact(DisplayName = "Кол-во воркеров от 1 до 10")]
    public void Extractor_Workers()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new Extractor("Barren", new RawProduct("BaseMetals"), 0));
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new Extractor("Barren", new RawProduct("BaseMetals"), 11));
    }

    [Fact(DisplayName = "Часы > 0")]
    public void Calculator_Hours()
    {
        var f = new BarrenBuildingFactory();
        Colony colony = Assert.IsType<ColonyBuildResult.Success>(
            Colony.ColonyBuilder.Create()
                .WithPlanet(new Barren("A"))
                .WithCommandCenter(f.CreateCommandCenter(0))
                .WithBuildings([])
                .Build()).Colony;

        Assert.Throws<ArgumentOutOfRangeException>(() => new Calculator().Calculate(colony, 0));
    }
}