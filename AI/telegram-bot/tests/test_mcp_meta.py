"""Проверка доверенного meta для add_reminder."""

from __future__ import annotations

import hashlib

from mcp_server.meta import expected_trust_token, is_trusted


def test_trust_token_derived_from_bot_token(monkeypatch):
    monkeypatch.delenv("MCP_TRUST_TOKEN", raising=False)
    monkeypatch.setenv("BOT_TOKEN", "123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijk")
    expected = hashlib.sha256(
        b"mcp-trust:123456789:ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijk"
    ).hexdigest()
    assert expected_trust_token() == expected
    assert is_trusted({"trust_token": expected, "owner_chat_id": 1})
    assert not is_trusted({"trust_token": "wrong", "owner_chat_id": 1})
    assert not is_trusted({"owner_chat_id": 1})


def test_explicit_mcp_trust_token_wins(monkeypatch):
    monkeypatch.setenv("MCP_TRUST_TOKEN", "shared-secret")
    monkeypatch.setenv("BOT_TOKEN", "ignored-for-trust")
    assert expected_trust_token() == "shared-secret"
    assert is_trusted({"trust_token": "shared-secret"})
    assert not is_trusted({"trust_token": "ignored-for-trust"})
