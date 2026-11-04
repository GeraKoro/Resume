"""Настройки чата и реплики. Запросы параметризованы, чаты не смешиваются."""

from typing import Protocol

import asyncpg

from app.dialog import (
    ALLOWED_TEMPERATURES,
    DEFAULT_MODE,
    DEFAULT_TEMPERATURE,
    MODES,
    DialogMessage,
    UserPrefs,
)


class StoreError(ValueError):
    """Некорректные настройки, которые нельзя записывать в базу."""


class DialogStore(Protocol):
    async def get_prefs(self, chat_id: int) -> UserPrefs: ...

    async def set_mode(self, chat_id: int, mode: str) -> None: ...

    async def set_temperature(self, chat_id: int, temperature: str) -> None: ...

    async def clear_history(self, chat_id: int) -> None: ...

    async def add_message(self, chat_id: int, role: str, content: str) -> None: ...

    async def history(self, chat_id: int) -> list[DialogMessage]: ...


class PostgresDialogStore:
    def __init__(self, pool: asyncpg.Pool) -> None:
        self._pool = pool

    async def get_prefs(self, chat_id: int) -> UserPrefs:
        await self._pool.execute(
            """
            INSERT INTO user_settings (chat_id, mode, temperature)
            VALUES ($1, $2, $3)
            ON CONFLICT (chat_id) DO NOTHING
            """,
            chat_id,
            DEFAULT_MODE,
            DEFAULT_TEMPERATURE,
        )
        row = await self._pool.fetchrow(
            "SELECT mode, temperature FROM user_settings WHERE chat_id = $1",
            chat_id,
        )
        return UserPrefs(mode=row["mode"], temperature=row["temperature"])

    async def set_mode(self, chat_id: int, mode: str) -> None:
        if mode not in MODES:
            raise StoreError("Неизвестный режим.")
        await self._pool.execute(
            """
            INSERT INTO user_settings (chat_id, mode, temperature)
            VALUES ($1, $2, $3)
            ON CONFLICT (chat_id) DO UPDATE SET mode = EXCLUDED.mode
            """,
            chat_id,
            mode,
            DEFAULT_TEMPERATURE,
        )

    async def set_temperature(self, chat_id: int, temperature: str) -> None:
        if temperature not in ALLOWED_TEMPERATURES:
            raise StoreError("Недопустимое значение temperature.")
        await self._pool.execute(
            """
            INSERT INTO user_settings (chat_id, mode, temperature)
            VALUES ($1, $2, $3)
            ON CONFLICT (chat_id) DO UPDATE SET temperature = EXCLUDED.temperature
            """,
            chat_id,
            DEFAULT_MODE,
            temperature,
        )

    async def clear_history(self, chat_id: int) -> None:
        await self._pool.execute("DELETE FROM dialog_messages WHERE chat_id = $1", chat_id)

    async def add_message(self, chat_id: int, role: str, content: str) -> None:
        if role not in {"user", "assistant"}:
            raise StoreError("Недопустимая роль реплики.")
        await self._pool.execute(
            "INSERT INTO dialog_messages (chat_id, role, content) VALUES ($1, $2, $3)",
            chat_id,
            role,
            content,
        )

    async def history(self, chat_id: int) -> list[DialogMessage]:
        rows = await self._pool.fetch(
            """
            SELECT role, content
            FROM dialog_messages
            WHERE chat_id = $1
            ORDER BY id
            """,
            chat_id,
        )
        return [DialogMessage(role=row["role"], content=row["content"]) for row in rows]
