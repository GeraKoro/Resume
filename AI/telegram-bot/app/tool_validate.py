"""Проверка аргументов инструментов до MCP. Без регулярных выражений."""

from __future__ import annotations

from datetime import UTC, date, datetime
from typing import Any


class ToolValidationError(ValueError):
    """Аргументы не прошли схему."""


def validate_tool_arguments(name: str, arguments: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(arguments, dict):
        raise ToolValidationError("Аргументы инструмента должны быть объектом.")
    if name == "get_weather":
        return _weather(arguments)
    if name == "get_schedule":
        return _schedule(arguments)
    if name == "add_reminder":
        return _reminder(arguments)
    raise ToolValidationError("Неизвестный инструмент.")


def _weather(arguments: dict[str, Any]) -> dict[str, Any]:
    _no_extra(arguments, {"city"})
    city = arguments.get("city")
    if not isinstance(city, str):
        raise ToolValidationError("Поле city должно быть строкой.")
    cleaned = city.strip()
    if not cleaned or len(cleaned) > 100:
        raise ToolValidationError("Название города должно быть от 1 до 100 символов.")
    return {"city": cleaned}


def _schedule(arguments: dict[str, Any]) -> dict[str, Any]:
    _no_extra(arguments, {"date"})
    value = arguments.get("date")
    if not isinstance(value, str):
        raise ToolValidationError("Поле date должно быть строкой YYYY-MM-DD.")
    try:
        day = date.fromisoformat(value.strip())
    except ValueError as exc:
        raise ToolValidationError("Дата должна быть в формате YYYY-MM-DD.") from exc
    return {"date": day.isoformat()}


def _reminder(arguments: dict[str, Any]) -> dict[str, Any]:
    _no_extra(arguments, {"text", "remind_at"})
    text = arguments.get("text")
    remind_at = arguments.get("remind_at")
    if not isinstance(text, str):
        raise ToolValidationError("Поле text должно быть строкой.")
    cleaned = text.strip()
    if not cleaned or len(cleaned) > 500:
        raise ToolValidationError("Текст напоминания должен быть от 1 до 500 символов.")
    if not isinstance(remind_at, str):
        raise ToolValidationError("Поле remind_at должно быть строкой ISO 8601.")
    try:
        when = datetime.fromisoformat(remind_at.strip())
    except ValueError as exc:
        raise ToolValidationError("Время напоминания должно быть в формате ISO 8601.") from exc
    if when.tzinfo is None:
        raise ToolValidationError("У времени напоминания должен быть часовой пояс.")
    if when <= datetime.now(UTC):
        raise ToolValidationError("Время напоминания должно быть в будущем.")
    return {"text": cleaned, "remind_at": when.isoformat()}


def _no_extra(arguments: dict[str, Any], allowed: set[str]) -> None:
    extra = sorted(key for key in arguments if key not in allowed)
    if extra:
        raise ToolValidationError("Лишние поля аргументов недопустимы.")


def summarize_arguments(name: str, arguments: dict[str, Any]) -> str:
    if name == "get_weather":
        return f"город «{arguments.get('city', '')}»"
    if name == "get_schedule":
        return f"дата {arguments.get('date', '')}"
    if name == "add_reminder":
        return f"текст «{arguments.get('text', '')}», время {arguments.get('remind_at', '')}"
    return "—"


def looks_like_instruction(payload: object) -> bool:
    """Грубая проверка на текст, похожий на инструкцию модели."""
    text = _as_text(payload).lower().replace("ё", "е")
    markers = (
        "игнорируй предыдущие",
        "ignore previous",
        "system prompt",
        "системный промпт",
        "ты теперь",
        "forget your instructions",
        "раскрой инструкц",
    )
    return any(marker in text for marker in markers)


def _as_text(payload: object) -> str:
    if isinstance(payload, str):
        return payload
    if isinstance(payload, dict):
        parts = []
        for value in payload.values():
            parts.append(_as_text(value))
        return " ".join(parts)
    if isinstance(payload, list):
        return " ".join(_as_text(item) for item in payload)
    return str(payload)
