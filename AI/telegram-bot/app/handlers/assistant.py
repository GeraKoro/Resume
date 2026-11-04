"""Команды личного чата. HTTP, SQL и тексты инструкций здесь не собираются."""

from __future__ import annotations

import logging
from datetime import UTC, datetime
from typing import Any, Protocol

from aiogram import F, Router
from aiogram.enums import ChatAction, ChatType
from aiogram.filters import Command, CommandObject
from aiogram.types import (
    CallbackQuery,
    InlineKeyboardButton,
    InlineKeyboardMarkup,
    Message,
)

from app.agent import AgentLoop
from app.agent_store import AgentStore, AuditRecord
from app.config import Settings
from app.dialog import ALLOWED_TEMPERATURES, MODES, DialogMessage, UserPrefs, limit_history
from app.guard import check_request
from app.llm import EMPTY_MESSAGE, UNEXPECTED_MESSAGE, ChatModel, LlmError
from app.mcp_client import DiscoveredTool, McpGateway
from app.prompts import build_messages
from app.store import DialogStore
from app.telegram_text import reply_messages
from app.timezones import HINT, format_in_timezone, format_local_now, validate_timezone

logger = logging.getLogger("app.assistant")

START_TEXT = """
Я учебный ассистент. Команды:
/study — объяснение по программированию
/translate — перевод на названный язык
/brief — конспект в три пункта
/quiz — один вопрос и проверка ответа
/settings — режим, модель и temperature
/reset — очистить историю этого чата
/tools — действия через MCP
/timezone — часовой пояс для напоминаний
/why — почему выбран последний инструмент
""".strip()

MODE_LINES = {
    "study": "Объясняю вопросы по программированию.",
    "translate": "Перевожу текст на названный язык.",
    "brief": "Сжимаю присланный текст в три пункта.",
    "quiz": "Задаю один вопрос по программированию и проверяю ответ.",
}
RESET_TEXT = "История этого чата очищена. Режим и temperature сохранены."
BAD_TEMPERATURE = "Допустимые значения temperature: 0.0, 0.3, 0.7, 1.0."
UNKNOWN_COMMAND_REPLY = "Такой команды нет. Напишите /start, чтобы увидеть список."
TOOLS_UNAVAILABLE = "Инструменты сейчас недоступны. MCP-сервер не подключён."
NO_AUDIT = "Пока нет запросов, по которым можно объяснить решение."
TIMEZONE_UNSET = "Часовой пояс ещё не задан. Пример: /timezone Europe/Moscow"
CONFIRM_EXPIRED = "Подтверждение истекло или уже использовано. Запросите напоминание снова."
CONFIRM_CANCELLED = "Создание напоминания отменено."
CONFIRM_DONE = "Напоминание создано."
CONFIRM_EXISTS = "Это напоминание уже было создано ранее."


class AgentMemory(Protocol):
    async def get_timezone(self, chat_id: int) -> str | None: ...

    async def set_timezone(self, chat_id: int, timezone: str) -> None: ...

    async def last_audit(self, chat_id: int) -> AuditRecord | None: ...


def create_router() -> Router:
    router = Router(name="assistant")
    router.message.filter(F.chat.type == ChatType.PRIVATE)
    # F.text отсекает подпись к фото и документу: исходный README их игнорирует.
    router.message.register(on_start, Command("start", ignore_case=True), F.text)
    router.message.register(on_mode, Command(*MODES, ignore_case=True), F.text)
    router.message.register(on_settings, Command("settings", ignore_case=True), F.text)
    router.message.register(on_reset, Command("reset", ignore_case=True), F.text)
    router.message.register(on_tools, Command("tools", ignore_case=True), F.text)
    router.message.register(on_timezone, Command("timezone", ignore_case=True), F.text)
    router.message.register(on_why, Command("why", ignore_case=True), F.text)
    router.message.register(on_text, F.text)
    router.callback_query.register(on_reminder_confirm, F.data.startswith("rem:"))
    return router


async def on_start(message: Message) -> None:
    await message.answer(START_TEXT, parse_mode=None)


async def on_mode(message: Message, command: CommandObject, store: DialogStore) -> None:
    mode = command.command.casefold()
    await store.set_mode(message.chat.id, mode)
    await store.clear_history(message.chat.id)
    await message.answer(
        f"Режим /{mode}. {MODE_LINES[mode]} История этого чата очищена.",
        parse_mode=None,
    )


async def on_settings(
    message: Message, command: CommandObject, store: DialogStore, settings: Settings
) -> None:
    chat_id = message.chat.id
    argument = (command.args or "").strip()
    if argument:
        if argument not in ALLOWED_TEMPERATURES:
            await message.answer(BAD_TEMPERATURE, parse_mode=None)
            return
        await store.set_temperature(chat_id, argument)
    prefs = await store.get_prefs(chat_id)
    await message.answer(_settings_text(prefs, settings.llm_model), parse_mode=None)


