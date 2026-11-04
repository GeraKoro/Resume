"""Создание напоминаний на стороне MCP-сервера."""

from __future__ import annotations

from datetime import UTC, datetime
from typing import Any

import asyncpg


class ReminderError(ValueError):
    """Отказ создания напоминания без секретов в тексте."""


async def add_reminder(
    pool: asyncpg.Pool,
    *,
    owner_chat_id: int | None,
    text: str,
    remind_at: str,
    action_key: str,
    timezone: str,
) -> dict[str, Any]:
    if owner_chat_id is None:
        raise ReminderError("Нет доверенного владельца напоминания.")
    cleaned = text.strip()
    if not cleaned or len(cleaned) > 500:
        raise ReminderError("Текст напоминания должен быть от 1 до 500 символов.")
    try:
        when = datetime.fromisoformat(remind_at)
    except ValueError as exc:
        raise ReminderError("Время напоминания должно быть в формате ISO 8601.") from exc
    if when.tzinfo is None:
        raise ReminderError("У времени напоминания должен быть часовой пояс.")
    if when <= datetime.now(UTC):
        raise ReminderError("Время напоминания должно быть в будущем.")
    row = await pool.fetchrow(
        """
        INSERT INTO reminders (chat_id, text, remind_at, timezone, action_key)
        VALUES ($1, $2, $3, $4, $5)
        ON CONFLICT (chat_id, action_key) DO UPDATE
        SET text = reminders.text
        RETURNING id, text, remind_at, timezone, (xmax = 0) AS created
        """,
        owner_chat_id,
        cleaned,
        when,
        timezone,
        action_key,
    )
    return {
        "reminder_id": row["id"],
        "text": row["text"],
        "remind_at": row["remind_at"].isoformat(),
        "timezone": row["timezone"],
        "created": bool(row["created"]),
    }
