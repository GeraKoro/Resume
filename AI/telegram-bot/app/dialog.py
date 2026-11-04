"""Ограничение истории диалога. Объём оценивается числом символов."""

from dataclasses import dataclass

ALLOWED_TEMPERATURES = ("0.0", "0.3", "0.7", "1.0")
DEFAULT_TEMPERATURE = "0.3"
DEFAULT_MODE = "study"
MODES = ("study", "translate", "brief", "quiz")


@dataclass(frozen=True)
class UserPrefs:
    mode: str
    temperature: str


@dataclass(frozen=True)
class DialogMessage:
    role: str
    content: str


def limit_history(
    messages: list[DialogMessage], *, max_messages: int, max_chars: int
) -> list[DialogMessage]:
    """Оставляет связный хвост истории.

    Текущий запрос — последнее сообщение — не удаляется. Лишние реплики
    снимаются с начала. Ведущий ответ ассистента без вопроса пользователя
    тоже снимается, чтобы модели не уходил обрывок пары.
    """
    selected = list(messages)
    while len(selected) > 1 and (len(selected) > max_messages or _chars(selected) > max_chars):
        selected.pop(0)
    while len(selected) > 1 and selected[0].role == "assistant":
        selected.pop(0)
    return selected


def _chars(messages: list[DialogMessage]) -> int:
    return sum(len(message.content) for message in messages)
