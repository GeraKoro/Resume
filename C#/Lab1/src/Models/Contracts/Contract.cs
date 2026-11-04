using Itmo.ObjectOrientedProgramming.Lab1.Models.AsteroidBelts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Fleets;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Prices;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Contracts;

public abstract class Contract
{
    public Price Price { get; }

    public Fleet Fleet { get; }

    public AsteroidBelt AsteroidBelt { get; }

    public abstract bool IsRejectable(decimal flight);

    public abstract bool CanStartTrip(decimal flight, decimal time, decimal mined);

    public abstract bool CanStartCycle(decimal flight, decimal time, decimal mined);

    protected Contract(
        Fleet fleet,
        AsteroidBelt asteroidBelt,
        Price price)
    {
        Fleet = fleet;
        AsteroidBelt = asteroidBelt;
        Price = price;
    }
}