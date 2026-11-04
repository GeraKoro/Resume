using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;

public record Veldspar : Ore
{
    public override string Name => "Veldspar";

    public override decimal Volume => 0.1m;

    public override IReadOnlyList<MineralYield> Yield { get; } =
    [
        new MineralYield(new Mineral("Tritanium"), 200),
    ];
}