using Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.AsteroidBelts;

public class AsteroidBelt
{
    public string Name { get; }

    public decimal Distance { get; }

    public Ore Ore { get; }

    public AsteroidBelt(string name, decimal distance, Ore ore)
    {
        if (distance < 0)
        {
            throw new ArgumentException("Distance cant be negative");
        }

        Name = name;
        Distance = distance;
        Ore = ore;
    }
}