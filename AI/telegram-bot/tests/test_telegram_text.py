import html

from app.telegram_text import reply_messages


def _python_body(part: str) -> str | None:
    prefix = '<pre><code class="language-python">'
    suffix = "</code></pre>"
    if part.startswith(prefix) and part.endswith(suffix):
        return part[len(prefix) : -len(suffix)]
    return None


def test_plain_reply_stays_unformatted():
    assert reply_messages("Просто текст") == [("Просто текст", None)]


def test_python_fence_becomes_html_code_block():
    source = "Смотрите:\n```python\nfor i in range(5):\n    print(i)\n```\n"
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert text.startswith("Смотрите:\n")
    assert "```" not in text
    assert (
        '<pre><code class="language-python">for i in range(5):\n    print(i)</code></pre>' in text
    )


def test_inline_code_and_bold_escape_html():
    text, mode = reply_messages("Цикл **for** проверяет `i < n` и a & b.")[0]
    assert mode == "HTML"
    assert "<b>for</b>" in text
    assert "<code>i &lt; n</code>" in text
    assert "a &amp; b." in text


def test_markdown_table_becomes_labeled_text():
    table = "| Тип | Пример |\n|-----|--------|\n| **for** | `i < n` |\n"
    text, mode = reply_messages("Таблица:\n" + table)[0]
    assert mode == "HTML"
    assert "<pre>" not in text
    assert "**" not in text
    assert "|" not in text
    assert "<b>Тип:</b> <b>for</b>" in text
    assert "<b>Пример:</b> <code>i &lt; n</code>" in text


def test_br_inside_backticks_becomes_python_block():
    source = "**Пример кода:** `python<br>table = [<br>    [1, 2]<br>]`"
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "<br>" not in text
    assert "`" not in text
    assert "<b>Пример кода:</b>" in text
    assert '<pre><code class="language-python">table = [\n    [1, 2]\n]</code></pre>' in text


def test_bold_labels_lists_quotes_and_single_table_row():
    source = "\n".join(
        [
            "**Способ:** **pandas.DataFrame**",
            "- **for** — перебирает элементы",
            "| **numpy.ndarray** | Массивы | Расчёты |",
            "> Важно: else относится к циклу",
        ]
    )
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "**" not in text
    assert "|" not in text
    assert "<b>Способ:</b> <b>pandas.DataFrame</b>" in text
    assert "• <b>for</b> — перебирает элементы" in text
    assert "<b>numpy.ndarray</b>" in text
    assert "<blockquote>Важно: else относится к циклу</blockquote>" in text


def test_wrapped_table_with_code_inside_a_cell_becomes_labeled_text():
    source = """
| Способ | Что это | Пример кода |
|---|---|---|
| список списков (list[list]) | Простейшая таблица: каждая строка
– список значений. | Маленькие наборы. |
```python
table = [
    [1, 2]
]
```
|
| **pandas.DataFrame** | Таблица с метаданными. |
```python
import pandas as pd
```
|
""".strip()
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "|" not in text
    assert "**" not in text
    assert "---" not in text
    assert "<b>Способ:</b> список списков (list[list])" in text
    assert "<b>Что это:</b> Простейшая таблица: каждая строка – список значений." in text
    assert '<pre><code class="language-python">table = [\n    [1, 2]\n]</code></pre>' in text
    assert "<b>Способ:</b> <b>pandas.DataFrame</b>" in text
    assert '<pre><code class="language-python">import pandas as pd</code></pre>' in text


def test_loose_table_rows_with_a_code_fence_keep_no_pipes():
    source = """
| **while** | while <условие>: | Выполняет тело, пока условие
истинно. |
```python
i = 0
```
|
| **break** | break | Прерывает цикл. |
""".strip()
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "|" not in text
    assert "**" not in text
    assert "<b>while</b>" in text
    assert "while &lt;условие&gt;:" in text
    assert "Выполняет тело, пока условие истинно." in text
    assert '<pre><code class="language-python">i = 0</code></pre>' in text
    assert "<b>break</b>" in text
    assert "Прерывает цикл." in text


def test_plain_example_is_not_turned_into_code():
    assert reply_messages("Краткий пример: for i in range(3): print(i)") == [
        ("Краткий пример: for i in range(3): print(i)", None)
    ]


def test_squashed_newlines_in_a_sample_become_line_breaks():
    assert reply_messages("Пример: i = 0\\nwhile i < 3:\\n    print(i)") == [
        ("Пример: i = 0\nwhile i < 3:\n    print(i)", None)
    ]
    assert reply_messages("В строке пишут символ \\n.") == [("В строке пишут символ \\n.", None)]


