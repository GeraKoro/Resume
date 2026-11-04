using Itmo.ObjectOrientedProgramming.Lab1.Models.AsteroidBelts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Contracts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Fleets;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Prices;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Result;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Stations;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;
using Xunit;

namespace Itmo.ObjectOrientedProgramming.Lab1.Tests;

// Тестовые сценарии из задания (раздел «Тестовые сценарии»).
// Значения по умолчанию: флот – один Venture, стратегия «Общий трюм»;
// пояс «Пояс A» – 4 а.е., Veldspar; налог 0;
// прайс-лист – Tritanium 4, Pyerite 10, Mexallon 70, Isogen 150.
public class ContractScenarioTests
{
    [Fact(DisplayName = "Сценарий 1. Контракт на срок")]
    public void TimeContract_WithSingleVenture_IsCompleted()
    {
        // Arrange: контракт на 6 ч
        var contract = new TimeContract(DefaultFleet(), DefaultBelt(), 6m, _defaultPrice);

        // Act
        CalculationResult result = _calculator.Calculate(contract, _station);

        // Assert: выполнен; 1 рейс, время работы 6 ч, добыто 400 м³;
        // Tritanium – 8000; выручка 32 000, аренда 6000, прибыль 26 000
        CalculationResult.Completed completed = Assert.IsType<CalculationResult.Completed>(result);
        Assert.Equal(1, completed.TripCount);
        Assert.Equal(6m, completed.Time);
        Assert.Equal(400m, completed.OreVolume);
        Assert.Equal(8000m, Assert.Single(completed.Minerals, m => m.Mineral.Name == "Tritanium").Amount);
        Assert.Equal(32000m, completed.Gain);
        Assert.Equal(6000m, completed.Rent);
        Assert.Equal(26000m, completed.Income);
    }

    [Fact(DisplayName = "Сценарий 2. Пустой флот")]
    public void Fleet_WithoutShips_CannotBeCreated()
    {
        // Arrange + Act: создание флота без кораблей
        // Assert: исключение
        Assert.Throws<ArgumentException>(() =>
            new Fleet([], new ConnectedHoldStrategy()));
    }

    [Fact(DisplayName = "Сценарий 3. Контракт на объём")]
    public void VolumeContract_WithSingleVenture_IsCompleted()
    {
        // Arrange: контракт на 1000 м³
        var contract = new VolumeContract(DefaultFleet(), DefaultBelt(), 1000m, _defaultPrice);

        // Act
        CalculationResult result = _calculator.Calculate(contract, _station);

        // Assert: выполнен; 3 рейса (400, 400, 200 м³), время работы 16 ч;
        // Tritanium – 20 000; выручка 80 000, аренда 16 000, прибыль 64 000
        CalculationResult.Completed completed = Assert.IsType<CalculationResult.Completed>(result);
        Assert.Equal(3, completed.TripCount);
        Assert.Equal(16m, completed.Time);
        Assert.Equal(1000m, completed.OreVolume);
        Assert.Equal(20000m, Assert.Single(completed.Minerals, m => m.Mineral.Name == "Tritanium").Amount);
        Assert.Equal(80000m, completed.Gain);
        Assert.Equal(16000m, completed.Rent);
        Assert.Equal(64000m, completed.Income);
    }

    [Fact(DisplayName = "Сценарий 4. Отклонение по сроку")]
    public void TimeContract_TooShort_IsRejected()
    {
        // Arrange: контракт на 2 ч
        var contract = new TimeContract(DefaultFleet(), DefaultBelt(), 2m, _defaultPrice);

        // Act
        CalculationResult result = _calculator.Calculate(contract, _station);

        // Assert: отклонён – срока не хватает на один рейс
        Assert.IsType<CalculationResult.Rejected>(result);
    }

    [Fact(DisplayName = "Сценарий 5. Отклонение по прайс-листу")]
    public void Contract_WithIncompletePriceList_IsRejected()
    {
        // Arrange: пояс с Scordite, в прайс-листе нет цены Pyerite; контракт на 6 ч
        var price = new Price((new Mineral("Tritanium"), 4m));
        var belt = new AsteroidBelt("Пояс A", 4m, new Scordite());
        var contract = new TimeContract(DefaultFleet(), belt, 6m, price);

        // Act
        CalculationResult result = _calculator.Calculate(contract, _station);

        // Assert: отклонён – в прайс-листе нет цены минерала из выхода руды
        Assert.IsType<CalculationResult.Rejected>(result);
    }

    private readonly Calculator _calculator = new();
    private readonly Station _station = new(new Tax(0));
    private readonly Price _defaultPrice = new(
        (new Mineral("Tritanium"), 4.0m),
        (new Mineral("Pyerite"), 10.0m),
        (new Mineral("Mexallon"), 70.0m),
        (new Mineral("Isogen"), 150.0m));

    private static Fleet DefaultFleet() =>
        new([new Venture()], new ConnectedHoldStrategy());

    private static AsteroidBelt DefaultBelt() =>
        new("Пояс A", 4m, new Veldspar());
}