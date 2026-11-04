namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Colonies;

public abstract record ColonyCopyResult
{
    private ColonyCopyResult() { }

    public sealed record Success(Colony Colony) : ColonyCopyResult;

    public sealed record Failure(string Reason) : ColonyCopyResult;
}