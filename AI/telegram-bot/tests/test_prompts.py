import pytest

from app.dialog import DialogMessage
from app.prompts import (
    BRIEF_INSTRUCTION,
    QUIZ_INSTRUCTION,
    STUDY_INSTRUCTION,
    TRANSLATE_EXAMPLES,
    TRANSLATE_INSTRUCTION,
    PromptError,
    build_messages,
)


def test_modes_have_different_tasks():
    # Assert
    assert "программирован" in STUDY_INSTRUCTION
    assert "только перевод" in TRANSLATE_INSTRUCTION
    assert "ровно три пункта" in BRIEF_INSTRUCTION
    assert "ровно один вопрос" in QUIZ_INSTRUCTION
    assert len({STUDY_INSTRUCTION, TRANSLATE_INSTRUCTION, BRIEF_INSTRUCTION, QUIZ_INSTRUCTION}) == 4


def test_translate_examples_cover_a_borderline_request():
    # Arrange
    pairs = list(zip(TRANSLATE_EXAMPLES[0::2], TRANSLATE_EXAMPLES[1::2], strict=True))
    # Assert
    assert all(question.role == "user" and answer.role == "assistant" for question, answer in pairs)
    borderline = next(answer for question, answer in pairs if "объясни" in question.content)
    assert borderline.content == "A list stores elements in order."
    missing_language = next(
        answer for question, answer in pairs if question.content == "Переведи: Доброе утро."
    )
    assert missing_language.content == "На какой язык перевести?"


def test_request_keeps_instruction_examples_and_history_in_order():
    # Arrange
    history = [
        DialogMessage("user", "Переведи на английский: Кот спит."),
        DialogMessage("assistant", "The cat is sleeping."),
        DialogMessage("user", "Переведи на английский: Пёс бежит."),
    ]
    # Act
    messages = build_messages("translate", history)
    # Assert
    assert messages[0] == {"role": "system", "content": TRANSLATE_INSTRUCTION}
    example_count = len(TRANSLATE_EXAMPLES)
    assert messages[1 : 1 + example_count] == [
        {"role": item.role, "content": item.content} for item in TRANSLATE_EXAMPLES
    ]
    assert messages[-1]["content"] == "Переведи на английский: Пёс бежит."


def test_study_and_brief_send_history_right_after_instruction():
    # Arrange
    history = [DialogMessage("user", "Что такое список?")]
    # Act
    study = build_messages("study", history)
    brief = build_messages("brief", [DialogMessage("user", "Длинный текст для конспекта.")])
    quiz = build_messages("quiz", [DialogMessage("user", "Квиз по циклам")])
    # Assert
    assert [item["role"] for item in study] == ["system", "user"]
    assert study[0]["content"] == STUDY_INSTRUCTION
    assert [item["role"] for item in brief] == ["system", "user"]
    assert brief[0]["content"] == BRIEF_INSTRUCTION
    assert [item["role"] for item in quiz] == ["system", "user"]
    assert quiz[0]["content"] == QUIZ_INSTRUCTION


def test_unknown_mode_is_rejected():
    # Act / Assert
    with pytest.raises(PromptError, match="режим"):
        build_messages("chat", [])
