using Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Colonies;

public class Colony
{
    public Planet Planet { get; }

    public CommandCenter CommandCenter { get; }

    public IReadOnlyList<Building> Buildings { get; }

    private Colony(Planet planet, CommandCenter commandCenter, IReadOnlyList<Building> buildings)
    {
        Planet = planet;
        CommandCenter = commandCenter;
        Buildings = buildings;
    }

    public ColonyCopyResult Copy(Planet planet)
    {
        if (planet.Type != Planet.Type)
        {
            return new ColonyCopyResult.Failure("Planets type must be equal");
        }

        var buildings = Buildings.Select(b => b.Clone()).ToList();
        CommandCenter commandCenter = CommandCenter.Clone();

        var colony = new Colony(planet, commandCenter, buildings);
        return new ColonyCopyResult.Success(colony);
    }

    public class ColonyBuilder : IColonyPlanet, IColonyCenter, IColonyBuildings
    {
        private readonly List<Building> _buildings = [];

        private Planet? _planet;

        private CommandCenter? _commandCenter;

        private ColonyBuilder() { }

        public static IColonyPlanet Create() => new ColonyBuilder();

        public IColonyCenter WithPlanet(Planet planet)
        {
            _planet = planet;
            return this;
        }

        public IColonyBuildings WithCommandCenter(CommandCenter commandCenter)
        {
            _commandCenter = commandCenter;
            return this;
        }

        public IColonyBuildings WithBuildings(IReadOnlyList<Building> buildings)
        {
            _buildings.AddRange(buildings);
            return this;
        }

        public ColonyBuildResult Build()
        {
            if (_planet is null || _commandCenter is null)
            {
                return new ColonyBuildResult.Failure("There must be a planet and command center");
            }

            if (_commandCenter.PlanetType != _planet.Type)
            {
                return new ColonyBuildResult.Failure("Command center planet type and planet type must match");
            }

            if (_buildings.Any(building => building.PlanetType != _planet.Type))
            {
                return new ColonyBuildResult.Failure("Building planet type and planet type must match");
            }

            int cpu = _buildings.Sum(b => b.Cpu);
            int pg = _buildings.Sum(b => b.Pg);

            if (cpu > _commandCenter.CpuProduced || pg > _commandCenter.PgProduced)
            {
                return new ColonyBuildResult.Failure("CPU or PG consuming bigger than command center producing");
            }

            var colony = new Colony(_planet, _commandCenter, _buildings);
            return new ColonyBuildResult.Success(colony);
        }
    }
}

public interface IColonyPlanet
{
    IColonyCenter WithPlanet(Planet planet);
}

public interface IColonyCenter
{
    IColonyBuildings WithCommandCenter(CommandCenter commandCenter);
}

public interface IColonyBuildings
{
    IColonyBuildings WithBuildings(IReadOnlyList<Building> buildings);

    ColonyBuildResult Build();
}