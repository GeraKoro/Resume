using Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Colonies;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;
using Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;
using Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

namespace Itmo.ObjectOrientedProgramming.Lab2.Calculations;

public class Calculator
{
    public IReadOnlyList<StockOneElement> Calculate(Colony colony, int hours)
    {
        if (hours <= 0)
        {
            throw new ArgumentOutOfRangeException(nameof(hours), hours, "Time must be > 0");
        }

        var stock = new List<StockOneElement>();
        for (int hour = 0; hour < hours; ++hour)
        {
            ProduceExtractor(colony, stock);
            ProduceBaseFactory(colony, stock);
            ProduceAdvancedFactory(colony, stock);
        }

        return stock;
    }

    private static void AddStock(List<StockOneElement> stock, Product product, int amount)
    {
        if (amount <= 0)
        {
            return;
        }

        int index = stock.FindIndex(s => s.Product == product);
        if (index < 0)
        {
            stock.Add(new StockOneElement(product, amount));
            return;
        }

        int newAmount = stock[index].Amount + amount;
        stock[index] = new StockOneElement(product, newAmount);
    }

    private static void RemoveStock(List<StockOneElement> stock, Product product, int amount)
    {
        if (amount <= 0)
        {
            return;
        }

        int index = stock.FindIndex(s => s.Product == product);
        if (index < 0 || stock[index].Amount < amount)
        {
            return;
        }

        int newAmount = stock[index].Amount - amount;
        if (newAmount == 0)
        {
            stock.RemoveAt(index);
        }
        else
        {
            stock[index] = new StockOneElement(product, newAmount);
        }
    }

    private static void TryProduce(List<StockOneElement> stock, Recipe recipe)
    {
        foreach (Product inputProduct in recipe.Input)
        {
            int index = stock.FindIndex(s => s.Product == inputProduct);
            if (index < 0 || stock[index].Amount < recipe.InputAmount)
            {
                return;
            }
        }

        foreach (Product inputProduct in recipe.Input)
        {
            RemoveStock(stock, inputProduct, recipe.InputAmount);
        }

        AddStock(stock, recipe.Output, recipe.OutputAmount);
    }

    private static void ProduceExtractor(Colony colony, List<StockOneElement> stock)
    {
        foreach (Extractor extractor in colony.Buildings.OfType<Extractor>())
        {
            RawExtractionNumber yield = colony.Planet.Yield.First(y => y.RawProduct == extractor.RawProduct);
            int amount = extractor.WorkerCount * yield.ExtractionNumber;
            AddStock(stock, new Product(extractor.RawProduct.Name), amount);
        }
    }

    private static void ProduceBaseFactory(Colony colony, List<StockOneElement> stock)
    {
        foreach (BaseFactory factory in colony.Buildings.OfType<BaseFactory>())
        {
            TryProduce(stock, factory.Recipe);
        }
    }

    private static void ProduceAdvancedFactory(Colony colony, List<StockOneElement> stock)
    {
        foreach (AdvancedFactory factory in colony.Buildings.OfType<AdvancedFactory>())
        {
            TryProduce(stock, factory.Recipe);
        }
    }
}