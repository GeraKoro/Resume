using Itmo.ObjectOrientedProgramming.Lab1.Models.AsteroidBelts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Contracts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Fleets;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Prices;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Stations;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab1.Tests;

public class ValidTests
{
    [Fact(DisplayName = "Нельзя пустой флот")]
    public void Fleet_Empty()
    {
        Assert.Throws<ArgumentException>(() =>
            new Fleet([], new ConnectedHoldStrategy()));
    }

    [Fact(DisplayName = "Налог от 0 до 1")]
    public void Tax_OutOfRange()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() => new Tax(-0.1m));
        Assert.Throws<ArgumentOutOfRangeException>(() => new Tax(1.1m));
    }

    [Fact(DisplayName = "Срок и объем натуральные числа")]
    public void Contract_NonPositive()
    {
        Assert.Throws<ArgumentException>(() =>
            new TimeContract(Fleet(), Belt(), 0m, Price()));
        Assert.Throws<ArgumentException>(() =>
            new VolumeContract(Fleet(), Belt(), -1m, Price()));
    }

    [Fact(DisplayName = "Дистанция неотрицательная")]
    public void AsteroidBelt_NegativeDistance()
    {
        Assert.Throws<ArgumentException>(() =>
            new AsteroidBelt("Пояс A", -1m, new Veldspar()));
    }

    [Fact(DisplayName = "Цена неотрицательная")]
    public void Price_Negative()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new Price((new Mineral("Tritanium"), -1m)));
    }

    [Fact(DisplayName = "Выход минералов неотрицательный")]
    public void MineralYield_Negative()
    {
        Assert.Throws<ArgumentOutOfRangeException>(() =>
            new MineralYield(new Mineral("Tritanium"), -1));
    }

    private static Fleet Fleet() =>
        new([new Venture()], new ConnectedHoldStrategy());

    private static AsteroidBelt Belt() =>
        new("Пояс A", 4m, new Veldspar());

    private static Price Price() =>
        new((new Mineral("Tritanium"), 4m));
}