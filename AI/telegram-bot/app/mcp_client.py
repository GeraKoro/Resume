"""MCP-клиент бота: discovery и вызов инструментов через stdio."""

from __future__ import annotations

import json
import logging
import os
import sys
from contextlib import AsyncExitStack
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

from app.config import Settings

logger = logging.getLogger("app.mcp")


@dataclass(frozen=True)
class DiscoveredTool:
    name: str
    description: str
    input_schema: dict[str, Any]
    side_effect: bool


class McpGateway:
    def __init__(self, settings: Settings, *, cwd: Path | None = None) -> None:
        self._settings = settings
        self._cwd = cwd or Path.cwd()
        self._stack: AsyncExitStack | None = None
        self._session: ClientSession | None = None
        self._tools: list[DiscoveredTool] = []
        self.available = False
        self.last_error = ""

    @property
    def tools(self) -> list[DiscoveredTool]:
        return list(self._tools)

    def tool(self, name: str) -> DiscoveredTool | None:
        for item in self._tools:
            if item.name == name:
                return item
        return None

    def openai_tools(self) -> list[dict[str, Any]]:
        return [
            {
                "type": "function",
                "function": {
                    "name": tool.name,
                    "description": tool.description,
                    "parameters": tool.input_schema,
                },
            }
            for tool in self._tools
        ]

    async def start(self) -> None:
        if self._stack is not None:
            return
        command, args = self._command()
        schedule = Path(self._settings.schedule_path)
        if not schedule.is_absolute():
            schedule = self._cwd / schedule
        env = {
            **os.environ,
            "SCHEDULE_PATH": str(schedule),
            "DEFAULT_APP_TIMEZONE": self._settings.default_app_timezone,
            "POSTGRES_HOST": self._settings.postgres_host,
            "POSTGRES_PORT": str(self._settings.postgres_port),
            "POSTGRES_DB": self._settings.postgres_db,
            "POSTGRES_USER": self._settings.postgres_user,
            "POSTGRES_PASSWORD": self._settings.postgres_password,
            "WEATHER_HTTP_TIMEOUT_SECONDS": str(self._settings.weather_http_timeout_seconds),
            "MCP_TRUST_TOKEN": self._settings.mcp_trust_token,
            "BOT_TOKEN": self._settings.bot_token,
            "PYTHONPATH": str(self._cwd),
        }
        params = StdioServerParameters(
            command=command,
            args=args,
            env=env,
            cwd=str(self._cwd),
        )
        stack = AsyncExitStack()
        try:
            read, write = await stack.enter_async_context(stdio_client(params))
            session = await stack.enter_async_context(ClientSession(read, write))
            await session.initialize()
            listed = await session.list_tools()
            self._tools = [_as_tool(item) for item in listed.tools]
            self._stack = stack
            self._session = session
            self.available = True
            self.last_error = ""
            logger.info("MCP подключён, инструментов: %s", len(self._tools))
        except Exception as exc:
            self.available = False
            self.last_error = type(exc).__name__
            self._tools = []
            logger.exception("Не удалось подключить MCP-сервер")
            await stack.aclose()
            self._stack = None
            self._session = None

    async def close(self) -> None:
        if self._stack is not None:
            await self._stack.aclose()
        self._stack = None
        self._session = None
        self._tools = []
        self.available = False

    async def call_tool(
        self,
        name: str,
        arguments: dict[str, Any],
        *,
        meta: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        if self._session is None or not self.available:
            raise RuntimeError("MCP недоступен")
        if self.tool(name) is None:
            raise ValueError("Неизвестный инструмент.")
        trusted = dict(meta or {})
        trusted["trust_token"] = self._settings.mcp_trust_token
        result = await self._session.call_tool(name, arguments=arguments, meta=trusted)
        if result.isError:
            texts = [
                getattr(block, "text", "")
                for block in result.content
                if getattr(block, "text", None)
            ]
            message = "\n".join(texts).strip() or "Инструмент вернул ошибку."
            return {"status": "error", "message": message}
        if result.structuredContent and isinstance(result.structuredContent, dict):
            return dict(result.structuredContent)
        texts = []
        for block in result.content:
            text = getattr(block, "text", None)
            if text:
                texts.append(text)
        joined = "\n".join(texts).strip()
        if joined.startswith("{") and joined.endswith("}"):
            try:
                payload = json.loads(joined)
                if isinstance(payload, dict):
                    return payload
            except json.JSONDecodeError:
                pass
        return {"status": "ok", "text": joined}

    def _command(self) -> tuple[str, list[str]]:
        custom = self._settings.mcp_server_command.strip()
        if custom:
            parts = custom.split()
            return parts[0], parts[1:]
        return sys.executable, ["-m", "mcp_server"]


def _as_tool(item: Any) -> DiscoveredTool:
    name = str(getattr(item, "name", ""))
    description = str(getattr(item, "description", "") or "")
    schema = getattr(item, "inputSchema", None) or {"type": "object", "properties": {}}
    side_effect = name == "add_reminder" or "изменен" in description.casefold()
    return DiscoveredTool(
        name=name,
        description=description,
        input_schema=dict(schema),
        side_effect=side_effect,
    )
