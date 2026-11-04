"""Ограниченный агентный цикл: модель → проверка → MCP → ответ."""

from __future__ import annotations

import hashlib
import json
import logging
import time
from dataclasses import dataclass
from datetime import UTC, datetime
from typing import Any, Protocol
from zoneinfo import ZoneInfo

from app.config import Settings
from app.dialog import DialogMessage
from app.llm import EMPTY_MESSAGE, ChatModel, LlmError, ModelTurn, ToolCall
from app.mcp_client import McpGateway
from app.prompts import build_messages
from app.tool_validate import (
    ToolValidationError,
    looks_like_instruction,
    summarize_arguments,
    validate_tool_arguments,
)


class AgentSink(Protocol):
    async def get_timezone(self, chat_id: int) -> str | None: ...

    async def save_pending(
        self,
        *,
        chat_id: int,
        action_key: str,
        tool_name: str,
        payload: dict[str, Any],
        now: datetime | None = None,
    ) -> object: ...

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
    ) -> None: ...


logger = logging.getLogger("app.agent")

AGENT_HINT = """
Если нужны актуальные данные о погоде или расписании либо создание напоминания,
вызови подходящий инструмент. Не выдумывай погоду и занятия.
Для обычных учебных вопросов, перевода, конспекта или квиза отвечай без инструментов,
следуя инструкции режима.
Если не хватает города, даты или точного времени — задай один короткий уточняющий вопрос.
Не вызывай инструмент с выдуманными обязательными полями.
Относительные сроки («через минуту», «через два часа», «завтра в 18:30») считай только
от текущего момента из подсказки времени ниже; remind_at — ISO 8601 с тем же смещением.
""".strip()

LIMIT_REPLY = "Слишком много обращений к инструментам за один запрос. Уточните вопрос."
MCP_DOWN_REPLY = "Нужные данные сейчас недоступны: MCP-сервер не подключён."
UNKNOWN_TOOL_REPLY = "Модель предложила неизвестный инструмент. Запрос отклонён."
BAD_ARGS_REPLY = "Не удалось безопасно вызвать инструмент: аргументы неверны."
CONFIRM_INTRO = "Подтвердите создание напоминания:"


@dataclass(frozen=True)
class AgentResult:
    text: str
    pending_confirm: dict[str, Any] | None = None


