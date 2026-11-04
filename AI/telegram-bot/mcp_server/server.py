"""MCP-сервер: погода, расписание, напоминания и ресурс недели."""

from __future__ import annotations

import logging
import os
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

import asyncpg
from mcp.server.fastmcp import Context, FastMCP

from mcp_server.meta import action_key as meta_action_key
from mcp_server.meta import is_trusted as meta_is_trusted
from mcp_server.meta import owner_chat_id as meta_owner_chat_id
from mcp_server.meta import timezone_name as meta_timezone_name
from mcp_server.reminders import ReminderError, add_reminder
from mcp_server.schedule_data import (
    ScheduleError,
    events_for_date,
    load_events,
    parse_iso_date,
    week_schedule,
)
from mcp_server.weather import WeatherError, get_weather

logger = logging.getLogger("mcp_server")

_POOL: asyncpg.Pool | None = None
_HTTP_GET = None


def _default_now() -> datetime:
    return datetime.now(UTC)


_NOW_PROVIDER = _default_now


@asynccontextmanager
async def _lifespan(_server: FastMCP) -> AsyncIterator[dict[str, Any]]:
    global _POOL
    password = os.environ.get("POSTGRES_PASSWORD", "")
    if password:
        try:
            pool = await open_pool_from_env()
            from app.schema import ensure_schema

            await ensure_schema(pool)
            _POOL = pool
            logger.info("PostgreSQL для напоминаний подключён.")
        except Exception:
            logger.exception(
                "Не удалось подключить PostgreSQL. Инструменты чтения доступны, "
                "add_reminder будет отвечать ошибкой."
            )
            _POOL = None
    else:
        logger.warning("POSTGRES_PASSWORD пуст: add_reminder будет недоступен.")
        _POOL = None
    try:
        yield {}
    finally:
        if _POOL is not None:
            await _POOL.close()
            _POOL = None


mcp = FastMCP("course-bot-tools", lifespan=_lifespan)


def configure(
    *,
    pool: asyncpg.Pool | None = None,
    http_get=None,
    now_provider=None,
) -> None:
    global _POOL, _HTTP_GET, _NOW_PROVIDER
    if pool is not None:
        _POOL = pool
    if http_get is not None:
        _HTTP_GET = http_get
    if now_provider is not None:
        _NOW_PROVIDER = now_provider


def _schedule_path() -> Path:
    raw = os.environ.get("SCHEDULE_PATH", "data/schedule.json")
    return Path(raw)


def _app_timezone() -> str:
    return os.environ.get("DEFAULT_APP_TIMEZONE", "Europe/Moscow")


def _request_meta(ctx: Context) -> object | None:
    return getattr(ctx.request_context, "meta", None)


@mcp.tool(
    name="get_weather",
    description=(
        "Текущая погода в городе. Чтение. Не выдумывает данные. "
        "В city передавай формулировку пользователя целиком: если названы город и "
        "страна — оба через запятую, без сокращений."
    ),
)
async def tool_get_weather(city: str) -> dict[str, Any]:
    try:
        return await get_weather(city, http_get=_HTTP_GET)
    except WeatherError as exc:
        return {"status": "error", "message": str(exc)}


@mcp.tool(
    name="get_schedule",
    description="Занятия на дату YYYY-MM-DD из учебного расписания. Чтение.",
)
async def tool_get_schedule(date: str) -> dict[str, Any]:
    try:
        day = parse_iso_date(date)
        events = load_events(_schedule_path())
        items = events_for_date(events, day)
        return {"status": "ok", "date": day.isoformat(), "items": items}
    except ScheduleError as exc:
        return {"status": "error", "message": str(exc)}


@mcp.tool(
    name="add_reminder",
    description=(
        "Создать напоминание. Изменение, нужно подтверждение пользователя. "
        "Аргументы: text и remind_at (ISO 8601 с зоной) в будущем."
    ),
)
async def tool_add_reminder(text: str, remind_at: str, ctx: Context) -> dict[str, Any]:
    meta = _request_meta(ctx)
    if not meta_is_trusted(meta):
        return {
            "status": "error",
            "message": "Нет доверенного контекста клиента.",
        }
    owner = meta_owner_chat_id(meta)
    if owner is None:
        return {"status": "error", "message": "Нет владельца напоминания."}
    key = meta_action_key(meta)
    timezone = meta_timezone_name(meta, _app_timezone())
    if not key:
        return {"status": "error", "message": "Нет ключа идемпотентности действия."}
    if _POOL is None:
        return {"status": "error", "message": "База напоминаний недоступна."}
    try:
        result = await add_reminder(
            _POOL,
            owner_chat_id=owner,
            text=text,
            remind_at=remind_at,
            action_key=key,
            timezone=timezone,
        )
        result["status"] = "ok"
        return result
    except ReminderError as exc:
        return {"status": "error", "message": str(exc)}


@mcp.resource("schedule://current-week")
async def resource_current_week() -> dict[str, Any]:
    try:
        events = load_events(_schedule_path())
        grouped = week_schedule(events, moment=_NOW_PROVIDER(), timezone_name=_app_timezone())
        return {"status": "ok", "timezone": _app_timezone(), "days": grouped}
    except ScheduleError as exc:
        return {"status": "error", "message": str(exc)}


async def open_pool_from_env() -> asyncpg.Pool:
    return await asyncpg.create_pool(
        host=os.environ.get("POSTGRES_HOST", "127.0.0.1"),
        port=int(os.environ.get("POSTGRES_PORT", "5432")),
        user=os.environ.get("POSTGRES_USER", "bot"),
        password=os.environ.get("POSTGRES_PASSWORD", ""),
        database=os.environ.get("POSTGRES_DB", "bot"),
        min_size=1,
        max_size=5,
        timeout=5,
        command_timeout=5,
    )
