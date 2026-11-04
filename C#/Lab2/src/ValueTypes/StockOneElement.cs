using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;

namespace Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

public class StockOneElement
{
    public Product Product { get; }

    public int Amount { get; }

    public StockOneElement(Product product, int amount)
    {
        Product = product;
        Amount = amount;
    }
}