async def on_reset(message: Message, store: DialogStore) -> None:
    await store.clear_history(message.chat.id)
    await message.answer(RESET_TEXT, parse_mode=None)


async def on_tools(message: Message, mcp: McpGateway) -> None:
    if not mcp.available or not mcp.tools:
        await message.answer(TOOLS_UNAVAILABLE, parse_mode=None)
        return
    lines = ["Доступные инструменты:"]
    for tool in mcp.tools:
        lines.append(f"• {_tool_line(tool)}")
    await message.answer("\n".join(lines), parse_mode=None)


async def on_timezone(message: Message, command: CommandObject, agents: AgentMemory) -> None:
    chat_id = message.chat.id
    argument = (command.args or "").strip()
    if not argument:
        saved = await agents.get_timezone(chat_id)
        if saved is None:
            await message.answer(TIMEZONE_UNSET, parse_mode=None)
            return
        local = format_local_now(saved)
        await message.answer(
            f"Текущая зона: {saved}\nСейчас там: {local}",
            parse_mode=None,
        )
        return
    try:
        zone = validate_timezone(argument)
    except ValueError:
        await message.answer(HINT, parse_mode=None)
        return
    await agents.set_timezone(chat_id, zone)
    local = format_local_now(zone)
    await message.answer(
        f"Зона сохранена: {zone}\nСейчас там: {local}",
        parse_mode=None,
    )


async def on_why(message: Message, agents: AgentMemory) -> None:
    record = await agents.last_audit(message.chat.id)
    if record is None:
        await message.answer(NO_AUDIT, parse_mode=None)
        return
    await message.answer(_why_text(record), parse_mode=None)


async def on_text(
    message: Message,
    store: DialogStore,
    settings: Settings,
    model: ChatModel,
    mcp: McpGateway,
    agents: AgentStore,
) -> None:
    text = message.text or ""
    chat_id = message.chat.id
    if _looks_like_command(text):
        await message.answer(UNKNOWN_COMMAND_REPLY, parse_mode=None)
        return
    prefs = await store.get_prefs(chat_id)
    decision = check_request(
        prefs.mode,
        text,
        max_chars=settings.history_max_chars,
        allow_agent_topics=mcp.available,
    )
    if not decision.allowed:
        await message.answer(decision.reply, parse_mode=None)
        return
    history = await store.history(chat_id)
    history.append(DialogMessage("user", text))
    limited = limit_history(
        history,
        max_messages=settings.history_max_messages,
        max_chars=settings.history_max_chars,
    )
    await message.bot.send_chat_action(chat_id, ChatAction.TYPING)
    try:
        if mcp.available:
            result = await AgentLoop(settings=settings, model=model, mcp=mcp, agents=agents).run(
                chat_id=chat_id,
                mode=prefs.mode,
                temperature=float(prefs.temperature),
                history=limited,
                user_text=text,
            )
            answer = result.text
            pending = result.pending_confirm
        else:
            answer = await model.complete(
                temperature=float(prefs.temperature),
                messages=build_messages(prefs.mode, limited),
            )
            pending = None
    except LlmError as exc:
        await message.answer(exc.user_message, parse_mode=None)
        return
    except Exception:
        logger.error("Непредвиденная ошибка вызова модели")
        await message.answer(UNEXPECTED_MESSAGE, parse_mode=None)
        return
    if not answer.strip():
        await message.answer(EMPTY_MESSAGE, parse_mode=None)
        return
    await store.add_message(chat_id, "user", text)
    await store.add_message(chat_id, "assistant", answer)
    markup = _confirm_keyboard(pending) if pending else None
    parts = list(reply_messages(answer))
    for index, (part, parse_mode) in enumerate(parts):
        kwargs: dict[str, Any] = {"parse_mode": parse_mode}
        if markup is not None and index == len(parts) - 1:
            kwargs["reply_markup"] = markup
        await message.answer(part, **kwargs)


