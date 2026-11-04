using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;

public record Scordite : Ore
{
    public override string Name => "Scordite";

    public override decimal Volume => 0.15m;

    public override IReadOnlyList<MineralYield> Yield { get; } =
    [
        new MineralYield(new Mineral("Tritanium"), 75),
        new MineralYield(new Mineral("Pyerite"), 55),
    ];
}