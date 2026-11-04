"""Таблицы диалога и агента. Создаются при старте, схему деплой сам не меняет."""

import asyncpg

STATEMENTS = (
    """
    CREATE TABLE IF NOT EXISTS user_settings (
        chat_id BIGINT PRIMARY KEY,
        mode TEXT NOT NULL,
        temperature TEXT NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS dialog_messages (
        id BIGSERIAL PRIMARY KEY,
        chat_id BIGINT NOT NULL,
        role TEXT NOT NULL,
        content TEXT NOT NULL
    )
    """,
    """
    CREATE INDEX IF NOT EXISTS dialog_messages_chat_idx
        ON dialog_messages (chat_id, id)
    """,
    """
    CREATE TABLE IF NOT EXISTS user_timezones (
        chat_id BIGINT PRIMARY KEY,
        timezone TEXT NOT NULL,
        updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS reminders (
        id BIGSERIAL PRIMARY KEY,
        chat_id BIGINT NOT NULL,
        text TEXT NOT NULL,
        remind_at TIMESTAMPTZ NOT NULL,
        timezone TEXT NOT NULL,
        action_key TEXT NOT NULL,
        created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
        UNIQUE (chat_id, action_key)
    )
    """,
    """
    CREATE INDEX IF NOT EXISTS reminders_chat_idx ON reminders (chat_id, remind_at)
    """,
    """
    CREATE TABLE IF NOT EXISTS pending_confirmations (
        id BIGSERIAL PRIMARY KEY,
        chat_id BIGINT NOT NULL,
        action_key TEXT NOT NULL,
        tool_name TEXT NOT NULL,
        payload JSONB NOT NULL,
        created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
        expires_at TIMESTAMPTZ NOT NULL,
        status TEXT NOT NULL DEFAULT 'pending',
        UNIQUE (chat_id, action_key)
    )
    """,
    """
    CREATE INDEX IF NOT EXISTS pending_confirmations_chat_idx
        ON pending_confirmations (chat_id, created_at DESC)
    """,
    """
    CREATE TABLE IF NOT EXISTS agent_audit (
        id BIGSERIAL PRIMARY KEY,
        chat_id BIGINT NOT NULL,
        requested_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
        action TEXT NOT NULL,
        tool_name TEXT,
        arguments_summary TEXT NOT NULL DEFAULT '',
        validation_status TEXT NOT NULL,
        execution_status TEXT NOT NULL,
        rationale TEXT NOT NULL DEFAULT '',
        duration_ms INTEGER NOT NULL DEFAULT 0
    )
    """,
    """
    CREATE INDEX IF NOT EXISTS agent_audit_chat_idx
        ON agent_audit (chat_id, id DESC)
    """,
)


async def ensure_schema(pool: asyncpg.Pool) -> None:
    async with pool.acquire() as connection:
        for statement in STATEMENTS:
            await connection.execute(statement)
