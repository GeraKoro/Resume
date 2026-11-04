import logging
from urllib.parse import unquote, urlsplit

from app.config import Settings


class SecretFilter(logging.Filter):
    def __init__(self, secrets: list[str]):
        super().__init__()
        self.secrets = sorted({s for s in secrets if s}, key=len, reverse=True)

    def filter(self, record: logging.LogRecord) -> bool:
        text = record.getMessage()
        if record.exc_info:
            text += "\n" + logging.Formatter().formatException(record.exc_info)
        if record.stack_info:
            text += "\n" + record.stack_info
        for secret in self.secrets:
            text = text.replace(secret, "[скрыто]")
        text = _hide_url_passwords(text)
        record.msg, record.args = text, ()
        record.exc_info = record.exc_text = record.stack_info = None
        return True


def _hide_url_passwords(text: str) -> str:
    output: list[str] = []
    index = 0
    lowered = text.lower()
    while index < len(text):
        scheme = _url_scheme(lowered, index)
        if scheme is None:
            output.append(text[index])
            index += 1
            continue
        userinfo_end = _userinfo_end(text, index + len(scheme))
        if userinfo_end is None:
            output.append(text[index])
            index += 1
            continue
        output.append(text[index : index + len(scheme)])
        output.append("[скрыто]@")
        index = userinfo_end
    return "".join(output)


def _url_scheme(lowered: str, index: int) -> str | None:
    for scheme in ("https://", "http://", "socks5://"):
        if lowered.startswith(scheme, index):
            return scheme
    return None


def _userinfo_end(text: str, start: int) -> int | None:
    at = text.find("@", start)
    if at == -1:
        return None
    userinfo = text[start:at]
    colon = userinfo.find(":")
    if colon <= 0 or colon != userinfo.rfind(":"):
        return None
    user = userinfo[:colon]
    password = userinfo[colon + 1 :]
    if not user or not password:
        return None
    if any(char in userinfo for char in " \t\r\n/@"):
        return None
    return at + 1


def configure_logging(settings: Settings) -> None:
    proxy = urlsplit(settings.telegram_proxy_url)
    handler = logging.StreamHandler()
    handler.addFilter(
        SecretFilter(
            [
                settings.bot_token,
                settings.postgres_password,
                settings.telegram_proxy_url,
                settings.llm_api_key,
                proxy.password or "",
                unquote(proxy.password or ""),
            ]
        )
    )
    handler.setFormatter(logging.Formatter("%(asctime)s %(levelname)s %(name)s: %(message)s"))
    logging.basicConfig(level=settings.log_level, handlers=[handler], force=True)
