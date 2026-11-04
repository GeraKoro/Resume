namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

public class Prospect : MiningShip
{
    public override string Name => "Prospect";

    public override decimal Capacity => 1000;

    public override decimal Speed => 5;

    public override decimal Rent => 1500;

    public override decimal MiningPerCycle(int cycleNumber)
        => cycleNumber == 1 ? 75 : 150;
}