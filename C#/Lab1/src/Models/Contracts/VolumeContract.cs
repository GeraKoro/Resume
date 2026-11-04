using Itmo.ObjectOrientedProgramming.Lab1.Models.AsteroidBelts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Fleets;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Prices;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Contracts;

public class VolumeContract : Contract
{
    private readonly decimal _volume;

    public VolumeContract(
        Fleet fleet,
        AsteroidBelt asteroidBelt,
        decimal volume,
        Price price) : base(fleet, asteroidBelt, price)
    {
        if (volume <= 0)
        {
            throw new ArgumentException("Volume must be > 0");
        }

        _volume = volume;
    }

    public override bool IsRejectable(decimal flight)
    {
        return false;
    }

    public override bool CanStartTrip(decimal flight, decimal time, decimal mined)
    {
        return mined < _volume;
    }

    public override bool CanStartCycle(decimal flight, decimal time, decimal mined)
    {
        return mined < _volume;
    }
}