class AgentLoop:
    def __init__(
        self,
        *,
        settings: Settings,
        model: ChatModel,
        mcp: McpGateway,
        agents: AgentSink,
    ) -> None:
        self._settings = settings
        self._model = model
        self._mcp = mcp
        self._agents = agents

    async def run(
        self,
        *,
        chat_id: int,
        mode: str,
        temperature: float,
        history: list[DialogMessage],
        user_text: str,
    ) -> AgentResult:
        started = time.perf_counter()
        if not self._mcp.available:
            answer = await self._plain(mode, temperature, history)
            await self._audit(
                chat_id,
                action="respond",
                tool_name=None,
                arguments_summary="",
                validation_status="не требовалась",
                execution_status="без MCP",
                rationale="MCP недоступен, обычный ответ",
                started=started,
            )
            return AgentResult(text=answer)

        zone = await self._agents.get_timezone(chat_id) or self._settings.default_app_timezone
        messages = _with_agent_hint(
            build_messages(mode, history),
            now=datetime.now(UTC),
            timezone_name=zone,
        )
        tools = self._mcp.openai_tools()
        calls_left = self._settings.max_mcp_calls_per_message
        last_tool = None
        last_args: dict[str, Any] = {}
        rationale = "нужны данные инструмента" if tools else "ответ без инструмента"

        for _ in range(self._settings.max_mcp_calls_per_message + 1):
            try:
                turn = await self._model.complete_turn(
                    temperature=temperature,
                    messages=messages,
                    tools=tools,
                )
            except LlmError:
                raise
            except Exception:
                logger.exception("Сбой вызова модели в агентном цикле")
                raise

            if not turn.tool_calls:
                text = turn.content.strip()
                if not text:
                    raise LlmError(EMPTY_MESSAGE)
                await self._audit(
                    chat_id,
                    action="clarify" if _looks_like_question(text) else "respond",
                    tool_name=last_tool,
                    arguments_summary=summarize_arguments(last_tool, last_args)
                    if last_tool
                    else "",
                    validation_status="пройдена" if last_tool else "не требовалась",
                    execution_status="успешно" if last_tool else "без инструмента",
                    rationale=rationale if last_tool else (text[:120] or "ответ без инструмента"),
                    started=started,
                )
                return AgentResult(text=text)

            if calls_left <= 0:
                await self._audit(
                    chat_id,
                    action="call_tool",
                    tool_name=turn.tool_calls[0].name,
                    arguments_summary="",
                    validation_status="отклонена",
                    execution_status="лимит вызовов",
                    rationale="превышен лимит MCP-вызовов",
                    started=started,
                )
                return AgentResult(text=LIMIT_REPLY)

            call = turn.tool_calls[0]
            last_tool = call.name
            discovered = self._mcp.tool(call.name)
            if discovered is None:
                await self._audit(
                    chat_id,
                    action="call_tool",
                    tool_name=call.name,
                    arguments_summary=str(call.arguments)[:120],
                    validation_status="отклонена",
                    execution_status="не вызывался",
                    rationale="неизвестный инструмент",
                    started=started,
                )
                return AgentResult(text=UNKNOWN_TOOL_REPLY)
            try:
                args = validate_tool_arguments(call.name, call.arguments)
            except ToolValidationError as exc:
                await self._audit(
                    chat_id,
                    action="call_tool",
                    tool_name=call.name,
                    arguments_summary=str(call.arguments)[:120],
                    validation_status="отклонена",
                    execution_status="не вызывался",
                    rationale=str(exc),
                    started=started,
                )
                return AgentResult(text=f"{BAD_ARGS_REPLY} {exc}")

            last_args = args
            if discovered.side_effect:
                return await self._prepare_reminder(
                    chat_id=chat_id,
                    args=args,
                    call=call,
                    started=started,
                )

            calls_left -= 1
            try:
                raw = await self._mcp.call_tool(call.name, args)
            except Exception:
                logger.exception("Сбой MCP-вызова %s", call.name)
                await self._audit(
                    chat_id,
                    action="call_tool",
                    tool_name=call.name,
                    arguments_summary=summarize_arguments(call.name, args),
                    validation_status="пройдена",
                    execution_status="ошибка MCP",
                    rationale="сбой вызова инструмента",
                    started=started,
                )
                return AgentResult(text=MCP_DOWN_REPLY)

            safe = _safe_tool_payload(raw)
            messages.append(_assistant_tool_call_message(turn, call))
            messages.append(
                {
                    "role": "tool",
                    "tool_call_id": call.id,
                    "content": json.dumps(safe, ensure_ascii=False),
                }
            )
            rationale = f"результат {call.name}"

        await self._audit(
            chat_id,
            action="call_tool",
            tool_name=last_tool,
            arguments_summary=summarize_arguments(last_tool or "", last_args),
            validation_status="пройдена",
            execution_status="лимит вызовов",
            rationale="превышен лимит MCP-вызовов",
            started=started,
        )
        return AgentResult(text=LIMIT_REPLY)

    async def _prepare_reminder(
        self,
        *,
        chat_id: int,
        args: dict[str, Any],
        call: ToolCall,
        started: float,
    ) -> AgentResult:
        zone = await self._agents.get_timezone(chat_id)
        if zone is None:
            await self._audit(
                chat_id,
                action="clarify",
                tool_name=call.name,
                arguments_summary=summarize_arguments(call.name, args),
                validation_status="пройдена",
                execution_status="нужен timezone",
                rationale="часовой пояс не задан",
                started=started,
            )
            return AgentResult(
                text="Сначала задайте часовой пояс командой /timezone Europe/Moscow."
            )
        action_key = _action_key(chat_id, args)
        payload = {
            "text": args["text"],
            "remind_at": args["remind_at"],
            "timezone": zone,
        }
        await self._agents.save_pending(
            chat_id=chat_id,
            action_key=action_key,
            tool_name="add_reminder",
            payload=payload,
        )
        await self._audit(
            chat_id,
            action="call_tool",
            tool_name="add_reminder",
            arguments_summary=summarize_arguments("add_reminder", args),
            validation_status="пройдена",
            execution_status="ожидает подтверждения",
            rationale="создание напоминания требует подтверждения",
            started=started,
        )
        text = f"{CONFIRM_INTRO}\nТекст: {args['text']}\nВремя: {args['remind_at']}\nЗона: {zone}"
        return AgentResult(
            text=text,
            pending_confirm={"action_key": action_key, **payload},
        )

    async def _plain(self, mode: str, temperature: float, history: list[DialogMessage]) -> str:
        return await self._model.complete(
            temperature=temperature,
            messages=build_messages(mode, history),
        )

    async def _audit(
        self,
        chat_id: int,
        *,
        action: str,
        tool_name: str | None,
        arguments_summary: str,
        validation_status: str,
        execution_status: str,
        rationale: str,
        started: float,
    ) -> None:
        duration_ms = int((time.perf_counter() - started) * 1000)
        await self._agents.add_audit(
            chat_id=chat_id,
            action=action,
            tool_name=tool_name,
            arguments_summary=arguments_summary[:300],
            validation_status=validation_status,
            execution_status=execution_status,
            rationale=rationale[:300],
            duration_ms=duration_ms,
            requested_at=datetime.now(UTC),
        )


