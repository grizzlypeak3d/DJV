# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Write a documentation page from its Markdown.

The pages are written in Markdown, which is what agents and people read in
the repository, and this makes the HTML the website, the application and the
iPad show. Only what the pages use, so that it stays a page of code rather
than a dependency, and so that the help viewer inside the application can
draw the same subset:

- "# Title", the page's one h1; "## Section" and "### Part", with an id from
  the text, or one given as "## Section {#id}"
- paragraphs; **bold**, *italic*, `code`, [links](page.html), ![pictures](x.svg)
- inline <kbd>keys</kbd>, the one HTML the pages write
- "- " and "1. " lists, nested by two spaces
- "Term" then ": definition" lines, a definition list
- tables of "| a | b |" rows, the second a "|---|" rule
- ``` code blocks
- "> [!NOTE]" and "> [!TIP]" blocks, the pages' notes

The navigation and the table of contents are left empty, for Nav.cmake to
fill the way it fills every page; a page with sections gets a table of
contents.

    python3 md2html.py PAGE.md PAGE.html
    python3 md2html.py --check PAGE.md PAGE.html
"""

import html
import re
import sys

HEAD = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title} - DJV</title>
<link rel="stylesheet" href="assets/docs.css">
<link rel="icon" href="assets/DJV_Icon_32.svg">
</head>
<body>
<input type="checkbox" id="nav-toggle">
<label for="nav-toggle" id="nav-button">Menu</label>
<nav>
<a class="logo" href="index.html"><img src="assets/DJV_Icon_32.svg" alt="">DJV</a>
<ul>
<li></li>
</ul>
</nav>
<main>
"""

TOC = """<div class="toc">
<ul>
<li></li>
</ul>
</div>
"""

TAIL = """</main>
</body>
</html>
"""


def slug(text):
    """An id from a heading's text: "A/B comparison" is "ab-comparison"."""
    text = re.sub(r"<[^>]+>", "", text).lower()
    text = re.sub(r"[^a-z0-9 -]", "", text)
    return re.sub(r"[ -]+", "-", text).strip("-")


def inline(text):
    """The inline markup of a run of text."""
    # Code first, so nothing inside it is taken for markup.
    parts = re.split(r"(`[^`]+`)", text)
    out = []
    for part in parts:
        if part.startswith("`") and part.endswith("`") and len(part) > 1:
            out.append("<code>" + html.escape(part[1:-1], quote=False) + "</code>")
            continue
        # Kept: the one tag the pages write.
        kbds = []

        def keep_kbd(m):
            kbds.append(m.group(1))
            return "\x00%d\x00" % (len(kbds) - 1)
        part = re.sub(r"<kbd>(.*?)</kbd>", keep_kbd, part)
        part = html.escape(part, quote=False)
        part = re.sub(
            r"!\[([^\]]*)\]\(([^)\s]+)\)",
            lambda m: '<img src="%s" alt="%s">' % (m.group(2), m.group(1)),
            part)
        part = re.sub(
            r"\[([^\]]+)\]\(([^)\s]+)\)",
            lambda m: '<a href="%s">%s</a>' % (m.group(2), m.group(1)),
            part)
        part = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", part)
        part = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<em>\1</em>", part)
        part = re.sub(
            r"\x00(\d+)\x00",
            lambda m: "<kbd>" + html.escape(kbds[int(m.group(1))], quote=False) + "</kbd>",
            part)
        out.append(part)
    return "".join(out)


