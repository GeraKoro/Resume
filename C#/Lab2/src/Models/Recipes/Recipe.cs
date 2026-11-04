using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;

public record Recipe(Product Output, int OutputAmount, int InputAmount, IReadOnlyList<Product> Input);