"""Погода через Open-Meteo. HTTP-клиент подменяется в тестах."""

from __future__ import annotations

from collections.abc import Awaitable, Callable
from typing import Any

import aiohttp

JsonGetter = Callable[[str, dict[str, Any]], Awaitable[dict[str, Any]]]

GEOCODE_URL = "https://geocoding-api.open-meteo.com/v1/search"
FORECAST_URL = "https://api.open-meteo.com/v1/forecast"


class WeatherError(ValueError):
    """Ошибка погоды без URL и ключей в пользовательском тексте."""


async def default_http_get(url: str, params: dict[str, Any]) -> dict[str, Any]:
    timeout = aiohttp.ClientTimeout(total=10)
    try:
        async with (
            aiohttp.ClientSession(timeout=timeout) as session,
            session.get(url, params=params) as response,
        ):
            if response.status >= 400:
                raise WeatherError("Сервис погоды сейчас недоступен.")
            try:
                payload = await response.json(content_type=None)
            except (aiohttp.ContentTypeError, ValueError) as exc:
                raise WeatherError("Сервис погоды вернул некорректный ответ.") from exc
    except TimeoutError as exc:
        raise WeatherError("Не удалось дождаться ответа сервиса погоды.") from exc
    except aiohttp.ClientError as exc:
        raise WeatherError("Сервис погоды сейчас недоступен.") from exc
    if not isinstance(payload, dict):
        raise WeatherError("Сервис погоды вернул некорректный ответ.")
    return payload


async def get_weather(
    city: str,
    *,
    http_get: JsonGetter | None = None,
) -> dict[str, Any]:
    cleaned = city.strip()
    if not cleaned or len(cleaned) > 100:
        raise WeatherError("Название города должно быть от 1 до 100 символов.")
    getter = http_get or default_http_get
    geo = await getter(
        GEOCODE_URL,
        {"name": cleaned, "count": 5, "language": "ru", "format": "json"},
    )
    results = geo.get("results")
    if not results:
        raise WeatherError("Населённый пункт не найден.")
    if not isinstance(results, list):
        raise WeatherError("Сервис погоды вернул некорректный ответ.")
    places = [item for item in results if isinstance(item, dict)]
    if not places:
        raise WeatherError("Населённый пункт не найден.")
    if _is_ambiguous(cleaned, places):
        return {
            "status": "ambiguous",
            "candidates": [
                {
                    "name": str(item.get("name") or ""),
                    "country": str(item.get("country") or ""),
                    "admin1": str(item.get("admin1") or ""),
                }
                for item in places
            ],
        }
    place = _pick_place(cleaned, places)
    forecast = await getter(
        FORECAST_URL,
        {
            "latitude": place["latitude"],
            "longitude": place["longitude"],
            "current": "temperature_2m,weather_code,wind_speed_10m",
            "timezone": "auto",
            "wind_speed_unit": "ms",
        },
    )
    current = forecast.get("current")
    units = forecast.get("current_units") or {}
    if not isinstance(current, dict):
        raise WeatherError("Сервис погоды вернул некорректный ответ.")
    return {
        "status": "ok",
        "place": str(place.get("name") or cleaned),
        "country": str(place.get("country") or ""),
        "temperature_c": current.get("temperature_2m"),
        "conditions": _weather_text(current.get("weather_code")),
        "weather_code": current.get("weather_code"),
        "wind_speed": current.get("wind_speed_10m"),
        "wind_unit": str(units.get("wind_speed_10m") or "m/s"),
        "observed_at": str(current.get("time") or ""),
        "timezone": str(forecast.get("timezone") or ""),
    }


def _related_name(query: str, name: str) -> bool:
    """Совпадение или расширение базового имени (Ростов / Ростов-на-Дону)."""
    q = query.casefold().strip()
    n = name.casefold().strip()
    if not q or not n:
        return False
    if n == q:
        return True
    return n.startswith(f"{q}-") or n.startswith(f"{q} ")


def _is_ambiguous(query: str, places: list[dict]) -> bool:
    if "," in query:
        return False
    if len(places) < 2:
        return False
    related = [item for item in places if _related_name(query, str(item.get("name") or ""))]
    if len(related) >= 2:
        return True
    first = str(places[0].get("name") or "").casefold()
    return any(str(item.get("name") or "").casefold() == first for item in places[1:])


def _pick_place(query: str, places: list[dict]) -> dict:
    if "," in query:
        left, right = [part.strip().casefold() for part in query.split(",", 1)]
        for item in places:
            name = str(item.get("name") or "").casefold()
            country = str(item.get("country") or "").casefold()
            if name == left and (not right or right in country or country in right):
                return item
    exact = [
        item
        for item in places
        if str(item.get("name") or "").casefold().strip() == query.casefold().strip()
    ]
    if exact:
        return exact[0]
    return places[0]


def _weather_text(code: object) -> str:
    mapping = {
        0: "ясно",
        1: "преимущественно ясно",
        2: "переменная облачность",
        3: "пасмурно",
        45: "туман",
        48: "изморозь",
        51: "лёгкая морось",
        61: "слабый дождь",
        63: "дождь",
        65: "сильный дождь",
        71: "слабый снег",
        73: "снег",
        75: "сильный снег",
        80: "ливень",
        95: "гроза",
    }
    if isinstance(code, int):
        return mapping.get(code, f"код {code}")
    return "неизвестно"
