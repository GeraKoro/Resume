"""Оценка маршрутизации на docs/evaluation/lab2-routing.json.

Не создаёт напоминания и не ходит в Open-Meteo: смотрит только первый ход модели.
Запуск из корня: .venv/bin/python -m scripts.eval_routing
"""

from __future__ import annotations

import argparse
import asyncio
import json
from dataclasses import asdict, dataclass
from datetime import datetime
from pathlib import Path
from typing import Any

from app.agent import AGENT_HINT, _looks_like_question
from app.config import Settings
from app.dialog import DialogMessage
from app.llm import GroqChatClient, LlmError
from app.prompts import build_messages
from app.tool_validate import ToolValidationError, validate_tool_arguments

ROOT = Path(__file__).resolve().parents[1]

TOOLS: list[dict[str, Any]] = [
    {
        "type": "function",
        "function": {
            "name": "get_weather",
            "description": (
                "Текущая погода в городе. Чтение. Не выдумывает данные. "
                "В city передавай формулировку пользователя целиком: если названы город и "
                "страна — оба через запятую, без сокращений."
            ),
            "parameters": {
                "type": "object",
                "properties": {
                    "city": {
                        "type": "string",
                        "description": (
                            "Город как у пользователя; при названной стране — «город, страна»."
                        ),
                    }
                },
                "required": ["city"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "get_schedule",
            "description": "Занятия на дату YYYY-MM-DD из учебного расписания. Чтение.",
            "parameters": {
                "type": "object",
                "properties": {
                    "date": {
                        "type": "string",
                        "description": "Дата занятия в формате YYYY-MM-DD.",
                    }
                },
                "required": ["date"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "add_reminder",
            "description": (
                "Создать напоминание. Изменение, нужно подтверждение пользователя. "
                "Аргументы: text и remind_at (ISO 8601 с зоной) в будущем."
            ),
            "parameters": {
                "type": "object",
                "properties": {
                    "text": {
                        "type": "string",
                        "description": "Краткий текст напоминания без слова «напомни».",
                    },
                    "remind_at": {
                        "type": "string",
                        "description": "Момент напоминания, ISO 8601 со смещением зоны.",
                    },
                },
                "required": ["text", "remind_at"],
            },
        },
    },
]


@dataclass
class CaseResult:
    id: str
    query: str
    expected_action: str
    actual_action: str
    expected_arguments: dict[str, Any]
    actual_arguments: dict[str, Any]
    action_ok: bool
    arguments_ok: bool | None
    note: str


def _normalize_city(value: str) -> str:
    return value.strip().casefold().replace("ё", "е")


def _normalize_text(value: str) -> str:
    return " ".join(value.strip().casefold().replace("ё", "е").split())


def _args_match(expected_action: str, expected: dict[str, Any], actual: dict[str, Any]) -> bool:
    if expected_action == "get_weather":
        got = actual.get("city")
        want = expected.get("city")
        if not isinstance(got, str) or not isinstance(want, str):
            return False
        got_n = _normalize_city(got)
        want_n = _normalize_city(want)
        return got_n == want_n or want_n in got_n
    if expected_action == "get_schedule":
        return actual.get("date") == expected.get("date")
    if expected_action == "add_reminder":
        text_ok = _normalize_text(str(actual.get("text", ""))) == _normalize_text(
            str(expected.get("text", ""))
        )
        when_got = str(actual.get("remind_at", ""))
        when_want = str(expected.get("remind_at", ""))
        try:
            time_ok = datetime.fromisoformat(when_got) == datetime.fromisoformat(when_want)
        except ValueError:
            time_ok = when_got == when_want
        return text_ok and time_ok
    return True


def _classify_text(text: str) -> str:
    if _looks_like_question(text):
        return "clarify"
    lowered = text.lower().replace("ё", "е")
    # Модель часто ставит «?» не в самом конце фразы.
    if "?" in text or lowered.startswith("когда") or "уточн" in lowered:
        return "clarify"
    return "respond"


def _build_messages(query: str, *, now: str, timezone: str) -> list[dict[str, Any]]:
    history = [DialogMessage("user", query)]
    messages: list[dict[str, Any]] = [dict(item) for item in build_messages("study", history)]
    clock = (
        f"Текущие дата и время для этого запроса: {now} (зона {timezone}). "
        f"Относительные слова вроде «сегодня», «завтра», «через два часа» "
        f"считай от этого момента. Часовой пояс пользователя: {timezone}."
    )
    if messages and messages[0].get("role") == "system":
        content = f"{messages[0]['content']}\n\n{AGENT_HINT}\n\n{clock}".strip()
        messages[0]["content"] = content
    else:
        messages.insert(0, {"role": "system", "content": f"{AGENT_HINT}\n\n{clock}"})
    return messages


async def _run_case(
    client: GroqChatClient,
    case: dict[str, Any],
    *,
    now: str,
    timezone: str,
    temperature: float,
) -> CaseResult:
    expected_action = str(case["expected_action"])
    expected_arguments = dict(case.get("expected_arguments") or {})
    query = str(case["query"])
    note = ""
    actual_action = "respond"
    actual_arguments: dict[str, Any] = {}
    try:
        turn = await client.complete_turn(
            temperature=temperature,
            messages=_build_messages(query, now=now, timezone=timezone),
            tools=TOOLS,
        )
    except LlmError as exc:
        needs_args = expected_action not in {"respond", "clarify"}
        return CaseResult(
            id=str(case["id"]),
            query=query,
            expected_action=expected_action,
            actual_action="error",
            expected_arguments=expected_arguments,
            actual_arguments={},
            action_ok=False,
            arguments_ok=False if needs_args else None,
            note=exc.user_message,
        )

    if turn.tool_calls:
        call = turn.tool_calls[0]
        actual_action = call.name
        try:
            actual_arguments = validate_tool_arguments(call.name, call.arguments)
        except ToolValidationError as exc:
            actual_arguments = dict(call.arguments)
            note = f"аргументы не прошли валидацию: {exc}"
    else:
        text = turn.content.strip()
        actual_action = _classify_text(text)
        note = "пустой ответ модели" if not text else text[:160]

    action_ok = actual_action == expected_action
    arguments_ok: bool | None
    if expected_action in {"respond", "clarify"}:
        arguments_ok = None
    else:
        arguments_ok = action_ok and _args_match(
            expected_action, expected_arguments, actual_arguments
        )
        if action_ok and not arguments_ok and not note:
            note = "действие верное, аргументы не совпали"

    return CaseResult(
        id=str(case["id"]),
        query=query,
        expected_action=expected_action,
        actual_action=actual_action,
        expected_arguments=expected_arguments,
        actual_arguments=actual_arguments,
        action_ok=action_ok,
        arguments_ok=arguments_ok,
        note=note,
    )


def _summary(rows: list[CaseResult]) -> dict[str, Any]:
    action_ok = sum(1 for row in rows if row.action_ok)
    tool_rows = [row for row in rows if row.expected_action not in {"respond", "clarify"}]
    args_ok = sum(1 for row in tool_rows if row.arguments_ok)
    return {
        "cases": len(rows),
        "routing_accuracy": f"{action_ok}/{len(rows)}",
        "argument_accuracy": f"{args_ok}/{len(tool_rows)}",
        "routing_ok": action_ok >= 10 if len(rows) == 12 else action_ok == len(rows),
        "arguments_ok": args_ok >= 8 if len(tool_rows) == 9 else args_ok == len(tool_rows),
    }


async def _amain(args: argparse.Namespace, payload: dict[str, Any]) -> dict[str, Any]:
    settings = Settings.load(args.env_file)
    context = payload["evaluation_context"]
    now = str(context["now"])
    timezone = str(context["timezone"])
    cases = list(payload["cases"])
    if args.cases:
        wanted = {item.strip() for item in args.cases.split(",") if item.strip()}
        cases = [case for case in cases if case["id"] in wanted]
    client = GroqChatClient(settings)
    rows: list[CaseResult] = []
    try:
        for case in cases:
            print(f"→ {case['id']} …", flush=True)
            row = await _run_case(
                client,
                case,
                now=now,
                timezone=timezone,
                temperature=args.temperature,
            )
            mark = "OK" if row.action_ok and row.arguments_ok is not False else "FAIL"
            print(
                f"  [{mark}] expected={row.expected_action} actual={row.actual_action} "
                f"args_ok={row.arguments_ok}",
                flush=True,
            )
            rows.append(row)
            await asyncio.sleep(args.delay)
    finally:
        await client.close()

    return {
        "version": payload.get("version"),
        "model": settings.llm_model,
        "temperature": args.temperature,
        "evaluation_context": context,
        "label": args.label,
        "summary": _summary(rows),
        "results": [asdict(row) for row in rows],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Оценка маршрутизации lab2")
    parser.add_argument(
        "--dataset",
        default=str(ROOT / "docs" / "evaluation" / "lab2-routing.json"),
    )
    parser.add_argument(
        "--out",
        default=str(ROOT / "docs" / "evaluation" / "lab2-routing-results.json"),
    )
    parser.add_argument("--env-file", default=str(ROOT / ".env"))
    parser.add_argument("--temperature", type=float, default=0.0)
    parser.add_argument("--delay", type=float, default=1.0)
    parser.add_argument("--cases", default="")
    parser.add_argument("--label", default="baseline")
    args = parser.parse_args()
    payload = json.loads(Path(args.dataset).read_text(encoding="utf-8"))
    report = asyncio.run(_amain(args, payload))
    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["summary"], ensure_ascii=False, indent=2))
    print(f"Сохранено: {out}")


if __name__ == "__main__":
    main()
