using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.FleetHolds;

public class Holds
{
    public Holds(IReadOnlyList<Ship> ships)
    {
        _shipHolds = new Dictionary<Ship, decimal>();
        foreach (Ship ship in ships)
        {
            _shipHolds[ship] = 0;
        }
    }

    private readonly Dictionary<Ship, decimal> _shipHolds;

    public decimal SpaceLeft(Ship ship)
    {
        return ship.Capacity - _shipHolds[ship];
    }

    public bool AddVolume(Ship ship, decimal volume)
    {
        if (SpaceLeft(ship) < volume)
        {
            return false;
        }

        _shipHolds[ship] += volume;
        return true;
    }

    public void Clear()
    {
        foreach (KeyValuePair<Ship, decimal> shipHold in _shipHolds)
        {
            _shipHolds[shipHold.Key] = 0;
        }
    }

    public decimal SpaceLeftOverall => _shipHolds.Sum(ship => ship.Key.Capacity - ship.Value);

    public decimal TotalFilled => _shipHolds.Sum(ship => ship.Value);
}