"""Клиент Groq. Обработчик Telegram сюда не собирает HTTP-запрос."""

from __future__ import annotations

import json
import logging
import time
import uuid
from collections.abc import Mapping, Sequence
from dataclasses import dataclass
from typing import Any, Protocol

import aiohttp

from app.config import Settings

logger = logging.getLogger("app.llm")

REASONING_EFFORT = "low"
MAX_COMPLETION_TOKENS = 2048

TIMEOUT_MESSAGE = "Не удалось дождаться ответа модели. Повторите запрос чуть позже."
UNAVAILABLE_MESSAGE = "Сервис модели сейчас недоступен. Повторите запрос чуть позже."
EMPTY_MESSAGE = "Модель вернула пустой ответ. Попробуйте переформулировать запрос."
UNEXPECTED_MESSAGE = "Не удалось получить ответ модели. Повторите запрос чуть позже."


class LlmError(Exception):
    """Ошибка модели с текстом, который можно показать пользователю."""

    def __init__(self, user_message: str) -> None:
        super().__init__(user_message)
        self.user_message = user_message


@dataclass(frozen=True)
class ToolCall:
    id: str
    name: str
    arguments: dict[str, Any]


@dataclass(frozen=True)
class ModelTurn:
    content: str
    tool_calls: tuple[ToolCall, ...] = ()


class ChatModel(Protocol):
    async def complete(self, *, temperature: float, messages: Sequence[Mapping[str, str]]) -> str:
        """Возвращает видимый текст ответа без рассуждения модели."""

    async def complete_turn(
        self,
        *,
        temperature: float,
        messages: Sequence[Mapping[str, Any]],
        tools: Sequence[Mapping[str, Any]] | None = None,
    ) -> ModelTurn:
        """Ответ модели: текст и/или структурированные вызовы инструментов."""


class GroqChatClient:
    def __init__(self, settings: Settings) -> None:
        self._settings = settings
        self._session: aiohttp.ClientSession | None = None

    async def complete(self, *, temperature: float, messages: Sequence[Mapping[str, str]]) -> str:
        turn = await self.complete_turn(temperature=temperature, messages=messages, tools=None)
        if not turn.content.strip():
            raise LlmError(EMPTY_MESSAGE)
        return turn.content

    async def complete_turn(
        self,
        *,
        temperature: float,
        messages: Sequence[Mapping[str, Any]],
        tools: Sequence[Mapping[str, Any]] | None = None,
    ) -> ModelTurn:
        request_id = uuid.uuid4().hex[:12]
        model = self._settings.llm_model
        started = time.perf_counter()
        logger.info("LLM вызов начат: id=%s model=%s", request_id, model)
        try:
            turn = await self._request(temperature, messages, tools)
        except LlmError:
            self._log_finish(request_id, model, started, failed=True)
            raise
        self._log_finish(request_id, model, started, failed=False)
        return turn

    async def close(self) -> None:
        if self._session is not None and not self._session.closed:
            await self._session.close()

    async def _request(
        self,
        temperature: float,
        messages: Sequence[Mapping[str, Any]],
        tools: Sequence[Mapping[str, Any]] | None,
    ) -> ModelTurn:
        client_timeout = aiohttp.ClientTimeout(total=self._settings.llm_timeout_seconds)
        session = await self._open(client_timeout)
        url = self._settings.llm_base_url.rstrip("/") + "/chat/completions"
        payload: dict[str, Any] = {
            "model": self._settings.llm_model,
            "temperature": temperature,
            "reasoning_effort": REASONING_EFFORT,
            "include_reasoning": False,
            "max_completion_tokens": MAX_COMPLETION_TOKENS,
            "messages": [dict(item) for item in messages],
        }
        if tools:
            payload["tools"] = list(tools)
            payload["tool_choice"] = "auto"
        headers = {"Authorization": f"Bearer {self._settings.llm_api_key}"}
        try:
            async with session.post(
                url, json=payload, headers=headers, timeout=client_timeout
            ) as response:
                if response.status >= 400:
                    await response.read()
                    raise LlmError(UNAVAILABLE_MESSAGE)
                try:
                    payload_out = await response.json(content_type=None)
                except (aiohttp.ContentTypeError, ValueError):
                    raise LlmError(UNAVAILABLE_MESSAGE) from None
        except LlmError:
            raise
        except TimeoutError:
            raise LlmError(TIMEOUT_MESSAGE) from None
        except aiohttp.ClientError:
            raise LlmError(UNAVAILABLE_MESSAGE) from None
        turn = _parse_turn(payload_out)
        if not turn.content.strip() and not turn.tool_calls:
            raise LlmError(EMPTY_MESSAGE)
        return turn

    async def _open(self, client_timeout: aiohttp.ClientTimeout) -> aiohttp.ClientSession:
        if self._session is None or self._session.closed:
            self._session = aiohttp.ClientSession(timeout=client_timeout)
        return self._session

    def _log_finish(self, request_id: str, model: str, started: float, *, failed: bool) -> None:
        elapsed_ms = (time.perf_counter() - started) * 1000
        outcome = "завершился ошибкой" if failed else "завершён"
        logger.info(
            "LLM вызов %s: id=%s model=%s duration_ms=%.0f",
            outcome,
            request_id,
            model,
            elapsed_ms,
        )


def _parse_turn(payload: object) -> ModelTurn:
    if not isinstance(payload, dict):
        return ModelTurn(content="")
    choices = payload.get("choices")
    if not isinstance(choices, list) or not choices or not isinstance(choices[0], dict):
        return ModelTurn(content="")
    message = choices[0].get("message")
    if not isinstance(message, dict):
        return ModelTurn(content="")
    content = message.get("content")
    text = content.strip() if isinstance(content, str) else ""
    raw_calls = message.get("tool_calls")
    calls: list[ToolCall] = []
    if isinstance(raw_calls, list):
        for item in raw_calls:
            parsed = _parse_tool_call(item)
            if parsed is not None:
                calls.append(parsed)
    return ModelTurn(content=text, tool_calls=tuple(calls))


def _parse_tool_call(item: object) -> ToolCall | None:
    if not isinstance(item, dict):
        return None
    function = item.get("function")
    if not isinstance(function, dict):
        return None
    name = function.get("name")
    if not isinstance(name, str) or not name.strip():
        return None
    raw_args = function.get("arguments")
    if isinstance(raw_args, dict):
        arguments = raw_args
    elif isinstance(raw_args, str):
        try:
            loaded = json.loads(raw_args or "{}")
        except json.JSONDecodeError:
            return None
        if not isinstance(loaded, dict):
            return None
        arguments = loaded
    else:
        arguments = {}
    call_id = item.get("id")
    return ToolCall(
        id=str(call_id or uuid.uuid4().hex[:8]),
        name=name.strip(),
        arguments=arguments,
    )
