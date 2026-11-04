using Itmo.ObjectOrientedProgramming.Lab1.Models.FleetHolds;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;

public class SeparatedHoldStrategy : IHoldStrategy
{
    public bool TryAddVolume(Ship miningShip, decimal volume, Holds holds, IReadOnlyList<Ship> ships)
    {
        return holds.AddVolume(miningShip, volume);
    }
}