using Itmo.ObjectOrientedProgramming.Lab1.Models.Contracts;
using Itmo.ObjectOrientedProgramming.Lab1.Models.FleetHolds;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Minerals;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ores;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Result;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Ships;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Stations;
using Itmo.ObjectOrientedProgramming.Lab1.Models.Strategies;

namespace Itmo.ObjectOrientedProgramming.Lab1;

public class Calculator
{
    public CalculationResult Calculate(Contract contract, Station station)
    {
        decimal flight = contract.AsteroidBelt.Distance / contract.Fleet.Speed;
        if (contract.IsRejectable(flight))
        {
            return new CalculationResult.Rejected("Contract is rejected");
        }

        var checkHolds = new Holds(contract.Fleet.Ships);
        IHoldStrategy checkStrategy = contract.Fleet.HoldStrategy;

        if (!contract.Fleet.Ships.OfType<MiningShip>()
                .Any(ship => checkStrategy.TryAddVolume(
                    ship,
                    ship.MiningPerCycle(1),
                    checkHolds,
                    contract.Fleet.Ships)))
        {
            return new CalculationResult.Rejected("No one can mine");
        }

        Ore ore = contract.AsteroidBelt.Ore;
        foreach (MineralYield mineralYield in ore.Yield)
        {
            if (!contract.Price.HasPrice(mineralYield.Mineral))
            {
                return new CalculationResult.Rejected("There is no price for " + mineralYield.Mineral);
            }
        }

        var holds = new Holds(contract.Fleet.Ships);
        decimal time = 0;
        decimal mined = 0;
        int tripCount = 0;
        IHoldStrategy strategy = contract.Fleet.HoldStrategy;
        var stock = new StationStock();

        while (contract.CanStartTrip(flight, time, mined))
        {
            time += flight;
            int cycleNumber = 1;
            while (contract.CanStartCycle(flight, time, mined))
            {
                decimal minedInCycle = MineCycle(contract, holds, strategy, cycleNumber);
                if (minedInCycle == 0)
                {
                    break;
                }

                mined += minedInCycle;
                ++time;
                ++cycleNumber;
            }

            time += flight;
            ++tripCount;
            decimal oreAmount = holds.TotalFilled;

            stock.Recycle(oreAmount, ore);
            holds.Clear();
        }

        decimal gain = stock.Minerals.Sum(mineralAmount =>
            mineralAmount.Amount
            * contract.Price.GetPrice(mineralAmount.Mineral));

        decimal rent = contract.Fleet.Ships.Sum(ship => ship.Rent * time);

        decimal taxAmount = gain * station.Tax.Value;
        decimal income = gain - rent - taxAmount;

        return new CalculationResult.Completed(
            time,
            tripCount,
            mined,
            stock.Minerals,
            stock.OreUnits,
            gain,
            rent,
            taxAmount,
            income);
    }

    private static decimal MineCycle(
        Contract contract,
        Holds holds,
        IHoldStrategy strategy,
        int cycleNumber)
    {
        decimal minedInCycle = 0;
        foreach (Ship ship in contract.Fleet.Ships)
        {
            if (ship is MiningShip miningShip)
            {
                decimal miningVolumePerCycle = miningShip.MiningPerCycle(cycleNumber);
                if (strategy.TryAddVolume(miningShip, miningVolumePerCycle, holds, contract.Fleet.Ships))
                {
                    minedInCycle += miningVolumePerCycle;
                }
            }
        }

        return minedInCycle;
    }
}