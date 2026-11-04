"""Проверки собственного MCP-сервера без Telegram и без LLM."""

from __future__ import annotations

import os
import sys
from datetime import UTC, datetime
from pathlib import Path

import pytest
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

from mcp_server.schedule_data import (
    ScheduleError,
    events_for_date,
    load_events,
    parse_iso_date,
    week_bounds,
    week_schedule,
)
from mcp_server.weather import WeatherError, get_weather

ROOT = Path(__file__).resolve().parents[1]
SCHEDULE = ROOT / "data" / "schedule.json"


def test_schedule_loads_two_weeks_and_date_query():
    events = load_events(SCHEDULE)
    assert len(events) >= 10
    day = parse_iso_date("2026-10-12")
    items = events_for_date(events, day)
    assert len(items) == 2
    assert items[0]["title"] == "Алгоритмы"
    assert events_for_date(events, parse_iso_date("2026-10-11")) == []


def test_bad_schedule_date_is_rejected():
    with pytest.raises(ScheduleError):
        parse_iso_date("12-10-2026")


def test_week_resource_crosses_month_boundary():
    events = load_events(SCHEDULE)
    moment = datetime(2026, 10, 31, 12, 0, tzinfo=UTC)
    monday, sunday = week_bounds(moment, "Europe/Moscow")
    assert monday.isoformat() == "2026-10-26"
    assert sunday.isoformat() == "2026-11-01"
    grouped = week_schedule(events, moment=moment, timezone_name="Europe/Moscow")
    assert list(grouped) == [
        "2026-10-26",
        "2026-10-27",
        "2026-10-28",
        "2026-10-29",
        "2026-10-30",
        "2026-10-31",
        "2026-11-01",
    ]


async def test_weather_city_not_found_with_stub():
    async def fake_get(url, params):
        return {"results": []}

    with pytest.raises(WeatherError, match="не найден"):
        await get_weather("Неттакогорода", http_get=fake_get)


async def test_weather_ambiguous_cities_with_stub():
    async def fake_get(url, params):
        return {
            "results": [
                {"name": "Москва", "country": "Россия", "admin1": "Москва"},
                {"name": "Москва", "country": "США", "admin1": "Idaho"},
            ]
        }

    result = await get_weather("Москва", http_get=fake_get)
    assert result["status"] == "ambiguous"
    assert len(result["candidates"]) == 2


async def test_weather_rostov_prefix_is_ambiguous():
    async def fake_get(url, params):
        return {
            "results": [
                {
                    "name": "Ростов",
                    "country": "Украина",
                    "admin1": "Сумская область",
                    "latitude": 50.6,
                    "longitude": 33.6,
                },
                {
                    "name": "Ростов-на-Дону",
                    "country": "Россия",
                    "admin1": "Ростовская область",
                    "latitude": 47.2,
                    "longitude": 39.7,
                },
            ]
        }

    result = await get_weather("Ростов", http_get=fake_get)
    assert result["status"] == "ambiguous"
    assert len(result["candidates"]) == 2


async def test_weather_success_with_stub():
    async def fake_get(url, params):
        if "geocoding" in url:
            return {
                "results": [
                    {
                        "name": "Казань",
                        "country": "Россия",
                        "admin1": "Татарстан",
                        "latitude": 55.8,
                        "longitude": 49.1,
                    }
                ]
            }
        return {
            "timezone": "Europe/Moscow",
            "current_units": {"wind_speed_10m": "m/s"},
            "current": {
                "time": "2026-10-12T09:00",
                "temperature_2m": 12.5,
                "weather_code": 3,
                "wind_speed_10m": 2.1,
            },
        }

    result = await get_weather("Казань", http_get=fake_get)
    assert result["status"] == "ok"
    assert result["place"] == "Казань"
    assert result["temperature_c"] == 12.5
    assert result["conditions"] == "пасмурно"


def _server_env(**extra: str) -> dict[str, str]:
    env = {
        **os.environ,
        "SCHEDULE_PATH": str(SCHEDULE),
        "DEFAULT_APP_TIMEZONE": "Europe/Moscow",
        "PYTHONPATH": str(ROOT),
        "POSTGRES_PASSWORD": "",
    }
    env.update(extra)
    return env


async def test_mcp_stdio_discovers_tools_and_schedule():
    params = StdioServerParameters(
        command=sys.executable,
        args=["-m", "mcp_server"],
        env=_server_env(),
        cwd=str(ROOT),
    )
    async with stdio_client(params) as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()
            tools = await session.list_tools()
            names = sorted(tool.name for tool in tools.tools)
            assert names == ["add_reminder", "get_schedule", "get_weather"]
            for tool in tools.tools:
                assert tool.inputSchema
                assert "properties" in tool.inputSchema
            resources = await session.list_resources()
            assert any(str(item.uri) == "schedule://current-week" for item in resources.resources)
            result = await session.call_tool("get_schedule", {"date": "2026-10-12"})
            assert result.structuredContent is not None
            assert result.structuredContent["status"] == "ok"
            assert len(result.structuredContent["items"]) == 2
            refused = await session.call_tool(
                "add_reminder",
                {"text": "тест", "remind_at": "2099-01-01T12:00:00+03:00"},
            )
            body = refused.structuredContent or {}
            assert body.get("status") == "error"
            assert "доверенного" in str(body.get("message") or "").casefold()
            forged = await session.call_tool(
                "add_reminder",
                {"text": "тест", "remind_at": "2099-01-01T12:00:00+03:00"},
                meta={"owner_chat_id": 999, "action_key": "forged-key"},
            )
            forged_body = forged.structuredContent or {}
            assert forged_body.get("status") == "error"
            assert "доверенного" in str(forged_body.get("message") or "").casefold()
