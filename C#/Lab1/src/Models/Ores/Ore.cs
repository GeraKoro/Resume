using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;

public abstract record Ore
{
    public abstract string Name { get; }

    public abstract decimal Volume { get; }

    public abstract IReadOnlyList<MineralYield> Yield { get; }
}