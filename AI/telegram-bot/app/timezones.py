"""Проверка IANA-зон и локального времени для /timezone."""

from __future__ import annotations

from datetime import datetime
from zoneinfo import ZoneInfo, ZoneInfoNotFoundError

HINT = "Укажите зону как в базе IANA, например Europe/Moscow."


def validate_timezone(name: str) -> str:
    cleaned = name.strip()
    if not cleaned or any(ch.isspace() for ch in cleaned):
        raise ValueError(HINT)
    try:
        ZoneInfo(cleaned)
    except ZoneInfoNotFoundError as exc:
        raise ValueError(HINT) from exc
    except Exception as exc:
        raise ValueError(HINT) from exc
    return cleaned


def format_local_now(timezone_name: str, *, now: datetime | None = None) -> str:
    zone = ZoneInfo(timezone_name)
    moment = (now or datetime.now(tz=zone)).astimezone(zone)
    return moment.strftime("%d.%m.%Y %H:%M")


def format_in_timezone(value: str | datetime, timezone_name: str) -> str:
    """Показать момент в IANA-зоне пользователя (для ответа о напоминании)."""
    zone = ZoneInfo(timezone_name)
    if isinstance(value, datetime):
        moment = value
    else:
        moment = datetime.fromisoformat(str(value).strip())
    if moment.tzinfo is None:
        moment = moment.replace(tzinfo=zone)
    return moment.astimezone(zone).isoformat()
