"""Разбиение ответа и простое оформление для Telegram."""

from html import escape

TELEGRAM_TEXT_LIMIT = 4096
_LANGUAGE_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+#-")


def split_telegram_text(text: str, limit: int = TELEGRAM_TEXT_LIMIT) -> list[str]:
    if limit < 1:
        raise ValueError("limit must be positive")
    parts: list[str] = []
    start = 0
    while start < len(text):
        end = min(start + limit, len(text))
        if end < len(text):
            newline = text.rfind("\n", start, end)
            if newline > start:
                end = newline + 1
        part = text[start:end]
        if not part:
            break
        parts.append(part)
        start = end
    return parts


def reply_messages(text: str, limit: int = TELEGRAM_TEXT_LIMIT) -> list[tuple[str, str | None]]:
    """Готовит части ответа. Разметка уходит как HTML, обычный текст — как есть."""
    if limit < 1:
        raise ValueError("limit must be positive")
    prepared = _normalize(text)
    html = _html(prepared)
    if html is None:
        return [(part, None) for part in split_telegram_text(prepared, limit)]
    return [(part, "HTML") for part in _split_html(html, limit)]


def _normalize(text: str) -> str:
    text = _replace_breaks(text)
    text = _replace_model_pre(text)
    text = _replace_model_code(text)
    text = _replace_model_bold(text)
    lines: list[str] = []
    for line in text.split("\n"):
        if line.count("\\n") >= 2:
            line = line.replace("\\n", "\n")
        lines.append(line)
    return "\n".join(lines)


def _replace_breaks(text: str) -> str:
    output: list[str] = []
    index = 0
    lowered = text.lower()
    while index < len(text):
        if lowered.startswith("<br", index):
            cursor = index + 3
            while cursor < len(text) and text[cursor] in " \t\n\r":
                cursor += 1
            if cursor < len(text) and text[cursor] == "/":
                cursor += 1
            if cursor < len(text) and text[cursor] == ">":
                output.append("\n")
                index = cursor + 1
                continue
        output.append(text[index])
        index += 1
    return "".join(output)


def _replace_model_pre(text: str) -> str:
    output: list[str] = []
    index = 0
    lowered = text.lower()
    while index < len(text):
        start = lowered.find("<pre>", index)
        if start == -1:
            output.append(text[index:])
            break
        parsed = _parse_model_pre(text, lowered, start)
        if parsed is None:
            output.append(text[index : start + 1])
            index = start + 1
            continue
        language, body, end = parsed
        output.append(text[index:start])
        output.append(f"```{language}\n{body}\n```")
        index = end
    return "".join(output)


def _parse_model_pre(text: str, lowered: str, start: int) -> tuple[str, str, int] | None:
    cursor = start + len("<pre>")
    while cursor < len(text) and text[cursor] in " \t\n\r":
        cursor += 1
    if not lowered.startswith("<code", cursor):
        return None
    cursor += len("<code")
    language = ""
    while cursor < len(text) and text[cursor] in " \t\n\r":
        cursor += 1
    marker = 'class="language-'
    if lowered.startswith(marker, cursor):
        cursor += len(marker)
        quote = text.find('"', cursor)
        if quote == -1 or not _is_language(text[cursor:quote]):
            return None
        language = text[cursor:quote]
        cursor = quote + 1
        while cursor < len(text) and text[cursor] in " \t\n\r":
            cursor += 1
    if cursor >= len(text) or text[cursor] != ">":
        return None
    body_start = cursor + 1
    close = lowered.find("</code>", body_start)
    if close == -1:
        return None
    after = close + len("</code>")
    while after < len(text) and text[after] in " \t\n\r":
        after += 1
    if not lowered.startswith("</pre>", after):
        return None
    body = _plain(text[body_start:close]).strip("\n")
    return language, body, after + len("</pre>")


def _replace_model_code(text: str) -> str:
    return _replace_paired_tag(text, "code", _code_to_markdown)


def _replace_model_bold(text: str) -> str:
    return _replace_paired_tag(text, "b", lambda body: f"**{_plain(body)}**")


def _replace_paired_tag(text: str, name: str, convert) -> str:
    output: list[str] = []
    index = 0
    lowered = text.lower()
    open_tag = f"<{name}>"
    close_tag = f"</{name}>"
    while index < len(text):
        start = lowered.find(open_tag, index)
        if start == -1:
            output.append(text[index:])
            break
        close = lowered.find(close_tag, start + len(open_tag))
        if close == -1:
            output.append(text[index:])
            break
        output.append(text[index:start])
        output.append(convert(text[start + len(open_tag) : close]))
        index = close + len(close_tag)
    return "".join(output)


