using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;

namespace Itmo.ObjectOrientedProgramming.Lab1.Models.Prices;

public class Price
{
    private readonly Dictionary<Mineral, decimal> _prices;

    public bool HasPrice(Mineral mineral)
    {
        return _prices.ContainsKey(mineral);
    }

    public decimal GetPrice(Mineral mineral)
    {
        return _prices[mineral];
    }

    public Price(params (Mineral Mineral, decimal Price)[] prices)
    {
        foreach ((Mineral mineral, decimal price) in prices)
        {
            ArgumentOutOfRangeException.ThrowIfNegative(
                price,
                "Price cant be negative");
        }

        _prices = prices.ToDictionary(pair => pair.Mineral, pair => pair.Price);
    }
}