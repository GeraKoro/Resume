"""Проверка сообщения до вызова модели.

Модуль не читает ключи, токены и пароли. Отклонённый текст нельзя ни отправлять
в Groq, ни писать в историю: ответ пользователю — готовая фраза.
"""

from dataclasses import dataclass

from app.dialog import MODES

EMPTY_REPLY = "Пустое сообщение не отправляю."
LONG_REPLY = "Сообщение слишком длинное. Сократите текст и отправьте снова."
SECRET_REPLY = "Не могу показать секреты или служебные данные."
PROMPT_REPLY = "Не могу показать системную инструкцию."
OFF_TOPIC_REPLY = "Этот запрос не относится к текущему режиму."

_ENV_NAMES = ("llm_api_key", "bot_token", "postgres_password", "telegram_proxy_url")
_FILE_VERBS = (
    "покажи",
    "выведи",
    "пришли",
    "скинь",
    "отправь",
    "прочитай",
    "открой",
    "скажи",
    "напечатай",
    "содержим",
    "dump",
)
_REVEAL = (
    "покажите",
    "покажи",
    "выведите",
    "выведи",
    "пришлите",
    "пришли",
    "скиньте",
    "скинь",
    "отправьте",
    "отправь",
    "дайте",
    "дай",
    "назови",
    "сообщи",
    "скажите",
    "скажи",
    "напечатай",
    "напиши",
    "прочитай",
    "открой",
    "reveal",
    "show me",
    "show",
    "print",
    "dump",
    "give me",
    "tell me",
)
_QUESTION_WORDS = frozenset({"как", "почему", "зачем", "что", "чем"})
_PROMPT_VERBS = (
    "покажите",
    "покажи",
    "выведите",
    "выведи",
    "повтори",
    "напечатай",
    "раскрой",
    "цитируй",
    "скажите",
    "скажи",
    "reveal",
    "show",
    "print",
    "repeat",
    "dump",
)
_STUDY_PARTS = (
    "программ",
    "питон",
    "пайтон",
    "python",
    "javascript",
    "java",
    "typescript",
    "c++",
    "c#",
    "golang",
    "голанг",
    "котлин",
    "kotlin",
    "руби",
    "алгоритм",
    "функци",
    "переменн",
    "цикл",
    "ошибк",
    "компил",
    "синтаксис",
    "рекурси",
    "массив",
    "список",
    "словар",
    "таблиц",
    "строк",
    "символ",
    "файл",
    "модул",
    "пакет",
    "объект",
    "метод",
    "услови",
    "оператор",
    "булев",
    "кортеж",
    "множеств",
    "декоратор",
    "замыкан",
    "генератор",
    "итератор",
    "наследован",
    "интерфейс",
    "указател",
    "асинхрон",
    "исключен",
    "exception",
    "скрипт",
    "библиотек",
    "фреймворк",
    "сортиров",
    "хеш",
    "очеред",
    "коммит",
    "репозитор",
    "docker",
    "linux",
    "терминал",
    "интерпретатор",
    "прокси",
    "секрет",
    ".env",
    "парол",
    "токен",
    "```",
)
_STUDY_WORDS = (
    "rust",
    "php",
    "ruby",
    "json",
    "http",
    "sql",
    "git",
    "api",
    "ооп",
    "стек",
    "bash",
    "def",
    "class",
    "import",
    "closure",
    "for",
    "while",
    "if",
    "код",
    "кода",
    "класс",
)
_TOKEN_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-")


@dataclass(frozen=True)
class GuardDecision:
    allowed: bool
    reply: str = ""


class GuardError(ValueError):
    """Режим не из списка. Текст сообщения в ошибку не входит."""


def check_request(
    mode: str, text: str, *, max_chars: int, allow_agent_topics: bool = False
) -> GuardDecision:
    """Решает, можно ли отдавать сообщение модели."""
    if mode not in MODES:
        raise GuardError("Неизвестный режим.")
    if not text.strip():
        return GuardDecision(False, EMPTY_REPLY)
    if len(text) > max_chars:
        return GuardDecision(False, LONG_REPLY)
    folded = text.lower().replace("ё", "е")
    if _has_credential_shape(text) or _asks_for_secret(folded):
        return GuardDecision(False, SECRET_REPLY)
    if _asks_for_prompt(folded):
        return GuardDecision(False, PROMPT_REPLY)
    if allow_agent_topics:
        return GuardDecision(True)
    if not _on_topic(mode, folded):
        return GuardDecision(False, OFF_TOPIC_REPLY)
    return GuardDecision(True)


