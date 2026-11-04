"""Чтение обезличенного расписания. Путь задаёт конфигурация, не модель."""

from __future__ import annotations

import json
from datetime import date, datetime, timedelta
from pathlib import Path
from zoneinfo import ZoneInfo


class ScheduleError(ValueError):
    """Ошибка расписания без путей и секретов в тексте для пользователя."""


def load_events(path: Path) -> list[dict]:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ScheduleError("Файл расписания недоступен.") from exc
    except (OSError, json.JSONDecodeError) as exc:
        raise ScheduleError("Файл расписания повреждён.") from exc
    events = raw.get("events")
    if not isinstance(events, list):
        raise ScheduleError("Файл расписания повреждён.")
    return [item for item in events if isinstance(item, dict)]


def events_for_date(events: list[dict], day: date) -> list[dict]:
    key = day.isoformat()
    items = []
    for event in events:
        if event.get("date") != key:
            continue
        items.append(
            {
                "start": str(event.get("start", "")),
                "end": str(event.get("end", "")),
                "title": str(event.get("title", "")),
                "location": str(event.get("location", "")),
            }
        )
    items.sort(key=lambda item: item["start"])
    return items


def parse_iso_date(value: str) -> date:
    try:
        return date.fromisoformat(value)
    except ValueError as exc:
        raise ScheduleError("Дата должна быть в формате YYYY-MM-DD.") from exc


def week_bounds(moment: datetime, timezone_name: str) -> tuple[date, date]:
    local = moment.astimezone(ZoneInfo(timezone_name)).date()
    monday = local - timedelta(days=local.weekday())
    sunday = monday + timedelta(days=6)
    return monday, sunday


def week_schedule(
    events: list[dict], *, moment: datetime, timezone_name: str
) -> dict[str, list[dict]]:
    monday, sunday = week_bounds(moment, timezone_name)
    grouped: dict[str, list[dict]] = {}
    day = monday
    while day <= sunday:
        grouped[day.isoformat()] = events_for_date(events, day)
        day += timedelta(days=1)
    return grouped
