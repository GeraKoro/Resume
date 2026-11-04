namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

public class MineralAmount
{
    public Mineral Mineral { get; }

    public decimal Amount { get; }

    public MineralAmount(Mineral mineral, decimal amount)
    {
        Mineral = mineral;
        Amount = amount;
    }
}