using Itmo.ObjectOrientedProgramming.Lab1.Models.FleetHolds;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;

public class ConnectedHoldStrategy : IHoldStrategy
{
    public bool TryAddVolume(Ship miningShip, decimal volume, Holds holds, IReadOnlyList<Ship> ships)
    {
        if (holds.SpaceLeftOverall < volume)
        {
            return false;
        }

        if (holds.AddVolume(miningShip, volume))
        {
            return true;
        }

        decimal volumeLeft = volume;
        decimal addVolume = Math.Min(holds.SpaceLeft(miningShip), volumeLeft);
        holds.AddVolume(miningShip, addVolume);
        volumeLeft -= addVolume;

        foreach (Ship ship in ships)
        {
            if (volumeLeft == 0)
            {
                break;
            }

            if (ship == miningShip)
            {
                continue;
            }

            addVolume = Math.Min(volumeLeft, holds.SpaceLeft(ship));
            holds.AddVolume(ship, addVolume);
            volumeLeft -= addVolume;
        }

        return true;
    }
}