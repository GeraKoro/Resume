using Itmo.ObjectOrientedProgramming.Lab1.Models.Fleets;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab1.Tests;

public class ShipsTests
{
    [Fact(DisplayName = "Venture всегда добывает 100 за цикл")]
    public void Venture_Mining()
    {
        var ship = new Venture();

        Assert.Equal(100m, ship.MiningPerCycle(1));
        Assert.Equal(100m, ship.MiningPerCycle(2));
        Assert.Equal(400m, ship.Capacity);
        Assert.Equal(4m, ship.Speed);
        Assert.Equal(1000m, ship.Rent);
    }

    [Fact(DisplayName = "Prospect первый цикл 75, потом 150")]
    public void Prospect_Mining()
    {
        var ship = new Prospect();

        Assert.Equal(75m, ship.MiningPerCycle(1));
        Assert.Equal(150m, ship.MiningPerCycle(2));
        Assert.Equal(150m, ship.MiningPerCycle(3));
    }

    [Fact(DisplayName = "Скорость флота = мин скорость корабля из флота")]
    public void Fleet_Speed()
    {
        var fleet = new Fleet(
            [new Venture(), new Epithal()],
            new ConnectedHoldStrategy());

        Assert.Equal(2m, fleet.Speed);
        Assert.Equal(3400m, fleet.Capacity);
    }
}