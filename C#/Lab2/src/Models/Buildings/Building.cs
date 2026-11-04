namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;

public abstract class Building
{
    public abstract string Name { get; }

    public abstract int Cpu { get; }

    public abstract int Pg { get; }

    public abstract string PlanetType { get; }

    public abstract Building Clone();
}