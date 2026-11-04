namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Stations;

public class Station
{
    public Tax Tax { get; }

    public Station(Tax tax)
    {
        Tax = tax;
    }
}