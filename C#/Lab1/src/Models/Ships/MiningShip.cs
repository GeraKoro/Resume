namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

public abstract class MiningShip : Ship
{
    public abstract decimal MiningPerCycle(int cycleNumber);
}