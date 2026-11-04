"""Попытки сломать бота: лишние типы сообщений, чужой SQL и команды не как обычный текст."""

from datetime import UTC, datetime
from unittest.mock import AsyncMock, Mock

import pytest
from aiogram import Bot, Dispatcher
from aiogram.types import Chat, Document, Message, PhotoSize, Sticker, Update, User

from app.dialog import DialogMessage, UserPrefs
from app.handlers.assistant import UNKNOWN_COMMAND_REPLY, create_router
from app.llm import TIMEOUT_MESSAGE, UNEXPECTED_MESSAGE, LlmError
from app.store import PostgresDialogStore
from tests.test_assistant import TOKEN, MemoryStore, _feed, _sent, _settings


def _message(**kwargs) -> Message:
    fields = {
        "message_id": 1,
        "date": datetime.now(UTC),
        "chat": Chat(id=7, type="private"),
        "from_user": User(id=7, is_bot=False, first_name="Студент"),
    }
    fields.update(kwargs)
    return Message(**fields)


async def _feed_message(message: Message, store: MemoryStore):
    bot = Bot(TOKEN)
    bot.session = AsyncMock()
    dispatcher = Dispatcher()
    dispatcher.include_router(create_router())
    model = AsyncMock()
    model.complete = AsyncMock(return_value="ответ")
    from tests.test_assistant import FakeMcp, MemoryAgents

    await dispatcher.feed_update(
        bot,
        Update(update_id=1, message=message),
        store=store,
        settings=_settings(),
        model=model,
        agents=MemoryAgents(),
        mcp=FakeMcp(),
    )
    return bot, model


@pytest.mark.parametrize(
    "message",
    [
        _message(
            sticker=Sticker(
                file_id="a",
                file_unique_id="b",
                type="regular",
                width=1,
                height=1,
                is_animated=False,
                is_video=False,
            )
        ),
        _message(
            document=Document(file_id="a", file_unique_id="b"),
            caption="Что такое список в Python?",
        ),
    ],
)
async def test_non_text_messages_do_not_reach_the_model(message: Message):
    # Arrange
    store = MemoryStore()
    await store.add_message(7, "user", "старое")
    # Act
    bot, model = await _feed_message(message, store)
    # Assert
    assert _sent(bot) == []
    model.complete.assert_not_called()
    assert await store.history(7) == [DialogMessage("user", "старое")]


@pytest.mark.parametrize(
    "message",
    [
        _message(
            photo=[PhotoSize(file_id="a", file_unique_id="b", width=1, height=1)],
            caption="/reset",
        ),
        _message(document=Document(file_id="a", file_unique_id="b"), caption="/study"),
    ],
)
async def test_command_in_a_caption_is_ignored(message: Message):
    # Исходный README: фото, документы и подписи к ним игнорируются.
    # Arrange
    store = MemoryStore()
    await store.add_message(7, "user", "старое")
    # Act
    bot, model = await _feed_message(message, store)
    # Assert
    assert _sent(bot) == []
    model.complete.assert_not_called()
    assert await store.history(7) == [DialogMessage("user", "старое")]


async def test_mixed_case_command_still_switches_mode():
    # Arrange
    store = MemoryStore()
    await store.add_message(7, "user", "старое")
    # Act
    bot, store, model = await _feed("/Study", store=store)
    # Assert
    assert _sent(bot)[0].text.startswith("Режим /study.")
    assert await store.history(7) == []
    model.complete.assert_not_called()


async def test_sql_text_stays_a_parameter():
    # Arrange
    pool = Mock()
    pool.execute = AsyncMock()
    store = PostgresDialogStore(pool)
    payload = "'); DROP TABLE dialog_messages; --"
    # Act
    await store.add_message(3, "user", payload)
    # Assert
    sql, chat_id, role, content = pool.execute.await_args.args
    assert "DROP" not in sql
    assert (chat_id, role, content) == (3, "user", payload)


async def test_next_message_works_after_model_error():
    # Arrange
    store = MemoryStore()
    failed = AsyncMock()
    failed.complete = AsyncMock(side_effect=LlmError(TIMEOUT_MESSAGE))
    recovered = AsyncMock()
    recovered.complete = AsyncMock(return_value="Список хранит элементы по порядку.")
    # Act
    await _feed("Что такое список в Python?", store=store, model=failed, update_id=1)
    bot, store, recovered = await _feed(
        "Что такое список в Python?",
        store=store,
        model=recovered,
        update_id=2,
    )
    # Assert
    assert _sent(bot)[0].text == "Список хранит элементы по порядку."
    assert [item.role for item in await store.history(7)] == ["user", "assistant"]


async def test_channel_message_is_ignored():
    # Act
    bot, _, model = await _feed("Что такое список в Python?", chat_type="channel")
    # Assert
    assert _sent(bot) == []
    model.complete.assert_not_called()


async def test_null_byte_in_a_study_question_still_gets_an_answer():
    # Arrange
    model = AsyncMock()
    model.complete = AsyncMock(return_value="ответ")
    # Act
    bot, _, model = await _feed("Что такое список в Python?\x00", model=model)
    # Assert
    assert _sent(bot)[0].text == "ответ"
    assert model.complete.await_args.kwargs["messages"][-1]["content"].endswith("\x00")


@pytest.mark.parametrize("mode", ["brief", "quiz"])
async def test_unknown_command_does_not_call_the_model(mode: str):
    # Arrange
    store = MemoryStore()
    store.prefs[7] = UserPrefs(mode, "0.3")
    await store.add_message(7, "user", "старое")
    # Act
    bot, store, model = await _feed("/help", store=store)
    # Assert
    assert _sent(bot)[0].text == UNKNOWN_COMMAND_REPLY
    model.complete.assert_not_called()
    assert await store.history(7) == [DialogMessage("user", "старое")]


async def test_settings_command_ignores_letter_case():
    # Act
    bot, store, model = await _feed("/SETTINGS 1.0")
    # Assert
    assert (await store.get_prefs(7)).temperature == "1.0"
    assert "1.0" in _sent(bot)[0].text
    model.complete.assert_not_called()


async def test_unexpected_model_error_is_a_safe_reply():
    # Arrange
    model = AsyncMock()
    model.complete = AsyncMock(side_effect=RuntimeError("boom llm-secret"))
    # Act
    bot, store, _ = await _feed("Что такое список в Python?", model=model)
    # Assert
    assert _sent(bot)[0].text == UNEXPECTED_MESSAGE
    assert "llm-secret" not in _sent(bot)[0].text
    assert await store.history(7) == []
