"""Единственное место чтения и проверки настроек приложения."""

import hashlib
import os
from collections.abc import Mapping
from dataclasses import dataclass, field
from pathlib import Path
from urllib.parse import urlsplit

from aiogram.utils.token import TokenValidationError, validate_token
from dotenv import dotenv_values


class ConfigError(ValueError):
    """Ошибка настройки без секретных значений в сообщении."""


@dataclass(frozen=True)
class Settings:
    bot_token: str = field(repr=False)
    postgres_password: str = field(repr=False)
    telegram_proxy_url: str = field(default="", repr=False)
    postgres_host: str = "127.0.0.1"
    postgres_port: int = 5432
    postgres_db: str = "bot"
    postgres_user: str = "bot"
    log_level: str = "INFO"
    health_port: int = 8080
    llm_api_key: str = field(default="", repr=False)
    llm_base_url: str = "https://api.groq.com/openai/v1"
    llm_model: str = "openai/gpt-oss-20b"
    llm_timeout_seconds: float = 30.0
    history_max_messages: int = 10
    history_max_chars: int = 4000
    schedule_path: str = "data/schedule.json"
    mcp_server_command: str = ""
    weather_http_timeout_seconds: float = 10.0
    default_app_timezone: str = "Europe/Moscow"
    max_mcp_calls_per_message: int = 2
    mcp_trust_token: str = field(default="", repr=False)

    @classmethod
    def load(
        cls, env_file: Path | str = ".env", *, environ: Mapping[str, str] | None = None
    ) -> "Settings":
        values = {
            **dotenv_values(env_file, interpolate=False),
            **(os.environ if environ is None else environ),
        }

        def value(key: str, default: str = "") -> str:
            return values.get(key) or default

        def port(key: str, default: str) -> int:
            try:
                result = int(value(key, default))
                if not 1 <= result <= 65535:
                    raise ValueError
                return result
            except ValueError:
                raise ConfigError(f"{key}: нужен номер порта от 1 до 65535.") from None

        token = value("BOT_TOKEN")
        try:
            validate_token(token)
        except TokenValidationError:
            raise ConfigError("BOT_TOKEN: укажите токен, полученный у BotFather.") from None
        password = value("POSTGRES_PASSWORD")
        if not password:
            raise ConfigError("POSTGRES_PASSWORD: пароль базы данных не задан.")
        proxy = value("TELEGRAM_PROXY_URL")
        if proxy:
            try:
                parsed = urlsplit(proxy)
                if (
                    parsed.scheme not in {"http", "socks5"}
                    or not parsed.hostname
                    or not parsed.port
                    or parsed.path not in {"", "/"}
                    or parsed.query
                    or parsed.fragment
                ):
                    raise ValueError
            except ValueError:
                raise ConfigError(
                    "TELEGRAM_PROXY_URL: нужен http://host:port или socks5://host:port; "
                    "при необходимости добавьте user:password@."
                ) from None
        level = value("LOG_LEVEL", "INFO").upper()
        if level not in {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"}:
            raise ConfigError("LOG_LEVEL: используйте DEBUG, INFO, WARNING, ERROR или CRITICAL.")
        postgres_port = port("POSTGRES_PORT", "5432")
        health_port = port("HEALTH_PORT", "8080")
        api_key = value("LLM_API_KEY").strip()
        if not api_key:
            raise ConfigError("LLM_API_KEY: укажите ключ доступа к API языковой модели.")
        base_url = value("LLM_BASE_URL").strip().rstrip("/")
        if not base_url:
            raise ConfigError(
                "LLM_BASE_URL: укажите адрес API. Для Groq это https://api.groq.com/openai/v1."
            )
        try:
            parsed = urlsplit(base_url)
            if (
                parsed.scheme not in {"http", "https"}
                or not parsed.hostname
                or parsed.username
                or parsed.password
            ):
                raise ValueError
        except ValueError:
            raise ConfigError(
                "LLM_BASE_URL: нужен адрес http:// или https:// без логина и пароля."
            ) from None
        model = value("LLM_MODEL").strip()
        if not model or any(char.isspace() for char in model):
            raise ConfigError("LLM_MODEL: укажите имя модели без пробелов.")

        def positive_int(key: str, default: str) -> int:
            try:
                number = int(value(key, default))
                if number < 1:
                    raise ValueError
                return number
            except ValueError:
                raise ConfigError(f"{key}: нужно целое число больше нуля.") from None

        try:
            timeout = float(value("LLM_TIMEOUT_SECONDS", "30"))
            if timeout <= 0:
                raise ValueError
        except ValueError:
            raise ConfigError("LLM_TIMEOUT_SECONDS: нужно положительное число секунд.") from None
        try:
            weather_timeout = float(value("WEATHER_HTTP_TIMEOUT_SECONDS", "10"))
            if weather_timeout <= 0:
                raise ValueError
        except ValueError:
            raise ConfigError(
                "WEATHER_HTTP_TIMEOUT_SECONDS: нужно положительное число секунд."
            ) from None
        schedule_path = value("SCHEDULE_PATH", "data/schedule.json").strip() or "data/schedule.json"
        app_tz = value("DEFAULT_APP_TIMEZONE", "Europe/Moscow").strip() or "Europe/Moscow"
        try:
            from zoneinfo import ZoneInfo

            ZoneInfo(app_tz)
        except Exception as exc:
            raise ConfigError(
                "DEFAULT_APP_TIMEZONE: укажите имя зоны из базы IANA, например Europe/Moscow."
            ) from exc
        mcp_command = value("MCP_SERVER_COMMAND").strip()
        trust = value("MCP_TRUST_TOKEN").strip()
        if not trust:
            trust = hashlib.sha256(f"mcp-trust:{token}".encode()).hexdigest()
        return cls(
            bot_token=token,
            postgres_password=password,
            telegram_proxy_url=proxy,
            postgres_host=value("POSTGRES_HOST", "127.0.0.1"),
            postgres_port=postgres_port,
            postgres_db=value("POSTGRES_DB", "bot"),
            postgres_user=value("POSTGRES_USER", "bot"),
            log_level=level,
            health_port=health_port,
            llm_api_key=api_key,
            llm_base_url=base_url,
            llm_model=model,
            llm_timeout_seconds=timeout,
            history_max_messages=positive_int("HISTORY_MAX_MESSAGES", "10"),
            history_max_chars=positive_int("HISTORY_MAX_CHARS", "4000"),
            schedule_path=schedule_path,
            mcp_server_command=mcp_command,
            weather_http_timeout_seconds=weather_timeout,
            default_app_timezone=app_tz,
            max_mcp_calls_per_message=positive_int("MAX_MCP_CALLS_PER_MESSAGE", "2"),
            mcp_trust_token=trust,
        )