def test_cycle_notes_keep_bold_words_and_bullets():
    source = "- **for** — перебирает элементы.\n- **while** — ждёт условие.\n"
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "**" not in text
    assert "• <b>for</b> — перебирает элементы." in text
    assert "• <b>while</b> — ждёт условие." in text


def test_labeled_answer_formats_code_and_quote():
    source = (
        "Тип цикла: **while**\n"
        "Синтаксис: while <условие>:\n"
        "Что делает: Выполняет тело, пока условие истинно\n"
        "Пример: i = 0\\nwhile i < 3:\\n    print(i)\n"
        "\n"
        "> Важно: else относится к циклу.\n"
    )
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "**" not in text
    assert "\\n" not in text
    assert "Тип цикла: <b>while</b>" in text
    assert "Синтаксис: while &lt;условие&gt;:" in text
    assert "Что делает: Выполняет тело, пока условие истинно" in text
    assert "Пример: i = 0\nwhile i &lt; 3:\n    print(i)" in text
    assert "<blockquote>Важно: else относится к циклу.</blockquote>" in text


def test_long_code_block_is_split_without_losing_code():
    body = "\n".join(f"line {number} {'x' * 40}" for number in range(80))
    messages = reply_messages(f"```python\n{body}\n```", limit=500)
    assert messages
    assert all(mode == "HTML" and len(part) <= 500 for part, mode in messages)
    pieces = []
    for part, _mode in messages:
        code = _python_body(part)
        assert code is not None
        pieces.append(html.unescape(code))
    assert "".join(pieces) == body


def test_bold_markers_become_telegram_bold_and_stay_inside_code():
    text, mode = reply_messages("Тип: **for** и **слово `код` ещё**")[0]
    assert mode == "HTML"
    assert "**" not in text
    assert "<b>for</b>" in text
    assert "<b>слово <code>код</code> ещё</b>" in text
    fenced, fenced_mode = reply_messages("```python\ndef f(**kwargs):\n    pass\n```")[0]
    assert fenced_mode == "HTML"
    assert '<pre><code class="language-python">def f(**kwargs):\n    pass</code></pre>' in fenced
    assert reply_messages("`**word**`")[0] == ("<b>word</b>\n", "HTML")


def test_named_code_block_with_russian_strings_stays_python():
    source = (
        "Способ: **pandas.DataFrame**\n"
        "Мини-пример:\n"
        "```python\n"
        "table = [\n"
        '    ["Имя", "Возраст"],\n'
        "]\n"
        "```\n"
        "```python\n"
        "python import numpy as np arr = np.array([[1, 2, 3],\n"
        "[4, 5, 6]])\n"
        "```\n"
    )
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "**" not in text
    assert "Способ: <b>pandas.DataFrame</b>" in text
    assert (
        '<pre><code class="language-python">table = [\n    ["Имя", "Возраст"],\n]</code></pre>'
        in text
    )
    assert (
        '<pre><code class="language-python">python import numpy as np arr = np.array([[1, 2, 3],\n'
        "[4, 5, 6]])</code></pre>" in text
    )


def test_markdown_headings_become_bold_and_rules_disappear():
    source = "### Как работают циклы в C++\n\nТекст.\n\n---\n\n#### 1. for-цикл\n"
    text, mode = reply_messages(source)[0]
    assert mode == "HTML"
    assert "#" not in text
    assert "---" not in text
    assert "<b>Как работают циклы в C++</b>" in text
    assert "<b>1. for-цикл</b>" in text
    assert "Текст." in text


def test_heading_moves_with_its_paragraph_to_the_next_message():
    intro = "Первый абзац про циклы.\n\n"
    heading = "#### 8. Потенциальные ошибки\n"
    body = "Ошибка: выход за пределы массива при обходе вектора.\n"
    source = "**Тема**\n\n" + intro + heading + "\n" + body
    limit = 120
    messages = reply_messages(source, limit=limit)
    assert all(mode == "HTML" and len(part) <= limit for part, mode in messages)
    owning = [part for part, _mode in messages if "Потенциальные ошибки" in part]
    assert owning == [part for part, _mode in messages if "Ошибка:" in part]
    assert len(owning) == 1
    assert owning[0].index("Потенциальные ошибки") < owning[0].index("Ошибка:")


def test_model_html_tags_are_formatted_once():
    text, mode = reply_messages("Тип цикла: <code>for</code>\n**Синтаксис:** `while`")[0]
    assert mode == "HTML"
    assert "&lt;code&gt;" not in text
    assert "<code>for</code>" in text
    assert "<b>Синтаксис:</b>" in text
    assert "<code>while</code>" in text
    assert text.count("<code>") == 2