def convert(lines):
    """The page's title and its body, as HTML."""
    title = ""
    body = []
    has_sections = False
    i = 0

    def is_block_start(line):
        return (
            line.startswith("#") or
            line.startswith("```") or
            line.startswith("> ") or
            line.startswith("|") or
            re.match(r"\s*(- |\d+\. )", line) is not None)

    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue

        m = re.match(r"(#{1,3}) (.*?)(?: \{#([\w-]+)\})?$", line)
        if m:
            level = len(m.group(1))
            text = m.group(2)
            if 1 == level:
                title = text
                body.append("<h1>%s</h1>" % inline(text))
                body.append("@TOC@")
            else:
                has_sections = has_sections or 2 == level
                body.append('<h%d id="%s">%s</h%d>' % (
                    level, m.group(3) or slug(text), inline(text), level))
            i += 1
            continue

        if line.startswith("```"):
            code = []
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                code.append(lines[i])
                i += 1
            i += 1
            body.append("<pre><code>" + html.escape("\n".join(code), quote=False) + "</code></pre>")
            continue

        m = re.match(r"> \[!(NOTE|TIP)\]$", line)
        if m:
            text = []
            i += 1
            while i < len(lines) and lines[i].startswith(">"):
                text.append(lines[i][1:].strip())
                i += 1
            body.append('<div class="%s">%s</div>' % (m.group(1).lower(), inline(" ".join(text))))
            continue

        if line.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")])
                i += 1
            head = "".join("<th>%s</th>" % inline(c) for c in rows[0])
            cells = "".join(
                "<tr>" + "".join("<td>%s</td>" % inline(c) for c in row) + "</tr>"
                for row in rows[2:])
            body.append(
                '<div class="table-wrap"><table><thead><tr>%s</tr></thead><tbody>%s</tbody></table></div>' %
                (head, cells))
            continue

        if re.match(r"\s*(- |\d+\. )", line):
            # A list, nested by indent: two spaces a level.
            items = []
            while i < len(lines) and lines[i].strip():
                m = re.match(r"(\s*)(- |\d+\. )(.*)", lines[i])
                if m:
                    items.append([len(m.group(1)) // 2, m.group(2).strip(), m.group(3)])
                else:
                    # A line continuing the item before it.
                    items[-1][2] += " " + lines[i].strip()
                i += 1
            out = []
            stack = []
            for level, marker, text in items:
                tag = "ol" if marker[0].isdigit() else "ul"
                while len(stack) > level + 1:
                    out.append("</li></%s>" % stack.pop())
                if len(stack) == level + 1:
                    out.append("</li>")
                while len(stack) < level + 1:
                    out.append("<%s>" % tag)
                    stack.append(tag)
                out.append("<li>" + inline(text))
            while stack:
                out.append("</li></%s>" % stack.pop())
            body.append("".join(out))
            continue

        if i + 1 < len(lines) and lines[i + 1].startswith(": "):
            items = []
            while i < len(lines) and lines[i].strip():
                if lines[i].startswith(": "):
                    items.append("<dd>%s</dd>" % inline(lines[i][2:].strip()))
                else:
                    items.append("<dt>%s</dt>" % inline(lines[i].strip()))
                i += 1
            body.append("<dl>\n" + "\n".join(items) + "\n</dl>")
            continue

        text = []
        while i < len(lines) and lines[i].strip() and not is_block_start(lines[i]):
            text.append(lines[i].strip())
            i += 1
        body.append("<p>%s</p>" % inline(" ".join(text)))

    out = "\n".join(body) + "\n"
    out = out.replace("@TOC@\n", TOC if has_sections else "")
    return title, out


def main(argv):
    check = "--check" in argv
    args = [a for a in argv if a != "--check"]
    if len(args) != 2:
        sys.exit(__doc__)
    with open(args[0], encoding="utf-8") as f:
        title, body = convert(f.read().splitlines())
    if not title:
        sys.exit("%s: no title" % args[0])
    page = HEAD.format(title=html.escape(title, quote=False)) + body + TAIL
    if check:
        # Against the page as written, before Nav.cmake filled it: the
        # navigation and the table of contents are left out of the compare.
        with open(args[1], encoding="utf-8") as f:
            written = f.read()
        def strip(s):
            s = re.sub(r"<nav>.*?</nav>", "<nav></nav>", s, flags=re.S)
            return re.sub(r'<div class="toc">.*?</div>', '<div class="toc"></div>', s, flags=re.S)
        if strip(written) != strip(page):
            sys.exit("%s is out of date with %s" % (args[1], args[0]))
        return
    with open(args[1], "w", encoding="utf-8") as f:
        f.write(page)


if __name__ == "__main__":
    main(sys.argv[1:])
