using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Result;

public abstract record CalculationResult
{
    private CalculationResult() { }

    public sealed record Rejected(string Reason) : CalculationResult;

    public sealed record Completed(
        decimal Time,
        int TripCount,
        decimal OreVolume,
        IReadOnlyList<MineralAmount> Minerals,
        decimal OreLeft,
        decimal Gain,
        decimal Rent,
        decimal Tax,
        decimal Income)
        : CalculationResult;
}