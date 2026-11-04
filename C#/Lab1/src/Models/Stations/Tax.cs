namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Stations;

public class Tax
{
    public decimal Value { get; }

    public Tax(decimal tax)
    {
        ArgumentOutOfRangeException.ThrowIfLessThan(tax, 0, "Tax cant be negative");
        ArgumentOutOfRangeException.ThrowIfGreaterThan(tax, 1, "Tax needs to be between 0 and 1");
        Value = tax;
    }
}