def _asks_for_secret(text: str) -> bool:
    if any(_has_word(text, name) for name in _ENV_NAMES):
        return True
    if _asks_for_env_file(text) or _asks_to_reveal_secret(text):
        return True
    if _asks_for_private_key(text) or _owned_secret(text) or _which_secret(text):
        return True
    return "your api key" in text or "your api-key" in text or "my api key" in text


def _asks_for_env_file(text: str) -> bool:
    for name in (".env", ".deploy"):
        start = 0
        while True:
            found = text.find(name, start)
            if found == -1:
                break
            after = text[found + len(name) : found + len(name) + 1]
            if _word(after):
                start = found + 1
                continue
            before = text[max(0, found - 60) : found]
            tail = text[found + len(name) : found + len(name) + 50]
            if _verb_before(before, _FILE_VERBS) or any(
                word in tail for word in ("целиком", "полностью", "содержим")
            ):
                return True
            start = found + 1
    return False


def _asks_to_reveal_secret(text: str) -> bool:
    for verb, after in _reveal_spots(text, _REVEAL):
        if _next_word(text, after) in _QUESTION_WORDS:
            continue
        if _secret_in(text[after : after + 80]):
            return True
        del verb
    return False


def _secret_in(window: str) -> bool:
    if _api_secret(window):
        return True
    if any(_has_word(window, word) for word in ("токен", "пароль", "прокси", "proxy", "password")):
        return True
    if _has_word(window, "token") and ("bot" in window or "telegram" in window):
        return True
    return _key_not_dictionary(window)


def _api_secret(window: str) -> bool:
    start = 0
    while True:
        found = window.find("api", start)
        if found == -1:
            return False
        if _has_word(window, "api") and _near_key(window[found:]):
            return True
        start = found + 1


def _near_key(fragment: str) -> bool:
    cursor = 3
    while cursor < len(fragment) and fragment[cursor] in " _-":
        cursor += 1
    return fragment.startswith("key", cursor) or fragment.startswith("ключ", cursor)


def _key_not_dictionary(window: str) -> bool:
    start = 0
    while True:
        found = window.find("ключ", start)
        if found == -1:
            return False
        if _has_word(window[found:], "ключ") or window.startswith("ключ", found):
            tail = window[found + len("ключ") :].lstrip()
            if not tail.startswith("словар"):
                before = window[found - 1] if found else ""
                after = window[found + len("ключ") : found + len("ключ") + 1]
                if not _word(before) and not _word(after):
                    return True
        start = found + 1


def _asks_for_private_key(text: str) -> bool:
    if _has_word(text, "id_rsa"):
        return True
    if not any(marker in text for marker in ("приватн", "закрыт", "ssh-ключ", "ssh ключ")):
        return False
    return any(_reveal_spots(text, _REVEAL))


def _owned_secret(text: str) -> bool:
    for owner in ("твой", "твоего", "ваш", "вашего", "your"):
        start = 0
        while True:
            found = text.find(owner, start)
            if found == -1:
                break
            if _term_at(text, found, owner):
                window = text[found + len(owner) : found + len(owner) + 40]
                if _secret_in(window.lstrip(" \t")):
                    return True
            start = found + 1
    return False


def _which_secret(text: str) -> bool:
    if "какой пароль" in text:
        return True
    if text.startswith("какой ") and (_api_secret(text) or _has_word(text, "токен")):
        return True
    if ("где лежит" in text or "где находится" in text) and any(
        _has_word(text, word) for word in ("ключ", "токен", "пароль")
    ):
        return True
    if "у тебя" in text or "у вас" in text:
        return any(part in text for part in ("парол", "токен", "прокси")) or _api_secret(text)
    return False


def _asks_for_prompt(text: str) -> bool:
    if _ignore_instructions(text) or _forget_instructions(text):
        return True
    if any(phrase in text for phrase in ("свою инструкц", "твою инструкц", "вашу инструкц")):
        return True
    if _reveal_then_prompt(text) or _prompt_then_demand(text):
        return True
    return False


def _ignore_instructions(text: str) -> bool:
    start = text.find("ignore ")
    while start != -1:
        rest = text[start + len("ignore ") :].lstrip()
        for extra in ("all ", "any "):
            if rest.startswith(extra):
                rest = rest[len(extra) :]
        for prefix in ("previous", "prior", "above"):
            if rest.startswith(prefix + " instructions"):
                return True
        start = text.find("ignore ", start + 1)
    return False


def _forget_instructions(text: str) -> bool:
    for verb in ("забудь", "игнорируй"):
        if verb in text and any(part in text for part in ("инструкц", "правил", "промпт")):
            return True
    return False


