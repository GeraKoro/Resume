using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Stations;

public class StationStock
{
    private const int PortionSize = 100;

    public int OreUnits { get; private set; }

    private readonly Dictionary<Mineral, decimal> _minerals = new Dictionary<Mineral, decimal>();

    public IReadOnlyList<MineralAmount> Minerals => _minerals.Select(
        x => new MineralAmount(x.Key, x.Value)).ToList();

    public void Recycle(decimal volume, Ore ore)
    {
        int units = (int)(volume / ore.Volume);
        OreUnits += units;

        int portions = OreUnits / PortionSize;
        OreUnits %= PortionSize;

        if (portions == 0)
        {
            return;
        }

        foreach (MineralYield mineralYield in ore.Yield)
        {
            decimal addMineralYield = mineralYield.AmountInPortion * portions;
            if (_minerals.ContainsKey(mineralYield.Mineral))
            {
                _minerals[mineralYield.Mineral] += addMineralYield;
            }
            else
            {
                _minerals[mineralYield.Mineral] = addMineralYield;
            }
        }
    }
}