import asyncio
import getpass
import hashlib
import os
import secrets
import subprocess
import sys
from collections.abc import Callable, Mapping
from dataclasses import asdict
from pathlib import Path

from dotenv import dotenv_values, set_key

from app.config import ConfigError, Settings
from app.db import create_pool
from scripts.common import CommandError, private_write, run_command


def _ask_required(ask: Callable[[str], str], prompt: str) -> str:
    """Спрашивает значение, пока пользователь не введёт непустую строку."""
    while True:
        value = ask(prompt).strip()
        if value:
            return value
        print("Значение обязательно, пустой ввод не подходит.", flush=True)


def prepare_env(
    root: Path,
    *,
    cloud: bool,
    ask: Callable[[str], str] = getpass.getpass,
    environ: Mapping[str, str] | None = None,
) -> Path:
    path = root / (".env.cloud" if cloud else ".env")
    existing = dotenv_values(path, interpolate=False)
    process = dict(os.environ if environ is None else environ)
    values = {**existing, **process}
    updates = {}
    defaults = {
        "POSTGRES_HOST": "db" if cloud else "127.0.0.1",
        "POSTGRES_PORT": "5432",
        "POSTGRES_DB": "bot",
        "POSTGRES_USER": "bot",
        "LOG_LEVEL": "INFO",
        "HEALTH_PORT": "8080",
    }
    for key, default in defaults.items():
        if not values.get(key):
            updates[key] = default
    if not values.get("BOT_TOKEN"):
        updates["BOT_TOKEN"] = _ask_required(ask, "Токен бота из BotFather (ввод скрыт): ")
    if not values.get("POSTGRES_PASSWORD"):
        updates["POSTGRES_PASSWORD"] = secrets.token_urlsafe(24)
    if "TELEGRAM_PROXY_URL" not in values or (cloud and not values.get("TELEGRAM_PROXY_URL")):
        updates["TELEGRAM_PROXY_URL"] = ask(
            "URL HTTP/SOCKS5 прокси (ввод скрыт; локально Enter — без прокси): "
        ).strip()
    merged = {**values, **updates}
    if cloud and not merged.get("TELEGRAM_PROXY_URL"):
        raise ConfigError("Для облака необходимо указать внешний прокси Telegram.")
    if not merged.get("LLM_BASE_URL"):
        entered = ask(
            "Базовый URL API Groq (ввод скрыт; Enter — https://api.groq.com/openai/v1): "
        ).strip()
        updates["LLM_BASE_URL"] = entered or "https://api.groq.com/openai/v1"
    if not merged.get("LLM_API_KEY"):
        updates["LLM_API_KEY"] = _ask_required(ask, "Ключ API Groq (ввод скрыт): ")
    if not merged.get("LLM_MODEL"):
        entered = ask("Имя модели (ввод скрыт; Enter — openai/gpt-oss-20b): ").strip()
        updates["LLM_MODEL"] = entered or "openai/gpt-oss-20b"
    for key, default in {
        "LLM_TIMEOUT_SECONDS": "30",
        "HISTORY_MAX_MESSAGES": "10",
        "HISTORY_MAX_CHARS": "4000",
    }.items():
        if not merged.get(key):
            updates[key] = default
    merged = {**values, **updates}
    Settings.load(path, environ=merged)
    if not path.exists():
        private_write(path, "# Секретные настройки. Не добавляйте этот файл в Git.\n")
    for key, value in updates.items():
        set_key(path, key, value, quote_mode="always", encoding="utf-8")
    path.chmod(0o600)
    return path


def compose_command(root: Path) -> list[str]:
    # Compose требует ASCII-имя проекта, даже если каталог назван по-русски.
    project = "itmo-local-" + hashlib.sha256(str(root.resolve()).encode("utf-8")).hexdigest()[:10]
    return [
        "docker",
        "compose",
        "--project-name",
        project,
        "--project-directory",
        str(root),
        "--env-file",
        str(root / ".env"),
        "-f",
        str(root / "compose.yaml"),
        "-f",
        str(root / "compose.local.yaml"),
    ]


async def check_database(settings: Settings) -> None:
    pool = await create_pool(settings)
    await pool.close()


def local_main(root: Path, action: str, setup_only: bool) -> int:
    if action == "health":
        return subprocess.call([sys.executable, "-m", "app.healthcheck"], cwd=root)
    run_command(["docker", "info"], label="Docker недоступен: запустите Docker Desktop/Engine")
    run_command(["docker", "compose", "version"], label="Проверка Docker Compose")
    if action == "up":
        path = prepare_env(root, cloud=False)
        settings = Settings.load(path)
        if settings.postgres_host not in {"127.0.0.1", "localhost"}:
            raise ConfigError("Для локального запуска POSTGRES_HOST должен быть 127.0.0.1.")
        print("Запускаем PostgreSQL и ждём готовности…", flush=True)
        run_command(
            [*compose_command(root), "up", "-d", "--wait", "--wait-timeout", "90", "db"],
            label="Запуск PostgreSQL; проверьте свободный порт и доступ к Docker Hub",
            timeout=600,
            cwd=root,
            # Один раз разбираем настройки Python-парсером; Compose получает те же значения.
            env={
                **os.environ,
                **{key.upper(): str(value) for key, value in asdict(settings).items()},
            },
        )
        try:
            asyncio.run(check_database(settings))
        except Exception:
            raise CommandError(
                "БД не прошла SELECT 1. Проверьте адрес и пароль; изменение env-файла "
                "не меняет пароль уже созданной роли PostgreSQL."
            ) from None
        print("Окружение готово, PostgreSQL отвечает на SELECT 1.", flush=True)
        if setup_only:
            print("Выберите Python из .venv в IDE и запустите модуль app из корня проекта.")
            return 0
        return subprocess.call([sys.executable, "-m", "app"], cwd=root)
    if not (root / ".env").exists():
        raise CommandError("Сначала выполните первоначальную настройку: start-скрипт --setup-only.")
    command = {"status": ["ps"], "logs": ["logs", "--tail", "100", "db"], "stop": ["stop", "db"]}[
        action
    ]
    print(run_command([*compose_command(root), *command], label="Управление локальной БД"))
    return 0
