using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;

namespace Itmo.ObjectOrientedProgramming.Lab2.ValueTypes;

public class RawExtractionNumber
{
    public RawProduct RawProduct { get; }

    public int ExtractionNumber { get; }

    public RawExtractionNumber(RawProduct rawProduct, int extractionNumber)
    {
        RawProduct = rawProduct;
        ExtractionNumber = extractionNumber;
    }
}