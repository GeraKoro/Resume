using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;

public record Kernite : Ore
{
    public override string Name => "Kernite";

    public override decimal Volume => 1.2m;

    public override IReadOnlyList<MineralYield> Yield { get; } =
    [
        new MineralYield(new Mineral("Mexallon"), 30),
        new MineralYield(new Mineral("Isogen"), 30),
    ];
}