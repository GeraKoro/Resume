"""IANA-зоны и форматирование времени напоминания."""

from datetime import UTC, datetime

from app.timezones import format_in_timezone, format_local_now, validate_timezone


def test_validate_and_format_local():
    assert validate_timezone("Europe/Moscow") == "Europe/Moscow"
    stamp = format_local_now(
        "Europe/Moscow",
        now=datetime(2026, 10, 31, 9, 33, tzinfo=UTC),
    )
    assert stamp.startswith("31.10.2026")


def test_format_in_timezone_from_utc_iso():
    got = format_in_timezone("2026-10-31T20:58:00+00:00", "Europe/Moscow")
    assert got.startswith("2026-10-31T23:58:00")
    assert "+03:00" in got or got.endswith("+0300")
