namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

public class Venture : MiningShip
{
    public override string Name => "Venture";

    public override decimal Capacity => 400;

    public override decimal Speed => 4;

    public override decimal Rent => 1000;

    public override decimal MiningPerCycle(int cycleNumber)
        => 100;
}