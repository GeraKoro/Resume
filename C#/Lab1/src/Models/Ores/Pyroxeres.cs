using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;

public record Pyroxeres : Ore
{
    public override string Name => "Pyroxeres";

    public override decimal Volume => 0.3m;

    public override IReadOnlyList<MineralYield> Yield { get; } =
    [
        new MineralYield(new Mineral("Pyerite"), 45),
        new MineralYield(new Mineral("Mexallon"), 15),
    ];
}