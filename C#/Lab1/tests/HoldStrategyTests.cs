using Itmo.ObjectOrientedProgramming.Lab1.Models.FleetHolds;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab1.Tests;

public class HoldStrategyTests
{
    [Fact(DisplayName = "Добавление и очистка трюма")]
    public void Hold_General()
    {
        var venture = new Venture();
        var holds = new Holds([venture]);

        Assert.True(holds.AddVolume(venture, 100m));
        Assert.Equal(100m, holds.TotalFilled);
        Assert.Equal(300m, holds.SpaceLeft(venture));

        Assert.False(holds.AddVolume(venture, 400m));

        holds.Clear();
        Assert.Equal(0m, holds.TotalFilled);
        Assert.Equal(400m, holds.SpaceLeft(venture));
    }

    [Fact(DisplayName = "Если стратегия раздельная, то корабль в свой трюм только кладет")]
    public void Separated_Startegy()
    {
        var venture = new Venture();
        var epithal = new Epithal();
        Ship[] ships = [venture, epithal];
        var holds = new Holds(ships);
        var strategy = new SeparatedHoldStrategy();

        Assert.True(strategy.TryAddVolume(venture, 400m, holds, ships));
        Assert.False(strategy.TryAddVolume(venture, 1m, holds, ships));
        Assert.Equal(400m, holds.TotalFilled);
        Assert.Equal(3000m, holds.SpaceLeft(epithal));
    }

    [Fact(DisplayName = "Если стратегия совместная, то корабли делятся")]
    public void Connected_Strategy()
    {
        var venture = new Venture();
        var epithal = new Epithal();
        Ship[] ships = [venture, epithal];
        var holds = new Holds(ships);
        var strategy = new ConnectedHoldStrategy();

        Assert.True(strategy.TryAddVolume(venture, 400m, holds, ships));
        Assert.True(strategy.TryAddVolume(venture, 100m, holds, ships));
        Assert.Equal(500m, holds.TotalFilled);
        Assert.Equal(0m, holds.SpaceLeft(venture));
        Assert.Equal(2900m, holds.SpaceLeft(epithal));
    }

    [Fact(DisplayName = "Не хватает места")]
    public void Not_Enough_Space()
    {
        var venture = new Venture();
        Ship[] ships = [venture];
        var holds = new Holds(ships);
        var strategy = new ConnectedHoldStrategy();

        Assert.False(strategy.TryAddVolume(venture, 4000m, holds, ships));
    }
}