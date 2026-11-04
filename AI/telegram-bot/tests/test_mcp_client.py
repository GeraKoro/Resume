"""Клиент MCP в боте: discovery и вызов без Telegram/LLM."""

from __future__ import annotations

from pathlib import Path

import pytest

from app.config import Settings
from app.mcp_client import McpGateway

ROOT = Path(__file__).resolve().parents[1]


def _settings(**extra: object) -> Settings:
    values = {
        "bot_token": "1:A" + "B" * 34,
        "postgres_password": "unused",
        "llm_api_key": "unused",
        "schedule_path": str(ROOT / "data" / "schedule.json"),
        "default_app_timezone": "Europe/Moscow",
        "postgres_host": "127.0.0.1",
        "postgres_port": 5433,
        "postgres_db": "bot",
        "postgres_user": "bot",
    }
    values.update(extra)
    return Settings(**values)  # type: ignore[arg-type]


async def test_gateway_discovers_tools_and_reads_schedule():
    gateway = McpGateway(_settings(postgres_password=""), cwd=ROOT)
    try:
        await gateway.start()
        assert gateway.available
        names = sorted(tool.name for tool in gateway.tools)
        assert names == ["add_reminder", "get_schedule", "get_weather"]
        schemas = gateway.openai_tools()
        assert all(item["type"] == "function" for item in schemas)
        weather = gateway.tool("get_weather")
        assert weather is not None
        assert "city" in weather.input_schema.get("properties", {})
        assert weather.side_effect is False
        reminder = gateway.tool("add_reminder")
        assert reminder is not None
        assert reminder.side_effect is True
        result = await gateway.call_tool("get_schedule", {"date": "2026-10-12"})
        assert result["status"] == "ok"
        assert len(result["items"]) == 2
    finally:
        await gateway.close()
        assert gateway.available is False


async def test_gateway_rejects_unknown_tool():
    gateway = McpGateway(_settings(postgres_password=""), cwd=ROOT)
    try:
        await gateway.start()
        with pytest.raises(ValueError, match="Неизвестный"):
            await gateway.call_tool("drop_database", {})
    finally:
        await gateway.close()


async def test_gateway_start_failure_keeps_bot_usable(tmp_path):
    gateway = McpGateway(
        _settings(mcp_server_command=f"{tmp_path / 'missing-binary'}"),
        cwd=ROOT,
    )
    await gateway.start()
    assert gateway.available is False
    assert gateway.tools == []
    assert gateway.last_error
    await gateway.close()