def _reveal_then_prompt(text: str) -> bool:
    for _verb, after in _reveal_spots(text, _PROMPT_VERBS):
        if _next_word(text, after) in _QUESTION_WORDS:
            continue
        if _prompt_object(text[after : after + 80]):
            return True
    return False


def _prompt_then_demand(text: str) -> bool:
    for phrase in ("системн", "system prompt"):
        start = 0
        while True:
            found = text.find(phrase, start)
            if found == -1:
                break
            window = text[found : found + 60]
            if phrase == "системн" and not any(part in window for part in ("промпт", "инструкц")):
                start = found + 1
                continue
            tail = text[found : found + 80]
            demands = ("целиком", "дословно", "полностью", "покажи", "выведи")
            if any(word in tail for word in demands):
                return True
            start = found + 1
    return False


def _prompt_object(window: str) -> bool:
    phrases = ("system prompt", "developer message", "developer prompt")
    if any(phrase in window for phrase in phrases):
        return True
    if "промпт" in window or "инструкц" in window:
        return True
    return "системн" in window and "сообщен" in window


def _has_credential_shape(text: str) -> bool:
    return (
        _has_bot_token(text)
        or _has_marked_key(text.lower(), "gsk_", 10)
        or _has_marked_key(text, "sk-", 20)
        or _has_url_password(text)
    )


def _has_bot_token(text: str) -> bool:
    index = 0
    while index < len(text):
        if not text[index].isdigit():
            index += 1
            continue
        start = index
        while index < len(text) and text[index].isdigit():
            index += 1
        length = index - start
        if not 8 <= length <= 12 or index >= len(text) or text[index] != ":":
            continue
        if start and _word(text[start - 1]):
            continue
        tail = index + 1
        count = 0
        while tail < len(text) and text[tail] in _TOKEN_CHARS:
            count += 1
            tail += 1
        if count >= 30 and (tail == len(text) or not _word(text[tail])):
            return True
    return False


def _has_marked_key(text: str, prefix: str, minimum: int) -> bool:
    start = 0
    while True:
        found = text.find(prefix, start)
        if found == -1:
            return False
        if found and _word(text[found - 1]):
            start = found + 1
            continue
        cursor = found + len(prefix)
        count = 0
        while cursor < len(text) and text[cursor] in _TOKEN_CHARS and text[cursor] != "-":
            count += 1
            cursor += 1
        if prefix == "sk-":
            count = 0
            cursor = found + len(prefix)
            while cursor < len(text) and (text[cursor].isalnum() or text[cursor] == "_"):
                count += 1
                cursor += 1
        if count >= minimum and (cursor == len(text) or not _word(text[cursor])):
            return True
        start = found + 1


def _has_url_password(text: str) -> bool:
    lowered = text.lower()
    index = 0
    while index < len(text):
        scheme = ""
        for candidate in ("https://", "http://", "socks5://"):
            if lowered.startswith(candidate, index):
                scheme = candidate
                break
        if not scheme:
            index += 1
            continue
        at = text.find("@", index + len(scheme))
        if at == -1:
            index += 1
            continue
        userinfo = text[index + len(scheme) : at]
        colon = userinfo.find(":")
        if (
            colon > 0
            and colon == userinfo.rfind(":")
            and not any(char in userinfo for char in " \t\r\n/@")
        ):
            return True
        index += 1
    return False


def _on_topic(mode: str, text: str) -> bool:
    if mode == "translate":
        return _is_translate(text)
    if mode == "brief":
        if _is_translate(text) or _is_quiz(text):
            return False
        if _is_brief(text):
            return True
        return not _is_question(text)
    if mode == "quiz":
        if _is_translate(text) or _is_brief(text):
            return False
        if _is_quiz(text):
            return True
        return not _is_question(text)
    if _is_translate(text) or _is_brief(text) or _is_quiz(text):
        return False
    return _is_study(text)


def _is_translate(text: str) -> bool:
    rest = _without_please(text.lstrip())
    for verb in ("переведите", "переведи", "перевести", "translate"):
        if rest.startswith(verb) and _boundary(rest, len(verb)):
            return True
    if not rest.startswith("на "):
        return False
    language = rest[3:]
    for stem in (
        "английск",
        "русск",
        "немецк",
        "французск",
        "испанск",
        "китайск",
        "японск",
        "итальянск",
        "корейск",
    ):
        if not language.startswith(stem):
            continue
        cursor = len(stem)
        while cursor < len(language) and _word(language[cursor]):
            cursor += 1
        if language[cursor:].lstrip().startswith(":"):
            return True
    return False