def _code_to_markdown(body: str) -> str:
    plain = _plain(body)
    if "\n" in plain:
        return f"```\n{plain.strip(chr(10))}\n```"
    return f"`{plain}`"


def _plain(text: str) -> str:
    return text.replace("&lt;", "<").replace("&gt;", ">").replace("&amp;", "&")


def _is_language(token: str) -> bool:
    return bool(token) and len(token) <= 32 and all(char in _LANGUAGE_CHARS for char in token)


def _heading_title(line: str) -> str | None:
    index = 0
    while index < 3 and index < len(line) and line[index] in " \t":
        index += 1
    if index < len(line) and line[index] in " \t":
        return None
    marks = 0
    while marks < 6 and index < len(line) and line[index] == "#":
        marks += 1
        index += 1
    if marks == 0 or (index < len(line) and line[index] == "#"):
        return None
    if index >= len(line) or line[index] not in " \t":
        return None
    title = line[index:].strip()
    return title or None


def _is_rule(line: str) -> bool:
    index = 0
    while index < 3 and index < len(line) and line[index] in " \t":
        index += 1
    if index >= len(line) or (index < len(line) and line[index] in " \t"):
        return False
    char = line[index]
    if char not in "-*_":
        return False
    count = 0
    while index < len(line) and line[index] == char:
        count += 1
        index += 1
    return count >= 3 and not line[index:].strip()


def _bullet_end(line: str) -> int | None:
    index = 0
    while index < len(line) and line[index] in " \t":
        index += 1
    if index >= len(line):
        return None
    marker = line[index]
    if marker == "-" and index + 1 < len(line) and line[index + 1] in " \t":
        return index + 1
    if marker == "*" and not line.startswith("**", index):
        if index + 1 < len(line) and line[index + 1] in " \t":
            return index + 1
    return None


def _is_separator_cell(cell: str) -> bool:
    text = cell.strip()
    if text.startswith(":"):
        text = text[1:]
    if text.endswith(":"):
        text = text[:-1]
    return len(text) >= 3 and text == "-" * len(text)


def _join_wrapped_lines(cell: str) -> str:
    parts: list[str] = []
    index = 0
    while index < len(cell):
        newline = cell.find("\n", index)
        if newline == -1:
            parts.append(cell[index:])
            break
        left = newline
        while left > index and cell[left - 1] in " \t":
            left -= 1
        right = newline + 1
        while right < len(cell) and cell[right] in " \t":
            right += 1
        parts.append(cell[index:left])
        parts.append(" ")
        index = right
    return "".join(parts)


def _token_spans(value: str) -> list[tuple[int, int]]:
    spans: list[tuple[int, int]] = []
    index = 0
    while index < len(value):
        start = value.find("\ue000", index)
        if start == -1:
            break
        end = value.find("\ue001", start + 1)
        number = value[start + 1 : end] if end != -1 else ""
        if end == -1 or not number.isdigit():
            index = start + 1
            continue
        spans.append((start, end + 1))
        index = end + 1
    return spans


def _restore_tokens(text: str, fences: list[tuple[str, str]]) -> str:
    output: list[str] = []
    position = 0
    for start, end in _token_spans(text):
        output.append(text[position:start])
        output.append(_restore(fences[int(text[start + 1 : end - 1])]))
        position = end
    output.append(text[position:])
    return "".join(output)


def _wrapped_in_stars(body: str) -> bool:
    return len(body) > 4 and body.startswith("**") and body.endswith("**") and "\n" not in body


def _find_block(html: str, start: int) -> tuple[int, int] | None:
    pre = html.find("<pre>", start)
    quote = html.find("<blockquote>", start)
    positions = [item for item in (pre, quote) if item != -1]
    if not positions:
        return None
    begin = min(positions)
    if html.startswith("<blockquote>", begin):
        close = html.find("</blockquote>", begin)
        if close == -1:
            return None
        return begin, close + len("</blockquote>")
    close = html.find("</code></pre>", begin)
    if close == -1 or not html.startswith("<pre><code", begin):
        return None
    return begin, close + len("</code></pre>")


def _split_pre(block: str) -> tuple[str, str, str] | None:
    if not block.startswith("<pre><code"):
        return None
    code_at = block.find("<code")
    open_end = block.find(">", code_at)
    close = block.rfind("</code></pre>")
    if open_end == -1 or close == -1 or close < open_end:
        return None
    return block[: open_end + 1], block[open_end + 1 : close], block[close:]


