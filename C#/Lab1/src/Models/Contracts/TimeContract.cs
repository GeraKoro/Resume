using Itmo.ObjectOrientedProgramming.Lab1.Models.AsteroidBelts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Fleets;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Prices;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Contracts;

public class TimeContract : Contract
{
    private readonly decimal _time;

    public TimeContract(
        Fleet fleet,
        AsteroidBelt asteroidBelt,
        decimal time,
        Price price) : base(fleet, asteroidBelt, price)
    {
        if (time <= 0)
        {
            throw new ArgumentException("Time must be > 0");
        }

        _time = time;
    }

    public override bool IsRejectable(decimal flight)
    {
        return (flight + 1 + flight) > _time;
    }

    public override bool CanStartTrip(decimal flight, decimal time, decimal mined)
    {
        return (flight + time + 1 + flight) <= _time;
    }

    public override bool CanStartCycle(decimal flight, decimal time, decimal mined)
    {
        return (flight + time + 1) <= _time;
    }
}