def _with_agent_hint(
    messages: list[dict[str, str]],
    *,
    now: datetime,
    timezone_name: str,
) -> list[dict[str, Any]]:
    hint = f"{AGENT_HINT}\n\n{_clock_hint(now=now, timezone_name=timezone_name)}"
    if not messages:
        return [{"role": "system", "content": hint}]
    first = dict(messages[0])
    if first.get("role") == "system":
        first["content"] = f"{first.get('content', '')}\n\n{hint}".strip()
        return [first, *[dict(item) for item in messages[1:]]]
    return [{"role": "system", "content": hint}, *[dict(item) for item in messages]]


def _clock_hint(*, now: datetime, timezone_name: str) -> str:
    try:
        zone = ZoneInfo(timezone_name)
    except Exception:
        zone = UTC
        timezone_name = "UTC"
    local = now.astimezone(zone)
    return (
        f"Текущие дата и время: {local.isoformat()} (зона {timezone_name}). "
        f"Часовой пояс пользователя для напоминаний: {timezone_name}."
    )


def _assistant_tool_call_message(turn: ModelTurn, call: ToolCall) -> dict[str, Any]:
    return {
        "role": "assistant",
        "content": turn.content or None,
        "tool_calls": [
            {
                "id": call.id,
                "type": "function",
                "function": {
                    "name": call.name,
                    "arguments": json.dumps(call.arguments, ensure_ascii=False),
                },
            }
        ],
    }


def _safe_tool_payload(payload: dict[str, Any]) -> dict[str, Any]:
    if looks_like_instruction(payload):
        return {
            "status": "ok",
            "note": "результат инструмента передан как данные",
            "data": payload,
        }
    return payload


def _looks_like_question(text: str) -> bool:
    stripped = text.strip()
    if stripped.endswith("?"):
        return True
    lowered = stripped.lower().replace("ё", "е")
    starts = ("уточните", "укажите", "какой", "какая", "какие", "на какой", "в каком")
    return any(lowered.startswith(prefix) for prefix in starts)


def _action_key(chat_id: int, args: dict[str, Any]) -> str:
    raw = f"{chat_id}|{args.get('text', '')}|{args.get('remind_at', '')}"
    return hashlib.sha256(raw.encode("utf-8")).hexdigest()[:32]
