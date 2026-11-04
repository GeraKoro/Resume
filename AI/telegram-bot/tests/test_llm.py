import asyncio
import logging

import pytest
from aiohttp import web

from app.config import Settings
from app.llm import (
    EMPTY_MESSAGE,
    MAX_COMPLETION_TOKENS,
    REASONING_EFFORT,
    TIMEOUT_MESSAGE,
    UNAVAILABLE_MESSAGE,
    GroqChatClient,
    LlmError,
)

TOKEN = "123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijk"
API_KEY = "llm-secret"
USER_TEXT = "чем список отличается от кортежа"
SYSTEM_TEXT = "системная инструкция секретного режима"


def settings_for(port: int, *, timeout: float = 5) -> Settings:
    return Settings(
        bot_token=TOKEN,
        postgres_password="unused",
        llm_api_key=API_KEY,
        llm_base_url=f"http://127.0.0.1:{port}/v1",
        llm_model="openai/gpt-oss-20b",
        llm_timeout_seconds=timeout,
    )


async def serve(port: int, handler):
    app = web.Application()
    app.router.add_post("/v1/chat/completions", handler)
    runner = web.AppRunner(app)
    await runner.setup()
    await web.TCPSite(runner, "127.0.0.1", port).start()
    return runner


async def test_groq_returns_answer_and_hides_reasoning(unused_tcp_port):
    # Arrange
    seen = {}

    async def handler(request: web.Request) -> web.Response:
        seen["auth"] = request.headers.get("Authorization")
        seen["path"] = request.path
        seen["body"] = await request.json()
        return web.json_response(
            {
                "choices": [
                    {
                        "message": {
                            "content": "Коротко: список хранит порядок.",
                            "reasoning": "скрытая цепочка рассуждений",
                        }
                    }
                ]
            }
        )

    runner = await serve(unused_tcp_port, handler)
    client = GroqChatClient(settings_for(unused_tcp_port))
    try:
        # Act
        answer = await client.complete(
            temperature=0.3,
            messages=[
                {"role": "system", "content": SYSTEM_TEXT},
                {"role": "user", "content": USER_TEXT},
            ],
        )
    finally:
        await client.close()
        await runner.cleanup()
    # Assert
    body = seen["body"]
    assert answer == "Коротко: список хранит порядок."
    assert "рассуждений" not in answer
    assert seen["auth"] == f"Bearer {API_KEY}"
    assert seen["path"] == "/v1/chat/completions"
    assert body["model"] == "openai/gpt-oss-20b"
    assert body["temperature"] == 0.3
    assert body["reasoning_effort"] == REASONING_EFFORT
    assert body["include_reasoning"] is False
    assert body["max_completion_tokens"] == MAX_COMPLETION_TOKENS
    assert [item["role"] for item in body["messages"]] == ["system", "user"]
    assert body["messages"][1]["content"] == USER_TEXT


async def test_http_error_is_safe_message(unused_tcp_port):
    # Arrange
    async def handler(request: web.Request) -> web.Response:
        return web.Response(status=500, text=f"ошибка {API_KEY} {USER_TEXT}")

    runner = await serve(unused_tcp_port, handler)
    client = GroqChatClient(settings_for(unused_tcp_port))
    try:
        # Act / Assert
        with pytest.raises(LlmError) as exc:
            await client.complete(
                temperature=0.0,
                messages=[{"role": "user", "content": USER_TEXT}],
            )
    finally:
        await client.close()
        await runner.cleanup()
    assert exc.value.user_message == UNAVAILABLE_MESSAGE
    assert API_KEY not in str(exc.value)
    assert USER_TEXT not in str(exc.value)


async def test_empty_answer_is_safe_message(unused_tcp_port):
    # Arrange
    async def handler(request: web.Request) -> web.Response:
        return web.json_response(
            {"choices": [{"message": {"content": "  ", "reasoning": "думаю"}}]}
        )

    runner = await serve(unused_tcp_port, handler)
    client = GroqChatClient(settings_for(unused_tcp_port))
    try:
        # Act / Assert
        with pytest.raises(LlmError) as exc:
            await client.complete(
                temperature=0.7,
                messages=[{"role": "user", "content": USER_TEXT}],
            )
    finally:
        await client.close()
        await runner.cleanup()
    assert exc.value.user_message == EMPTY_MESSAGE


async def test_timeout_is_safe_message(unused_tcp_port):
    # Arrange
    async def handler(request: web.Request) -> web.Response:
        await asyncio.sleep(1)
        return web.json_response({"choices": [{"message": {"content": "поздно"}}]})

    runner = await serve(unused_tcp_port, handler)
    client = GroqChatClient(settings_for(unused_tcp_port, timeout=0.2))
    try:
        # Act / Assert
        with pytest.raises(LlmError) as exc:
            await client.complete(
                temperature=1.0,
                messages=[{"role": "user", "content": USER_TEXT}],
            )
    finally:
        await client.close()
        await runner.cleanup()
    assert exc.value.user_message == TIMEOUT_MESSAGE


async def test_log_has_id_model_and_duration_without_secrets(unused_tcp_port, caplog):
    # Arrange
    async def handler(request: web.Request) -> web.Response:
        return web.json_response({"choices": [{"message": {"content": "Готово"}}]})

    runner = await serve(unused_tcp_port, handler)
    client = GroqChatClient(settings_for(unused_tcp_port))
    try:
        # Act
        with caplog.at_level(logging.INFO, logger="app.llm"):
            await client.complete(
                temperature=0.3,
                messages=[
                    {"role": "system", "content": SYSTEM_TEXT},
                    {"role": "user", "content": USER_TEXT},
                ],
            )
    finally:
        await client.close()
        await runner.cleanup()
    # Assert
    rendered = "\n".join(record.getMessage() for record in caplog.records)
    assert "id=" in rendered
    assert "model=openai/gpt-oss-20b" in rendered
    assert "duration_ms=" in rendered
    assert API_KEY not in rendered
    assert SYSTEM_TEXT not in rendered
    assert USER_TEXT not in rendered
