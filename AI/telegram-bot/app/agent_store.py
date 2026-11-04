"""Часовые пояса, подтверждения, напоминания и аудит агента."""

from __future__ import annotations

import json
from dataclasses import dataclass
from datetime import UTC, datetime, timedelta
from typing import Any

import asyncpg

CONFIRM_TTL = timedelta(minutes=5)


@dataclass(frozen=True)
class PendingAction:
    id: int
    chat_id: int
    action_key: str
    tool_name: str
    payload: dict[str, Any]
    expires_at: datetime
    status: str


@dataclass(frozen=True)
class AuditRecord:
    requested_at: datetime
    action: str
    tool_name: str | None
    arguments_summary: str
    validation_status: str
    execution_status: str
    rationale: str


@dataclass(frozen=True)
class ReminderRecord:
    id: int
    text: str
    remind_at: datetime
    timezone: str
    created: bool


class AgentStore:
    def __init__(self, pool: asyncpg.Pool) -> None:
        self._pool = pool

    async def get_timezone(self, chat_id: int) -> str | None:
        row = await self._pool.fetchrow(
            "SELECT timezone FROM user_timezones WHERE chat_id = $1",
            chat_id,
        )
        return None if row is None else row["timezone"]

    async def set_timezone(self, chat_id: int, timezone: str) -> None:
        await self._pool.execute(
            """
            INSERT INTO user_timezones (chat_id, timezone, updated_at)
            VALUES ($1, $2, NOW())
            ON CONFLICT (chat_id) DO UPDATE
            SET timezone = EXCLUDED.timezone, updated_at = NOW()
            """,
            chat_id,
            timezone,
        )

    async def save_pending(
        self,
        *,
        chat_id: int,
        action_key: str,
        tool_name: str,
        payload: dict[str, Any],
        now: datetime | None = None,
    ) -> PendingAction:
        moment = now or datetime.now(UTC)
        expires = moment + CONFIRM_TTL
        row = await self._pool.fetchrow(
            """
            INSERT INTO pending_confirmations
                (chat_id, action_key, tool_name, payload, created_at, expires_at, status)
            VALUES ($1, $2, $3, $4::jsonb, $5, $6, 'pending')
            ON CONFLICT (chat_id, action_key) DO UPDATE
            SET tool_name = EXCLUDED.tool_name,
                payload = EXCLUDED.payload,
                created_at = EXCLUDED.created_at,
                expires_at = EXCLUDED.expires_at,
                status = 'pending'
            RETURNING id, chat_id, action_key, tool_name, payload, expires_at, status
            """,
            chat_id,
            action_key,
            tool_name,
            json.dumps(payload, ensure_ascii=False),
            moment,
            expires,
        )
        return _pending(row)

    async def get_pending(self, chat_id: int, action_key: str) -> PendingAction | None:
        row = await self._pool.fetchrow(
            """
            SELECT id, chat_id, action_key, tool_name, payload, expires_at, status
            FROM pending_confirmations
            WHERE chat_id = $1 AND action_key = $2
            """,
            chat_id,
            action_key,
        )
        return None if row is None else _pending(row)

    async def mark_pending(
        self,
        chat_id: int,
        action_key: str,
        status: str,
        *,
        from_status: str = "pending",
    ) -> PendingAction | None:
        row = await self._pool.fetchrow(
            """
            UPDATE pending_confirmations
            SET status = $3
            WHERE chat_id = $1 AND action_key = $2 AND status = $4
            RETURNING id, chat_id, action_key, tool_name, payload, expires_at, status
            """,
            chat_id,
            action_key,
            status,
            from_status,
        )
        return None if row is None else _pending(row)

    async def create_reminder(
        self,
        *,
        chat_id: int,
        text: str,
        remind_at: datetime,
        timezone: str,
        action_key: str,
    ) -> ReminderRecord:
        row = await self._pool.fetchrow(
            """
            INSERT INTO reminders (chat_id, text, remind_at, timezone, action_key)
            VALUES ($1, $2, $3, $4, $5)
            ON CONFLICT (chat_id, action_key) DO UPDATE
            SET text = reminders.text
            RETURNING id, text, remind_at, timezone,
                      (xmax = 0) AS created
            """,
            chat_id,
            text,
            remind_at,
            timezone,
            action_key,
        )
        return ReminderRecord(
            id=row["id"],
            text=row["text"],
            remind_at=row["remind_at"],
            timezone=row["timezone"],
            created=bool(row["created"]),
        )

    async def add_audit(
        self,
        *,
        chat_id: int,
        action: str,
        tool_name: str | None,
        arguments_summary: str,
        validation_status: str,
        execution_status: str,
        rationale: str,
        duration_ms: int,
        requested_at: datetime | None = None,
    ) -> None:
        await self._pool.execute(
            """
            INSERT INTO agent_audit (
                chat_id, requested_at, action, tool_name, arguments_summary,
                validation_status, execution_status, rationale, duration_ms
            )
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)
            """,
            chat_id,
            requested_at or datetime.now(UTC),
            action,
            tool_name,
            arguments_summary,
            validation_status,
            execution_status,
            rationale,
            duration_ms,
        )

    async def last_audit(self, chat_id: int) -> AuditRecord | None:
        row = await self._pool.fetchrow(
            """
            SELECT requested_at, action, tool_name, arguments_summary,
                   validation_status, execution_status, rationale
            FROM agent_audit
            WHERE chat_id = $1
            ORDER BY id DESC
            LIMIT 1
            """,
            chat_id,
        )
        if row is None:
            return None
        return AuditRecord(
            requested_at=row["requested_at"],
            action=row["action"],
            tool_name=row["tool_name"],
            arguments_summary=row["arguments_summary"],
            validation_status=row["validation_status"],
            execution_status=row["execution_status"],
            rationale=row["rationale"],
        )


def _pending(row: asyncpg.Record) -> PendingAction:
    payload = row["payload"]
    if isinstance(payload, str):
        payload = json.loads(payload)
    return PendingAction(
        id=row["id"],
        chat_id=row["chat_id"],
        action_key=row["action_key"],
        tool_name=row["tool_name"],
        payload=dict(payload),
        expires_at=row["expires_at"],
        status=row["status"],
    )
