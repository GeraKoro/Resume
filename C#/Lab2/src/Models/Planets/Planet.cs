using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Planets;

public abstract class Planet
{
    public abstract string Type { get; }

    public abstract IReadOnlyList<RawExtractionNumber> Yield { get; }

    public string Name { get; }

    protected Planet(string name)
    {
        Name = name;
    }
}