def _html(text: str) -> str | None:
    fences: list[tuple[str, str]] = []
    hidden = _hide_fences(text, fences)
    hidden = _hide_multiline_ticks(hidden, fences)
    if not fences and not _has_markup(hidden):
        return None
    rendered = _lines(hidden)
    return _bold_outside_code(_restore_tokens(rendered, fences))


def _hide_fences(text: str, fences: list[tuple[str, str]]) -> str:
    output: list[str] = []
    index = 0
    while True:
        start = text.find("```", index)
        if start == -1:
            output.append(text[index:])
            break
        output.append(text[index:start])
        rest = text[start + 3 :]
        newline = rest.find("\n")
        close = rest.find("```")
        if newline == -1 or (close != -1 and close < newline):
            if close == -1:
                fences.append(("", rest.removesuffix("\n")))
                output.append(_fence_token(len(fences) - 1))
                break
            fences.append(("", rest[:close]))
            output.append(_fence_token(len(fences) - 1))
            index = start + 3 + close + 3
            continue
        language = rest[:newline].strip()
        body_start = newline + 1
        close = rest.find("```", body_start)
        body = rest[body_start:] if close == -1 else rest[body_start:close]
        fences.append(_detach_language(language, body.removesuffix("\n")))
        output.append(_fence_token(len(fences) - 1))
        if close == -1:
            break
        index = start + 3 + close + 3
    return "".join(output)


def _hide_multiline_ticks(text: str, fences: list[tuple[str, str]]) -> str:
    output: list[str] = []
    index = 0
    while index < len(text):
        start = text.find("`", index)
        if start == -1:
            output.append(text[index:])
            break
        if text.startswith("```", start):
            output.append(text[index : start + 3])
            index = start + 3
            continue
        end = text.find("`", start + 1)
        if end == -1:
            output.append(text[index:])
            break
        body = text[start + 1 : end]
        if "\n" not in body:
            output.append(text[index : end + 1])
            index = end + 1
            continue
        output.append(text[index:start])
        fences.append(_detach_language("", body.strip("\n")))
        output.append("\n" + _fence_token(len(fences) - 1) + "\n")
        index = end + 1
    return "".join(output)


def _fence_token(number: int) -> str:
    return f"\ue000{number}\ue001"


def _detach_language(language: str, body: str) -> tuple[str, str]:
    if language or "\n" not in body:
        return language, body
    first, rest = body.split("\n", 1)
    token = first.strip()
    if rest.strip() and _is_language(token):
        return token, rest.removesuffix("\n")
    return language, body


def _has_markup(text: str) -> bool:
    if "**" in text or "`" in text:
        return True
    return any(
        line.startswith(">")
        or line.strip().startswith("|")
        or _bullet_end(line) is not None
        or _heading_title(line) is not None
        or _is_rule(line)
        for line in text.split("\n")
    )


def _restore(fence: tuple[str, str]) -> str:
    language, body = fence
    return _pre(language, body)


def _pre(language: str, body: str) -> str:
    if not _is_language(language):
        language = ""
    code = escape(body.strip("\n"), quote=False)
    if language:
        return f'<pre><code class="language-{language}">{code}</code></pre>'
    return f"<pre><code>{code}</code></pre>"


def _lines(text: str) -> str:
    lines = text.split("\n")
    output: list[str] = []
    index = 0
    while index < len(lines):
        if _is_header(lines, index):
            block, index = _header_table(lines, index)
        elif _is_loose(lines[index]):
            block, index = _loose_table(lines, index)
        elif lines[index].strip() == "|":
            index += 1
            continue
        elif lines[index].startswith(">"):
            quotes: list[str] = []
            while index < len(lines) and lines[index].startswith(">"):
                quotes.append(_inline(lines[index][1:].lstrip()))
                index += 1
            block = "<blockquote>" + "\n".join(quotes) + "</blockquote>\n"
        else:
            heading = _heading_title(lines[index])
            bullet = _bullet_end(lines[index])
            if heading is not None:
                block = "<b>" + _inline(heading) + "</b>\n"
            elif _is_rule(lines[index]):
                block = "\n"
            elif bullet is not None:
                block = "• " + _inline(lines[index][bullet:].lstrip()) + "\n"
            else:
                block = _inline(lines[index]) + "\n"
            index += 1
        output.append(block)
    return "".join(output)


def _is_header(lines: list[str], index: int) -> bool:
    line = lines[index].strip()
    if not line.startswith("|") or line.count("|") < 2 or index + 1 >= len(lines):
        return False
    return _is_separator(lines[index + 1])


def _is_separator(line: str) -> bool:
    cells = _cells(line)
    return bool(cells) and all(_is_separator_cell(cell) for cell in cells)


