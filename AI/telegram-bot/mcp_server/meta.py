"""Чтение доверенного _meta из MCP Context без аргументов модели."""

from __future__ import annotations

import hashlib
import os
import secrets
from typing import Any


def meta_dict(meta: object | None) -> dict[str, Any]:
    if meta is None:
        return {}
    if isinstance(meta, dict):
        return dict(meta)
    extra = getattr(meta, "model_extra", None)
    if isinstance(extra, dict) and extra:
        return dict(extra)
    data: dict[str, Any] = {}
    for key in ("owner_chat_id", "action_key", "timezone", "trust_token"):
        if hasattr(meta, key):
            value = getattr(meta, key)
            if value is not None:
                data[key] = value
    dump = getattr(meta, "model_dump", None)
    if callable(dump):
        try:
            dumped = dump(exclude_none=True)
            if isinstance(dumped, dict):
                data.update({k: v for k, v in dumped.items() if not str(k).startswith("_")})
        except TypeError:
            pass
    return data


def owner_chat_id(meta: object | None) -> int | None:
    raw = meta_dict(meta).get("owner_chat_id")
    try:
        return int(raw) if raw is not None else None
    except (TypeError, ValueError):
        return None


def action_key(meta: object | None) -> str:
    return str(meta_dict(meta).get("action_key") or "")


def timezone_name(meta: object | None, default: str) -> str:
    value = meta_dict(meta).get("timezone")
    return str(value or default)


def expected_trust_token() -> str:
    """Секрет доверия: MCP_TRUST_TOKEN или производная от BOT_TOKEN."""
    explicit = (os.environ.get("MCP_TRUST_TOKEN") or "").strip()
    if explicit:
        return explicit
    bot = (os.environ.get("BOT_TOKEN") or "").strip()
    if bot:
        return hashlib.sha256(f"mcp-trust:{bot}".encode()).hexdigest()
    return ""


def is_trusted(meta: object | None, expected: str | None = None) -> bool:
    """Проверяет, что meta пришёл от клиента, знающего секрет доверия."""
    token = expected if expected is not None else expected_trust_token()
    got = str(meta_dict(meta).get("trust_token") or "")
    if not token or not got:
        return False
    return secrets.compare_digest(got, token)
