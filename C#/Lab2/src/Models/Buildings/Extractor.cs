using Itmo.ObjectOrientedProgramming.Lab2.Models.Products;

namespace Itmo.ObjectOrientedProgramming.Lab2.Models.Buildings;

public class Extractor : Building
{
    public int WorkerCount { get; }

    public override string Name => nameof(Extractor);

    public override int Cpu => CpuConsumptionFactory + (CpuConsumptionWorker * WorkerCount);

    public override int Pg => PgConsumptionFactory + (PgConsumptionWorker * WorkerCount);

    public override string PlanetType { get; }

    public RawProduct RawProduct { get; }

    public Extractor(string planetType, RawProduct rawProduct, int workerCount)
    {
        if (workerCount < 1 || workerCount > 10)
        {
            throw new ArgumentOutOfRangeException(nameof(workerCount), "Count of workers must be between 1 and 10");
        }

        PlanetType = planetType;
        RawProduct = rawProduct;
        WorkerCount = workerCount;
    }

    public override Extractor Clone() => new Extractor(PlanetType, RawProduct, WorkerCount);

    private const int CpuConsumptionFactory = 400;
    private const int CpuConsumptionWorker = 110;

    private const int PgConsumptionFactory = 2600;
    private const int PgConsumptionWorker = 550;
}