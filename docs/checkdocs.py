# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Check what the pages say about the application against the application.

The pages name menu items as **Menu > Item** and commands as `Group/Name`;
this fails on any that the actions reference (actions.md, made from the
application) does not have, so a renamed or removed item breaks the build
rather than leaving a page that describes something that is not there. It
also fails on links to pages, or sections of pages, that do not exist.

    python3 checkdocs.py DOCS_DIR
"""

import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from md2html import slug  # noqa: E402


def reference(docs):
    """The menu paths and the commands the actions reference lists."""
    paths = set()
    commands = set()
    menu = None
    with open(os.path.join(docs, "actions.md"), encoding="utf-8") as f:
        for line in f:
            m = re.match(r"## (.*)", line)
            if m:
                menu = m.group(1).strip()
                continue
            if not line.startswith("| ") or line.startswith("| Menu item") or line.startswith("| Command |"):
                continue
            cells = [c.strip() for c in line.strip().strip("|").split(" | ")]
            for c in cells:
                for name in re.findall(r"`([A-Z][A-Za-z]*/[A-Za-z0-9]+)`", c):
                    commands.add(name)
            if menu and menu != "Commands without a menu item":
                item = cells[0].rstrip(" >")
                # "Import..." is named without its dots as well.
                parts = [p.strip().rstrip(".").strip() if p.strip().endswith("...") else p.strip()
                         for p in item.split(" > ")]
                # The item, and each sub menu on the way to it.
                for i in range(len(parts)):
                    paths.add(" > ".join([menu] + parts[:i + 1]))
                paths.add(menu)
    return paths, commands


def anchors(md):
    """The ids of a page's sections, as md2html.py gives them."""
    out = set()
    for line in md.splitlines():
        m = re.match(r"#{2,3} (.*?)(?: \{#([\w-]+)\})?$", line)
        if m:
            out.add(m.group(2) or slug(m.group(1)))
    return out


def main(argv):
    if len(argv) != 1:
        sys.exit(__doc__)
    docs = argv[0]
    paths, commands = reference(docs)
    pages = {}
    for path in glob.glob(os.path.join(docs, "*.md")):
        with open(path, encoding="utf-8") as f:
            pages[os.path.basename(path)[:-3]] = f.read()
    # A page made from Markdown counts before its HTML is made.
    html = {os.path.basename(p)[:-5] for p in glob.glob(os.path.join(docs, "*.html"))}
    html |= set(pages)
    errors = []
    for name, md in pages.items():
        if "actions" == name:
            continue
        # Code blocks name files and commands of other programs.
        text = re.sub(r"```.*?```", "", md, flags=re.S)
        for path in re.findall(r"\*\*([A-Z][^*\n]*? > [^*\n]+)\*\*", text):
            if path.rstrip(" .") not in paths:
                errors.append("%s.md: no menu item \"%s\"" % (name, path))
        for command in re.findall(r"`([A-Z][A-Za-z]*/[A-Z][A-Za-z0-9]*)`", text):
            if command not in commands:
                errors.append("%s.md: no command \"%s\"" % (name, command))
        for target in re.findall(r"\]\(([\w-]+)\.html(?:#([\w-]+))?\)", text):
            page, anchor = target
            if page not in html:
                errors.append("%s.md: no page \"%s.html\"" % (name, page))
            elif anchor and page in pages and anchor not in anchors(pages[page]):
                errors.append("%s.md: no section \"%s.html#%s\"" % (name, page, anchor))
        for anchor in re.findall(r"\]\(#([\w-]+)\)", text):
            if anchor not in anchors(md):
                errors.append("%s.md: no section \"#%s\"" % (name, anchor))
    if errors:
        sys.exit("\n".join(sorted(set(errors))))


if __name__ == "__main__":
    main(sys.argv[1:])
