namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;

public abstract class Ship
{
    public abstract string Name { get; }

    public abstract decimal Speed { get; }

    public abstract decimal Rent { get; }

    public abstract decimal Capacity { get; }
}