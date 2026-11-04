"""RUN_INTEGRATION=1 python -m pytest -m integration -v.

Тест создаёт собственные контейнер и volume; пользовательскую БД не использует.
"""

import asyncio
import json
import os
import secrets
import socket
import time
from dataclasses import replace
from datetime import UTC, datetime, timedelta

import asyncpg
import pytest

from app.agent_store import AgentStore
from app.config import Settings
from app.db import create_pool
from app.dialog import DEFAULT_TEMPERATURE, DialogMessage, UserPrefs
from app.health import HealthState, health_result
from app.schema import ensure_schema
from app.store import PostgresDialogStore
from mcp_server.reminders import add_reminder
from scripts.common import CommandError, run_command

pytestmark = [
    pytest.mark.integration,
    pytest.mark.skipif(
        os.environ.get("RUN_INTEGRATION") != "1",
        reason="Включите RUN_INTEGRATION=1 для Docker-тестов",
    ),
]


@pytest.fixture
def database():
    name = "itmo-test-" + secrets.token_hex(6)
    volume = name + "-data"
    password = secrets.token_urlsafe(24)

    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]

    def docker(*args):
        return run_command(["docker", *args], label="Интеграционный Docker-тест", timeout=180)

    def wait_until_ready():
        deadline = time.monotonic() + 60
        while True:
            try:
                docker("exec", name, "pg_isready", "-h", "127.0.0.1", "-U", "bot", "-d", "bot")
                return
            except CommandError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(1)

    docker("info")
    docker("volume", "create", volume)
    try:
        docker(
            "run",
            "-d",
            "--name",
            name,
            "-p",
            f"127.0.0.1:{port}:5432",
            "-e",
            "POSTGRES_DB=bot",
            "-e",
            "POSTGRES_USER=bot",
            "-e",
            f"POSTGRES_PASSWORD={password}",
            "--mount",
            f"type=volume,source={volume},target=/var/lib/postgresql/data",
            "postgres:16-bookworm",
        )
        info = json.loads(docker("inspect", name))[0]
        assert int(info["NetworkSettings"]["Ports"]["5432/tcp"][0]["HostPort"]) == port
        wait_until_ready()
        yield (
            Settings(
                bot_token="123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijk",
                postgres_password=password,
                postgres_port=port,
            ),
            docker,
            wait_until_ready,
            name,
        )
    finally:
        try:
            docker("rm", "-f", name)
        finally:
            docker("volume", "rm", volume)


async def test_real_db_password_restart_health_and_persistence(database):
    # Arrange
    settings, docker, wait_until_ready, name = database
    pool = await create_pool(settings)
    task = asyncio.create_task(asyncio.Event().wait())
    state = HealthState(pool=pool, initialized=True, polling_task=task)
    try:
        await pool.execute("CREATE TABLE integration_probe (value text NOT NULL)")
        await pool.execute("INSERT INTO integration_probe VALUES ($1)", "Сохранилось 👋")
        # Act / Assert: реальное подключение и ошибка пароля.
        assert (await health_result(state))[0] == 200
        with pytest.raises(asyncpg.InvalidPasswordError):
            await create_pool(replace(settings, postgres_password="wrong-password"))
        # Act / Assert: потеря соединения и сохранность данных после запуска.
        await asyncio.to_thread(docker, "stop", name)
        assert (await health_result(state))[0] == 503
        await asyncio.to_thread(docker, "start", name)
        await asyncio.to_thread(wait_until_ready)
        for _ in range(60):
            if (await health_result(state))[0] == 200:
                break
            await asyncio.sleep(1)
        assert (await health_result(state))[0] == 200
        assert await pool.fetchval("SELECT value FROM integration_probe") == "Сохранилось 👋"
    finally:
        task.cancel()
        await asyncio.gather(task, return_exceptions=True)
        await pool.close()


async def test_dialog_settings_are_separate_and_default_temperature_is_moderate(database):
    # Arrange
    settings, *_ = database
    pool = await create_pool(settings)
    try:
        await ensure_schema(pool)
        store = PostgresDialogStore(pool)
        # Act
        created = await store.get_prefs(1)
        await store.set_temperature(1, "1.0")
        await store.add_message(1, "user", "вопрос первого")
        await store.add_message(1, "assistant", "ответ первого")
        await store.add_message(2, "user", "вопрос второго")
        await store.set_mode(2, "translate")
        await store.clear_history(1)
        # Assert
        assert created.temperature == DEFAULT_TEMPERATURE
        assert created.mode == "study"
        first = await store.get_prefs(1)
        second = await store.get_prefs(2)
        assert first == UserPrefs(mode="study", temperature="1.0")
        assert second == UserPrefs(mode="translate", temperature=DEFAULT_TEMPERATURE)
        assert await store.history(1) == []
        assert await store.history(2) == [DialogMessage(role="user", content="вопрос второго")]
    finally:
        await pool.close()


async def test_reminder_idempotent_on_same_action_key(database):
    settings, *_ = database
    pool = await create_pool(settings)
    try:
        await ensure_schema(pool)
        agents = AgentStore(pool)
        when = datetime.now(UTC) + timedelta(hours=5)
        first = await agents.create_reminder(
            chat_id=42,
            text="перерыв",
            remind_at=when,
            timezone="Europe/Moscow",
            action_key="same-key-once",
        )
        second = await agents.create_reminder(
            chat_id=42,
            text="другой текст",
            remind_at=when + timedelta(hours=1),
            timezone="Europe/Moscow",
            action_key="same-key-once",
        )
        assert first.created is True
        assert second.created is False
        assert second.id == first.id
        assert second.text == "перерыв"
        count = await pool.fetchval("SELECT COUNT(*) FROM reminders WHERE chat_id = $1", 42)
        assert count == 1

        via_mcp = await add_reminder(
            pool,
            owner_chat_id=42,
            text="ещё раз",
            remind_at=when.isoformat(),
            action_key="same-key-once",
            timezone="Europe/Moscow",
        )
        assert via_mcp["created"] is False
        assert via_mcp["reminder_id"] == first.id
        count = await pool.fetchval("SELECT COUNT(*) FROM reminders WHERE chat_id = $1", 42)
        assert count == 1
    finally:
        await pool.close()