def _is_brief(text: str) -> bool:
    rest = _without_please(text.lstrip())
    for verb in ("сделай", "составь", "напиши", "дай", "подготовь"):
        if rest.startswith(verb) and _boundary(rest, len(verb)):
            rest = rest[len(verb) :].lstrip()
            break
    if rest.startswith("мне ") or rest.startswith("мне\t"):
        rest = rest[3:].lstrip()
    if rest.startswith("кратк"):
        cursor = 5
        while cursor < len(rest) and _word(rest[cursor]):
            cursor += 1
        if cursor < len(rest) and rest[cursor].isspace():
            rest = rest[cursor:].lstrip()
    if rest.startswith("основные мысли"):
        return True
    for noun, expandable in (
        ("конспект", False),
        ("пересказ", True),
        ("суммир", True),
        ("сожмите", False),
        ("сожми", False),
        ("тезис", True),
    ):
        if not rest.startswith(noun):
            continue
        if expandable:
            cursor = len(noun)
            while cursor < len(rest) and _word(rest[cursor]):
                cursor += 1
            if _boundary(rest, cursor):
                return True
        elif _boundary(rest, len(noun)):
            return True
    return False


def _is_quiz(text: str) -> bool:
    if _has_word(text, "квиз"):
        return True
    rest = _without_please(text.lstrip())
    if rest.startswith("давай ") or rest.startswith("давай\t"):
        rest = rest[6:].lstrip()
    for phrase in ("проверьте меня", "проверь меня", "задайте вопрос", "задай вопрос", "тест"):
        if rest.startswith(phrase) and _boundary(rest, len(phrase)):
            return True
    return False


def _is_question(text: str) -> bool:
    rest = text.lstrip()
    for word in ("что", "почему", "зачем", "как", "объясни", "расскажи", "помоги", "чем"):
        if rest.startswith(word) and _boundary(rest, len(word)):
            return True
    return text.rstrip().endswith("?")


def _is_study(text: str) -> bool:
    if any(part in text for part in _STUDY_PARTS):
        return True
    if any(_has_word(text, word) for word in _STUDY_WORDS):
        return True
    if _number_word(text) or _stem_then(text, "тип", "данн") or _stem_then(text, "баз", "данн", 3):
        return True
    return _stem_then(text, "системн", "промпт")


def _number_word(text: str) -> bool:
    if "чисел" in text:
        return True
    start = 0
    while True:
        found = text.find("числ", start)
        if found == -1:
            return False
        after = found + len("числ")
        if after < len(text) and text[after] in "оаеу":
            nxt = text[after + 1] if after + 1 < len(text) else ""
            if nxt != "н":
                return True
        start = found + 1


def _stem_then(text: str, stem: str, nxt: str, limit: int = 20) -> bool:
    start = 0
    while True:
        found = text.find(stem, start)
        if found == -1:
            return False
        cursor = found + len(stem)
        extra = 0
        while extra < limit and cursor < len(text) and _word(text[cursor]):
            cursor += 1
            extra += 1
        if extra == 0 and stem == "системн":
            start = found + 1
            continue
        while cursor < len(text) and text[cursor].isspace():
            cursor += 1
        if text.startswith(nxt, cursor):
            return True
        start = found + 1


def _without_please(text: str) -> str:
    if text.startswith("пожалуйста"):
        return text[len("пожалуйста") :].lstrip(" ,")
    return text


def _reveal_spots(text: str, verbs: tuple[str, ...]) -> list[tuple[str, int]]:
    spots: list[tuple[str, int]] = []
    for verb in verbs:
        start = 0
        while True:
            found = text.find(verb, start)
            if found == -1:
                break
            if _term_at(text, found, verb):
                spots.append((verb, found + len(verb)))
            start = found + 1
    return spots


def _verb_before(before: str, verbs: tuple[str, ...]) -> bool:
    return any(_reveal_spots(before, verbs))


def _has_word(text: str, word: str) -> bool:
    start = 0
    while True:
        found = text.find(word, start)
        if found == -1:
            return False
        if _term_at(text, found, word):
            return True
        start = found + 1


def _term_at(text: str, found: int, word: str) -> bool:
    before = text[found - 1] if found else ""
    after_at = found + len(word)
    after = text[after_at] if after_at < len(text) else ""
    return not _word(before) and not _word(after)


def _next_word(text: str, index: int) -> str:
    while index < len(text) and not _word(text[index]):
        index += 1
    end = index
    while end < len(text) and _word(text[end]):
        end += 1
    return text[index:end]


def _boundary(text: str, index: int) -> bool:
    return index >= len(text) or not _word(text[index])


def _word(char: str) -> bool:
    return bool(char) and (char == "_" or char.isalnum())
