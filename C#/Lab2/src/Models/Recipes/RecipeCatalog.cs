namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Recipes;

public class RecipeCatalog
{
    public IReadOnlyList<Recipe> FirstLevel { get; }

    public IReadOnlyList<Recipe> SecondLevel { get; }

    public RecipeCatalog(IReadOnlyList<Recipe> firstLevel, IReadOnlyList<Recipe> secondLevel)
    {
        FirstLevel = firstLevel;
        SecondLevel = secondLevel;
    }
}