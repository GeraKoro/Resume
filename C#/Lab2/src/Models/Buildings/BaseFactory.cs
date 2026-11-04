using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;

public class BaseFactory : Building
{
    public override string Name => nameof(BaseFactory);

    public override int Cpu => 200;

    public override int Pg => 800;

    public override string PlanetType { get; }

    public Recipe Recipe { get; }

    public BaseFactory(string planetType, Recipe recipe)
    {
        PlanetType = planetType;
        Recipe = recipe;
    }

    public override BaseFactory Clone() => new BaseFactory(PlanetType, Recipe);
}