def _is_loose(line: str) -> bool:
    stripped = line.strip()
    return stripped.startswith("|") and stripped.count("|") >= 2 and not _is_separator(stripped)


def _header_table(lines: list[str], index: int) -> tuple[str, int]:
    headers = _cells(lines[index])
    index += 2
    blocks: list[str] = []
    while index < len(lines) and _is_loose(lines[index]):
        row, index = _take_row(lines, index)
        cells = _cells(row)
        formatted: list[str] = []
        for cell_index, cell in enumerate(cells):
            value = _render_value(_collapse(cell))
            if not value:
                continue
            label = headers[cell_index].strip() if cell_index < len(headers) else ""
            if label and not _is_separator(label):
                formatted.append(f"<b>{_inline(label)}:</b> {value}")
            else:
                formatted.append(value)
        if formatted:
            blocks.append("\n".join(formatted))
    return ("\n\n".join(blocks) + "\n") if blocks else "", index


def _loose_table(lines: list[str], index: int) -> tuple[str, int]:
    blocks: list[str] = []
    while index < len(lines) and _is_loose(lines[index]):
        row, index = _take_row(lines, index)
        parts = [_render_value(_collapse(cell)) for cell in _cells(row) if cell.strip()]
        if parts:
            blocks.append("\n".join(parts))
    return ("\n\n".join(blocks) + "\n") if blocks else "", index


def _take_row(lines: list[str], index: int) -> tuple[str, int]:
    buffer = lines[index]
    index += 1
    guard = 0
    while index < len(lines) and guard < 30 and not _row_done(buffer):
        if "\ue000" in lines[index] or not lines[index].strip() or lines[index].strip() == "|":
            break
        buffer += " " + lines[index].strip()
        index += 1
        guard += 1
    if index < len(lines) and "\ue000" in lines[index]:
        buffer += "\n" + lines[index].strip()
        index += 1
        if index < len(lines) and lines[index].strip() == "|":
            index += 1
    return buffer, index


def _row_done(buffer: str) -> bool:
    return buffer.rstrip().endswith("|") and buffer.count("|") >= 2


def _cells(line: str) -> list[str]:
    stripped = line.strip()
    if stripped.startswith("|"):
        stripped = stripped[1:]
    if stripped.endswith("|"):
        stripped = stripped[:-1]
    return [cell.strip() for cell in stripped.split("|")]


def _collapse(cell: str) -> str:
    if "\ue000" in cell:
        return cell.strip()
    return _join_wrapped_lines(cell).strip()


def _render_value(value: str) -> str:
    parts: list[str] = []
    position = 0
    for start, end in _token_spans(value):
        parts.append(_inline(value[position:start]))
        parts.append("\n" + value[start:end])
        position = end
    parts.append(_inline(value[position:]))
    return "".join(parts).strip()


def _inline(text: str) -> str:
    parts: list[str] = []
    index = 0
    while index < len(text):
        if text.startswith("**", index):
            end = text.find("**", index + 2)
            body = text[index + 2 : end] if end != -1 else ""
            if end != -1 and body.strip() and "\n" not in body:
                parts.append("<b>" + _inline(body) + "</b>")
                index = end + 2
                continue
        if text.startswith("`", index) and not text.startswith("```", index):
            end = text.find("`", index + 1)
            if end != -1 and end > index + 1 and "\n" not in text[index + 1 : end]:
                body = text[index + 1 : end]
                if _wrapped_in_stars(body):
                    parts.append("<b>" + escape(body[2:-2], quote=False) + "</b>")
                else:
                    parts.append("<code>" + escape(body, quote=False) + "</code>")
                index = end + 1
                continue
        next_mark = len(text)
        for marker in ("**", "`"):
            found = text.find(marker, index)
            if found != -1:
                next_mark = min(next_mark, found)
        if next_mark == index:
            parts.append(escape(text[index], quote=False))
            index += 1
        else:
            parts.append(escape(text[index:next_mark], quote=False))
            index = next_mark
    return "".join(parts)


def _bold_outside_code(html: str) -> str:
    """Двойные звёздочки вне кода — это <b>, как требует HTML-режим Telegram."""
    output: list[str] = []
    index = 0
    while index < len(html):
        if html.startswith("<pre", index) or html.startswith("<code", index):
            close = "</pre>" if html.startswith("<pre", index) else "</code>"
            end = html.find(close, index)
            if end == -1:
                output.append(html[index:])
                break
            end += len(close)
            output.append(html[index:end])
            index = end
            continue
        if html.startswith("**", index):
            end = html.find("**", index + 2)
            body = html[index + 2 : end] if end != -1 else ""
            if end != -1 and body and "\n" not in body and "*" not in body:
                output.append("<b>" + body + "</b>")
                index = end + 2
                continue
        output.append(html[index])
        index += 1
    return "".join(output)


