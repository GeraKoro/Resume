#pragma once

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

class Station {

private:
    std::string code_ = "NONE";
    std::string name_ = "NONE";
    std::string type_ = "NONE";
    std::string city_ = "NONE";
    std::string transport_type_ = "NONE";

public:
    Station() = default;
    Station(std::string code, std::string name, std::string type, std::string city, std::string transport_type = {})
        : code_(std::move(code))
        , name_(std::move(name))
        , type_(std::move(type))
        , city_(std::move(city))
        , transport_type_(transport_type.empty() ? "NONE" : std::move(transport_type))
    {}

    const std::string& GetCode() const { return code_; }
    const std::string& GetName() const { return name_; }
    const std::string& GetType() const { return type_; }
    const std::string& GetCity() const { return city_; }
    const std::string& GetTransportType() const { return transport_type_; }

    void Print() const {
        std::cout << "Станция\n";
        std::cout << "Код: " << code_ << ", тип: " << type_ << '\n';
        std::cout << "Название: " << name_ << ", город: " << city_ << '\n';
    }
};

class Path {

private:
    std::string transport_type_ = "NONE";
    std::string transport_name_ = "NONE";
    std::string departure_station_name_ = "NONE";
    std::string departure_station_code_ = "NONE";
    std::string arrival_station_name_ = "NONE";
    std::string arrival_station_code_ = "NONE";
    std::string departure_time_ = "NONE";
    std::string arrival_time_ = "NONE";
    std::size_t duration_ = 0;
    std::vector<Path> connection_;

public:
    Path() = default;
    Path(std::string type, std::string name, std::string from_name, std::string from_code, std::string to_name, std::string to_code,
         std::string dep_time, std::string arr_time, std::size_t duration)
        : transport_type_(std::move(type))
        , transport_name_(std::move(name))
        , departure_station_name_(std::move(from_name))
        , departure_station_code_(std::move(from_code))
        , arrival_station_name_(std::move(to_name))
        , arrival_station_code_(std::move(to_code))
        , departure_time_(std::move(dep_time))
        , arrival_time_(std::move(arr_time))
        , duration_(duration)
    {}

    const std::string& GetTransportType() const { return transport_type_; }
    const std::string& GetTransportName() const { return transport_name_; }
    const std::string& GetDepartureStationName() const { return departure_station_name_; }
    const std::string& GetDepartureStationCode() const { return departure_station_code_; }
    const std::string& GetArrivalStationName() const { return arrival_station_name_; }
    const std::string& GetArrivalStationCode() const { return arrival_station_code_; }
    const std::string& GetDepartureTime() const { return departure_time_; }
    const std::string& GetArrivalTime() const { return arrival_time_; }
    std::size_t GetDuration() const { return duration_; }

    const std::vector<Path>& GetConnection() const { return connection_; }
    void SetConnection(std::vector<Path> connection) { connection_ = std::move(connection); }
};

class Route {

private:
    Path first_half_path_;
    std::optional<Path> second_half_path_;
    std::string departure_from_ = "NONE";
    std::string arrival_to_ = "NONE";
    std::string departure_date_ = "NONE";
    std::string arrival_date_ = "NONE";
    std::size_t total_duration_ = 0;

public:
    Route() = default;

    Route(Path path, std::string date, std::string from, std::string to)
        : first_half_path_(std::move(path))
        , departure_date_(std::move(date))
        , arrival_date_(departure_date_)
        , departure_from_(std::move(from))
        , arrival_to_(std::move(to))
    {
        total_duration_ = first_half_path_.GetDuration();
    }

    Route(Path path_first, Path path_second, std::string date, std::string from, std::string to)
        : first_half_path_(std::move(path_first))
        , second_half_path_(std::move(path_second))
        , departure_date_(std::move(date))
        , arrival_date_(departure_date_)
        , departure_from_(std::move(from))
        , arrival_to_(std::move(to))
    {
        total_duration_ = first_half_path_.GetDuration() + second_half_path_->GetDuration();
    }

    bool HasTransfer() const { return second_half_path_.has_value(); }

    const Path& GetFirstPath() const { return first_half_path_; }
    const std::optional<Path>& GetSecondPath() const { return second_half_path_; }

    const std::string& GetDepartureFrom() const { return departure_from_; }
    const std::string& GetArrivalTo() const { return arrival_to_; }
    const std::string& GetDepartureDate() const { return departure_date_; }
    const std::string& GetArrivalDate() const { return arrival_date_; }

    std::size_t GetTotalDuration() const {
        std::size_t total = first_half_path_.GetDuration();
        if (HasTransfer()) {
            total += second_half_path_->GetDuration();
        }
        return total;
    }
};
