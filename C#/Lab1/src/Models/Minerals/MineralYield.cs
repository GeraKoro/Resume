namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

public class MineralYield
{
    public Mineral Mineral { get; }

    public int AmountInPortion { get; }

    public MineralYield(Mineral mineral, int amount)
    {
        ArgumentOutOfRangeException.ThrowIfNegative(amount, "Amount in portion cant be negative");
        Mineral = mineral;
        AmountInPortion = amount;
    }
}