async def on_reminder_confirm(
    query: CallbackQuery, agents: AgentStore, mcp: McpGateway, settings: Settings
) -> None:
    data = query.data or ""
    chat = query.message.chat if query.message else None
    if chat is None or chat.type != ChatType.PRIVATE:
        await query.answer()
        return
    chat_id = chat.id
    if not data.startswith("rem:ok:") and not data.startswith("rem:no:"):
        await query.answer()
        return
    action = "ok" if data.startswith("rem:ok:") else "no"
    action_key = data[7:]
    pending = await agents.get_pending(chat_id, action_key)
    if pending is None or pending.status != "pending":
        await query.answer(CONFIRM_EXPIRED, show_alert=True)
        return
    if pending.expires_at <= datetime.now(UTC):
        await agents.mark_pending(chat_id, action_key, "expired")
        await query.answer(CONFIRM_EXPIRED, show_alert=True)
        return
    if action == "no":
        await agents.mark_pending(chat_id, action_key, "cancelled")
        if query.message:
            await query.message.edit_reply_markup(reply_markup=None)
        await query.answer(CONFIRM_CANCELLED)
        if query.message:
            await query.message.answer(CONFIRM_CANCELLED, parse_mode=None)
        return
    if not mcp.available:
        await query.answer(TOOLS_UNAVAILABLE, show_alert=True)
        return
    marked = await agents.mark_pending(chat_id, action_key, "confirmed")
    if marked is None:
        await query.answer(CONFIRM_EXPIRED, show_alert=True)
        return
    try:
        result = await mcp.call_tool(
            "add_reminder",
            {
                "text": pending.payload["text"],
                "remind_at": pending.payload["remind_at"],
            },
            meta={
                "owner_chat_id": chat_id,
                "action_key": action_key,
                "timezone": pending.payload.get("timezone", "Europe/Moscow"),
                "trust_token": settings.mcp_trust_token,
            },
        )
    except Exception:
        logger.exception("Не удалось создать напоминание")
        await agents.mark_pending(chat_id, action_key, "pending", from_status="confirmed")
        await query.answer("Не удалось создать напоминание.", show_alert=True)
        return
    if result.get("status") == "error":
        await agents.mark_pending(chat_id, action_key, "pending", from_status="confirmed")
        message = str(result.get("message") or "Не удалось создать напоминание.")
        await query.answer(message[:180], show_alert=True)
        return
    await agents.mark_pending(chat_id, action_key, "executed", from_status="confirmed")
    await agents.add_audit(
        chat_id=chat_id,
        action="call_tool",
        tool_name="add_reminder",
        arguments_summary=(
            f"текст «{pending.payload.get('text', '')}», "
            f"время {pending.payload.get('remind_at', '')}"
        ),
        validation_status="пройдена",
        execution_status="успешно",
        rationale="пользователь подтвердил создание напоминания",
        duration_ms=0,
        requested_at=datetime.now(UTC),
    )
    if query.message:
        await query.message.edit_reply_markup(reply_markup=None)
    created = result.get("created", True)
    reminder_id = result.get("reminder_id", "—")
    zone = str(result.get("timezone") or pending.payload.get("timezone") or "Europe/Moscow")
    raw_when = result.get("remind_at", pending.payload.get("remind_at"))
    try:
        when = format_in_timezone(str(raw_when), zone)
    except Exception:
        when = str(raw_when)
    text = (
        CONFIRM_EXISTS if not created else f"{CONFIRM_DONE} id={reminder_id}, время {when} ({zone})"
    )
    await query.answer("Готово")
    if query.message:
        await query.message.answer(text, parse_mode=None)


def _confirm_keyboard(pending: dict[str, Any]) -> InlineKeyboardMarkup:
    key = str(pending["action_key"])
    return InlineKeyboardMarkup(
        inline_keyboard=[
            [
                InlineKeyboardButton(text="Подтвердить", callback_data=f"rem:ok:{key}"),
                InlineKeyboardButton(text="Отменить", callback_data=f"rem:no:{key}"),
            ]
        ]
    )


def _tool_line(tool: DiscoveredTool) -> str:
    kind = "изменение, нужно подтверждение" if tool.side_effect else "чтение"
    short = tool.description.split(".")[0].strip() or tool.name
    return f"{tool.name} — {short} ({kind})"


def _why_text(record: AuditRecord) -> str:
    zone = "Europe/Moscow"
    local = record.requested_at.astimezone()
    try:
        from zoneinfo import ZoneInfo

        local = record.requested_at.astimezone(ZoneInfo(zone))
    except Exception:
        pass
    stamp = local.strftime("%d.%m.%Y %H:%M")
    if record.action == "call_tool" and record.tool_name:
        action = f"вызов инструмента {record.tool_name}"
    elif record.action == "clarify":
        action = "уточнение"
    else:
        action = "ответ без инструмента"
    args = record.arguments_summary or "—"
    return (
        f"Последний запрос: {stamp} ({zone})\n"
        f"Действие: {action}\n"
        f"Аргументы: {args}\n"
        f"Проверка: {record.validation_status}\n"
        f"Выполнение: {record.execution_status}\n"
        f"Причина: {record.rationale or '—'}"
    )


def _looks_like_command(text: str) -> bool:
    stripped = text.lstrip()
    return len(stripped) > 1 and stripped.startswith("/") and _word_char(stripped[1])


def _word_char(char: str) -> bool:
    return char == "_" or char.isalnum()


def _settings_text(prefs: UserPrefs, model_name: str) -> str:
    return f"Режим: /{prefs.mode}\nМодель: {model_name}\nTemperature: {prefs.temperature}"
