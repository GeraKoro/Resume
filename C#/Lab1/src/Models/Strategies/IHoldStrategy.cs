using Itmo.ObjectOrientedProgramming.Lab1.Models.FleetHolds;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;

public interface IHoldStrategy
{
    bool TryAddVolume(Ship miningShip, decimal volume, Holds holds, IReadOnlyList<Ship> ships);
}