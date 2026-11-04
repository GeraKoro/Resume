namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;

public class CommandCenter : Building
{
    private static readonly (int Cpu, int Pg)[] Powers =
    [
        (1675, 6000),
        (7057, 9000),
        (12136, 12000),
        (17215, 15000),
        (21315, 17000),
        (25415, 19000),
    ];

    public override string Name => nameof(CommandCenter);

    public override int Cpu => 0;

    public override int Pg => 0;

    public override string PlanetType { get; }

    public int Level { get; }

    public int CpuProduced => Powers[Level].Cpu;

    public int PgProduced => Powers[Level].Pg;

    public CommandCenter(string planetType, int level)
    {
        if (level < 0 || level > 5)
        {
            throw new ArgumentOutOfRangeException(nameof(level), "Level must be between 0 and 5");
        }

        Level = level;
        PlanetType = planetType;
    }

    public override CommandCenter Clone() => new CommandCenter(PlanetType, Level);
}