using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Fleets;

public class Fleet
{
    private readonly List<Ship> _ships;

    public IReadOnlyList<Ship> Ships => _ships;

    public decimal Speed => _ships.Min(ship => ship.Speed);

    public decimal Capacity => _ships.Sum(ship => ship.Capacity);

    public Fleet(IReadOnlyList<Ship> ships, IHoldStrategy holdStrategy)
    {
        if (ships.Count == 0)
        {
            throw new ArgumentException("Fleet can't be empty");
        }

        _ships = new List<Ship>(ships);
        HoldStrategy = holdStrategy;
    }

    public IHoldStrategy HoldStrategy { get; }
}