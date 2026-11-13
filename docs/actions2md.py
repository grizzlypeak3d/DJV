# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Write the actions reference page from the application's own list.

"djv -actionsJSON" prints the menus, every action with its keys and
description, and every command; this writes them as the reference page, in
Markdown, for md2html.py. Made from the application rather than by hand, so
it cannot drift from it: the check below is what continuous integration runs.

    python3 actions2md.py DJV TMP actions.md
    python3 actions2md.py --check DJV TMP actions.md

DJV is the application, run with its settings and log in TMP so that it
neither reads nor writes the user's.
"""

import json
import os
import subprocess
import sys

INTRO = """# Actions reference

Every menu item, with its default keys and the name of its command, as the
application reports them. The descriptions are the items' tooltips. This
page is made from the application itself, so it lists exactly what the
version it was made from has.

**Ctrl/Cmd** is Command on macOS and Control elsewhere; **Ctrl** is Control
everywhere. The keys can be changed in the **Shortcuts** section of the
**Settings** tool.

The commands are how the menu items are run from outside the window: with
`-command` on the [command line](command-line.html), and from
[Python](python.html). Commands that take a value say so.
"""


def cell(text):
    """Text for a table cell: one line, with no column breaks in it."""
    return " ".join(text.split()).replace("|", "\\|")


def rows(items, actions, prefix=""):
    """The table rows of a menu's items, with sub menus' items under their
    titles."""
    out = []
    for item in items:
        if "action" in item:
            action = actions[item["action"]]
            keys = ", ".join("<kbd>%s</kbd>" % k for k in action["shortcuts"])
            out.append("| %s | %s | `%s` | %s |" % (
                cell(prefix + action["text"]),
                keys,
                action["name"],
                cell(action["tooltip"])))
        else:
            title = item["menu"]
            if item["items"]:
                out.extend(rows(item["items"], actions, prefix + title + " > "))
            else:
                # Filled as the application runs: the recent files, the
                # files to compare.
                out.append("| %s | | | %s |" % (
                    cell(prefix + title + " >"),
                    "Filled in as files are opened."))
    return out


def convert(data):
    actions = {a["name"]: a for a in data["actions"]}
    lines = [INTRO]
    for menu in data["menus"]:
        lines.append("## %s" % menu["menu"])
        lines.append("")
        lines.append("| Menu item | Keys | Command | Description |")
        lines.append("|---|---|---|---|")
        lines.extend(rows(menu["items"], actions))
        lines.append("")
    commands = [c for c in data["commands"] if c["name"] not in actions]
    if commands:
        lines.append("## Commands without a menu item")
        lines.append("")
        lines.append("For scripts and the command line.")
        lines.append("")
        lines.append("| Command | Description |")
        lines.append("|---|---|")
        for command in commands:
            lines.append("| `%s` | %s |" % (command["name"], cell(command["doc"])))
        lines.append("")
    return "\n".join(lines)


def main(argv):
    check = "--check" in argv
    args = [a for a in argv if a != "--check"]
    if len(args) != 3:
        sys.exit(__doc__)
    exe, tmp, out = args
    result = subprocess.run(
        [
            exe, "-actionsJSON", "-resetSettings",
            "-settingsFile", os.path.join(tmp, "actions-settings.json"),
            "-logFile", os.path.join(tmp, "actions.log")
        ],
        capture_output=True,
        text=True,
        check=True)
    page = convert(json.loads(result.stdout))
    if check:
        with open(out, encoding="utf-8") as f:
            if f.read() != page:
                sys.exit("%s is out of date with the application; "
                         "build the djvDocsActions target" % out)
        return
    with open(out, "w", encoding="utf-8") as f:
        f.write(page)


if __name__ == "__main__":
    main(sys.argv[1:])
