using Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab2.Tests;

public class BuildingsTests
{
    [Fact(DisplayName = "Extractor CPU и PG")]
    public void Extractor_Consumption()
    {
        var e = new Extractor("Barren", new RawProduct("BaseMetals"), 4);

        Assert.Equal(840, e.Cpu);
        Assert.Equal(4800, e.Pg);
    }

    [Fact(DisplayName = "CommandCenter 2 уровень")]
    public void CommandCenter_Level2()
    {
        var c = new CommandCenter("Barren", 2);

        Assert.Equal(0, c.Cpu);
        Assert.Equal(12136, c.CpuProduced);
        Assert.Equal(12000, c.PgProduced);
    }

    [Fact(DisplayName = "Clone без смены типа")]
    public void Extractor_Clone()
    {
        var e = new Extractor("Barren", new RawProduct("BaseMetals"), 4);
        Extractor copy = e.Clone();

        Assert.Equal(4, copy.WorkerCount);
        Assert.NotSame(e, copy);
    }

    [Fact(DisplayName = "Потребление фабрик")]
    public void Factories_Consumption()
    {
        var recipe = new Recipe(new Product("X"), 40, 6000, [new Product("Y")]);
        Assert.Equal(200, new BaseFactory("Barren", recipe).Cpu);
        Assert.Equal(800, new BaseFactory("Barren", recipe).Pg);
        Assert.Equal(500, new AdvancedFactory("Barren", recipe).Cpu);
        Assert.Equal(700, new AdvancedFactory("Barren", recipe).Pg);
    }
}