def _split_html(html: str, limit: int) -> list[str]:
    return _pack_units(_paragraph_units(_atoms(html)), limit)


def _atoms(html: str) -> list[str]:
    atoms: list[str] = []
    position = 0
    while position < len(html):
        found = _find_block(html, position)
        if found is None:
            atoms.extend(_text_atoms(html[position:]))
            break
        begin, end = found
        atoms.extend(_text_atoms(html[position:begin]))
        atoms.append(html[begin:end])
        position = end
    return atoms


def _fit_block(block: str, limit: int) -> list[str]:
    if len(block) <= limit:
        return [block]
    pieces = _split_pre(block)
    if pieces is None:
        return _text_atoms(block)
    open_tag, body, close_tag = pieces
    budget = limit - len(open_tag) - len(close_tag)
    if budget < 1:
        raise ValueError("limit must fit the markup")
    chunks: list[str] = []
    start = 0
    while start < len(body):
        end = min(start + budget, len(body))
        if end < len(body):
            newline = body.rfind("\n", start, end)
            if newline > start:
                end = newline + 1
        chunks.append(open_tag + body[start:end] + close_tag)
        start = end
    return chunks


def _text_atoms(text: str) -> list[str]:
    atoms: list[str] = []
    start = 0
    while start < len(text):
        newline = text.find("\n", start)
        if newline == -1:
            atoms.append(text[start:])
            break
        atoms.append(text[start : newline + 1])
        start = newline + 1
    return atoms


def _paragraph_units(atoms: list[str]) -> list[str]:
    """Абзац — это строки до пустой. Заголовок не отделяется от следующего абзаца."""
    groups: list[list[str]] = []
    buffer: list[str] = []
    for atom in atoms:
        if not atom:
            continue
        buffer.append(atom)
        if not atom.strip():
            groups.append(buffer)
            buffer = []
    if buffer:
        groups.append(buffer)
    units: list[str] = []
    index = 0
    while index < len(groups):
        combined = list(groups[index])
        index += 1
        while _ends_with_heading(combined) and index < len(groups):
            combined.extend(groups[index])
            index += 1
        units.append("".join(combined))
    return units


def _ends_with_heading(atoms: list[str]) -> bool:
    content = [atom for atom in atoms if atom.strip()]
    return bool(content) and _is_heading_atom(content[-1])


def _is_heading_atom(atom: str) -> bool:
    line = atom.strip()
    if "\n" in line or not line.startswith("<b>") or not line.endswith("</b>"):
        return False
    inside = line[len("<b>") : -len("</b>")]
    return bool(inside) and "<b>" not in inside and "</b>" not in inside


def _pack_units(units: list[str], limit: int) -> list[str]:
    parts: list[str] = []
    current = ""
    for unit in units:
        if not unit:
            continue
        if len(unit) <= limit:
            if current and len(current) + len(unit) > limit:
                parts.append(current)
                current = ""
            current += unit
            continue
        if current:
            parts.append(current)
            current = ""
        parts.extend(_pack_long_unit(unit, limit))
    if current.strip():
        parts.append(current)
    return parts


def _pack_long_unit(unit: str, limit: int) -> list[str]:
    """Длинный абзац режется по строкам, но заголовок уходит вместе со следующим текстом."""
    atoms = _atoms(unit)
    parts: list[str] = []
    current = ""
    index = 0
    while index < len(atoms):
        atom = atoms[index]
        if not atom:
            index += 1
            continue
        if _is_heading_atom(atom):
            end = index + 1
            while end < len(atoms) and atoms[end].strip():
                end += 1
            follower = "".join(atoms[index:end])
            next_len = _next_len(atoms, index)
            heading_fits_ahead = len(follower) <= limit or len(atom) + next_len <= limit
            if current and heading_fits_ahead and len(current) + len(atom) + next_len > limit:
                parts.append(current)
                current = ""
        if len(atom) > limit:
            if current:
                parts.append(current)
                current = ""
            if atom.startswith("<pre") or atom.startswith("<blockquote"):
                parts.extend(_fit_block(atom, limit))
            else:
                parts.extend(split_telegram_text(atom, limit))
            index += 1
            continue
        if current and len(current) + len(atom) > limit:
            parts.append(current)
            current = ""
        current += atom
        index += 1
    if current.strip():
        parts.append(current)
    return parts


def _next_len(atoms: list[str], index: int) -> int:
    if index + 1 >= len(atoms):
        return 0
    return len(atoms[index + 1])
