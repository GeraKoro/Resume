"""Идемпотентность напоминаний без Docker: повтор того же action_key."""

from __future__ import annotations

from datetime import UTC, datetime, timedelta
from typing import Any
from unittest.mock import AsyncMock

import pytest

from app.agent_store import AgentStore
from mcp_server.reminders import ReminderError, add_reminder


class _Row(dict):
    def __getitem__(self, key: str) -> Any:
        return dict.__getitem__(self, key)


@pytest.mark.asyncio
async def test_agent_store_create_reminder_idempotent():
    when = datetime.now(UTC) + timedelta(hours=2)
    pool = AsyncMock()
    pool.fetchrow = AsyncMock(
        side_effect=[
            _Row(
                {
                    "id": 7,
                    "text": "перерыв",
                    "remind_at": when,
                    "timezone": "Europe/Moscow",
                    "created": True,
                }
            ),
            _Row(
                {
                    "id": 7,
                    "text": "перерыв",
                    "remind_at": when,
                    "timezone": "Europe/Moscow",
                    "created": False,
                }
            ),
        ]
    )
    store = AgentStore(pool)
    first = await store.create_reminder(
        chat_id=1,
        text="перерыв",
        remind_at=when,
        timezone="Europe/Moscow",
        action_key="k1",
    )
    second = await store.create_reminder(
        chat_id=1,
        text="другое",
        remind_at=when,
        timezone="Europe/Moscow",
        action_key="k1",
    )
    assert first.created is True
    assert second.created is False
    assert second.id == first.id
    assert pool.fetchrow.await_count == 2


@pytest.mark.asyncio
async def test_mcp_add_reminder_requires_owner_and_is_idempotent():
    when = (datetime.now(UTC) + timedelta(hours=2)).isoformat()
    with pytest.raises(ReminderError) as missing:
        await add_reminder(
            AsyncMock(),
            owner_chat_id=None,
            text="перерыв",
            remind_at=when,
            action_key="k2",
            timezone="Europe/Moscow",
        )
    assert "владельца" in str(missing.value)

    pool = AsyncMock()
    pool.fetchrow = AsyncMock(
        side_effect=[
            _Row(
                {
                    "id": 3,
                    "text": "перерыв",
                    "remind_at": datetime.fromisoformat(when),
                    "timezone": "Europe/Moscow",
                    "created": True,
                }
            ),
            _Row(
                {
                    "id": 3,
                    "text": "перерыв",
                    "remind_at": datetime.fromisoformat(when),
                    "timezone": "Europe/Moscow",
                    "created": False,
                }
            ),
        ]
    )
    first = await add_reminder(
        pool,
        owner_chat_id=9,
        text="перерыв",
        remind_at=when,
        action_key="k2",
        timezone="Europe/Moscow",
    )
    second = await add_reminder(
        pool,
        owner_chat_id=9,
        text="ещё",
        remind_at=when,
        action_key="k2",
        timezone="Europe/Moscow",
    )
    assert first["created"] is True
    assert second["created"] is False
    assert second["reminder_id"] == first["reminder_id"]
