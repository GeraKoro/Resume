using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;

public class AdvancedFactory : Building
{
    public override string Name => nameof(AdvancedFactory);

    public override int Cpu => 500;

    public override int Pg => 700;

    public override string PlanetType { get; }

    public Recipe Recipe { get; }

    public AdvancedFactory(string planetType, Recipe recipe)
    {
        PlanetType = planetType;
        Recipe = recipe;
    }

    public override AdvancedFactory Clone() => new AdvancedFactory(PlanetType, Recipe);
}