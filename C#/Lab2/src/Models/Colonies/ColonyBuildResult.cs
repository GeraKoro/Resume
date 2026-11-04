namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Colonies;

public abstract record ColonyBuildResult
{
    private ColonyBuildResult() { }

    public sealed record Success(Colony Colony) : ColonyBuildResult;

    public sealed record Failure(string Reason) : ColonyBuildResult;
}