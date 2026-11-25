# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""Put the C++ headers' comments into a module's type stubs, as docstrings.

Usage: doc_stub.py --headers <dir>... --bindings <dir>... [--report <file>]
           <stub.pyi>...

The bindings have almost no docstrings of their own: what a class or a
method is for is in the //! comment above it in its header. nanobind's
stubgen writes the signatures only, so this reads the binding sources to
learn which C++ class and member each Python name is bound to, reads the
headers' comments, and adds them to the stubs, which is where editors and
agents look. The bindings are not changed.

Matching is by name, so it is a reading of the sources rather than a
compiler's: a binding written as a lambda is matched to the C++ member of
the same name (or getX, isX, observeX for a property), and a class whose
name is in more than one namespace is told apart by the namespaces the
binding file uses. What no comment was found for is counted, and listed
with --report.
"""

import argparse
import ast
import os
import re
import sys
import textwrap


# Headers


class HeaderDocs:
    """The //! comments of the headers, by qualified C++ name."""

    def __init__(self):
        # Class or enum -> its comment, by qualified name ("ftk::Image").
        self.classes = {}
        # (qualified class or namespace, member) -> comment.
        self.members = {}
        # Last component -> qualified names, for the lookups by short name.
        self.byShortName = {}
        # Qualified class -> its base classes' names, as written.
        self.bases = {}

    def addClass(self, qualified, doc):
        if doc and qualified not in self.classes:
            self.classes[qualified] = doc
        self.byShortName.setdefault(qualified.split("::")[-1], set()).add(qualified)

    def addMember(self, scope, name, doc):
        if not doc:
            return
        key = (scope, name)
        prev = self.members.get(key)
        if prev is None:
            self.members[key] = doc
        elif doc not in prev:
            # Overloads described apart: both, in the order declared.
            self.members[key] = prev + "\n\n" + doc


_CLASS_RE = re.compile(
    r"^\s*(?:template\s*<[^>]*>\s*)?(class|struct)\s+(?:[A-Z_][A-Z0-9_]*\s+)*([A-Za-z_]\w*)\s*(?:final\s*)?(?::[^{;]*)?(\{|$)")
_ENUM_RE = re.compile(
    r"^\s*enum\s+(?:class|struct)?\s*(?:[A-Z_][A-Z0-9_]*\s+)*([A-Za-z_]\w*)\s*(?::\s*[\w:]+\s*)?(\{|$)")
_NAMESPACE_RE = re.compile(r"^\s*namespace\s+([A-Za-z_][\w:]*)\s*(\{|$)")
_TRAILING_DOC_RE = re.compile(r"//!<\s?(.*)$")


def _cleanDoc(lines):
    """A docstring from comment lines: Doxygen's markup taken out,
    paragraphs kept."""
    out = []
    for line in lines:
        line = line.rstrip()
        if re.match(r"^\s*\\name\b", line) or line.strip() in ("@{", "@}"):
            continue
        line = re.sub(r"\\brief\s+", "", line)
        line = re.sub(r"\\param(?:\[\w+\])?\s+(\w+)\s*", r"\1: ", line)
        line = re.sub(r"\\returns?\s+", "Returns ", line)
        line = re.sub(r"\\(?:sa|see)\s+", "See ", line)
        line = re.sub(r"\\(?:note)\s+", "Note: ", line)
        line = re.sub(r"\\[a-z]+\s*", "", line)
        out.append(line)
    while out and not out[0].strip():
        out.pop(0)
    while out and not out[-1].strip():
        out.pop()
    # Paragraphs: a blank comment line between them; the lines within one
    # joined, except a list or an indented block, which keep theirs.
    text = []
    para = []
    for line in out:
        if not line.strip():
            if para:
                text.append(" ".join(para))
                para = []
            text.append("")
        elif re.match(r"^\s*([-*]|\d+\.)\s", line) or line.startswith("  "):
            if para:
                text.append(" ".join(para))
                para = []
            text.append(line.rstrip())
        else:
            para.append(line.strip())
    if para:
        text.append(" ".join(para))
    return "\n".join(text).strip()


def _declName(decl):
    """The name a declaration declares: a function's, a field's, or none."""
    decl = re.sub(r"//.*$", "", decl)
    decl = re.sub(r"\b[A-Z_][A-Z0-9_]*_API(?:_TYPE)?\b", "", decl)
    decl = decl.strip()
    if not decl or decl.startswith(("#", "using ", "typedef ", "friend ",
                                    "public:", "protected:", "private:",
                                    "FTK_", "TL_", "DJV_", "template")):
        return None
    if "operator" in decl:
        return None
    m = re.match(r"^[^(=;{]*?([A-Za-z_~]\w*)\s*\(", decl)
    if m:
        return m.group(1)
    m = re.match(r"^[^(;{]*?([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)?(?:=|;|\{)", decl)
    if m:
        return m.group(1)
    return None


def readHeaders(dirs):
    docs = HeaderDocs()
    for top in dirs:
        for root, subdirs, files in os.walk(top):
            subdirs[:] = [d for d in subdirs if not d.endswith(("Test", "Py"))]
            for f in sorted(files):
                if f.endswith(".h"):
                    _readHeader(os.path.join(root, f), docs)
    return docs


def _readHeader(path, docs):
    with open(path, encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines()

    # The scope stack: (kind, name, depth at which it opened).
    scopes = []
    depth = 0
    comment = []
    pending = None
    enumScope = None

    def qualified(name=None):
        parts = [s[1] for s in scopes if s[1]]
        if name:
            parts.append(name)
        return "::".join(parts)

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if stripped.startswith("//!") and not stripped.startswith("//!<"):
            comment.append(stripped[3:][1:] if stripped[3:4] == " " else stripped[3:])
            i += 1
            continue
        if stripped.startswith("///@") or stripped.startswith("///"):
            i += 1
            continue
        if stripped.startswith("//") or not stripped:
            if not stripped:
                comment = []
            i += 1
            continue

        if re.match(r"^template\s*<.*>\s*$", stripped):
            # The class or function is on the next line.
            i += 1
            continue

        doc = _cleanDoc(comment)
        trailing = _TRAILING_DOC_RE.search(line)
        if trailing and not doc:
            doc = _cleanDoc([trailing.group(1)])
        comment = []

        opened = None
        m = _NAMESPACE_RE.match(line)
        if m:
            opened = ("namespace", m.group(1))
        else:
            m = _ENUM_RE.match(line)
            if m and ";" not in line.split("{")[0]:
                opened = ("enum", m.group(1))
                docs.addClass(qualified(m.group(1)), doc)
            else:
                m = _CLASS_RE.match(line)
                if m and not line.rstrip().endswith(";"):
                    opened = ("class", m.group(2))
                    docs.addClass(qualified(m.group(2)), doc)
                    inherit = re.search(r":\s*(.*?)(\{|$)", line[m.end(2):])
                    if inherit:
                        bases = re.findall(
                            r"(?:public|protected|private)\s+([\w:]+)", inherit.group(1))
                        docs.bases[qualified(m.group(2))] = [
                            b for b in bases if not b.startswith("std::")]
                elif scopes and scopes[-1][0] == "enum":
                    # Enumerators: "Name," or "Name = 1,".
                    em = re.match(r"^\s*([A-Za-z_]\w*)\s*(?:=[^,]*)?,?", line)
                    if em and doc:
                        docs.addMember(qualified(), em.group(1), doc)
                elif scopes and scopes[-1][0] in ("class", "namespace"):
                    # A declaration may run over several lines; its name is
                    # on the first.
                    name = _declName(line)
                    if name and doc:
                        docs.addMember(qualified(), name, doc)

        opens = line.count("{")
        closes = line.count("}")
        if opened:
            if opens:
                scopes.append((opened[0], opened[1], depth))
                pending = None
            else:
                # The brace on a line of its own after it.
                pending = opened
        elif pending and stripped.startswith("{"):
            scopes.append((pending[0], pending[1], depth))
            pending = None
        elif pending and ";" in line:
            # A declaration only ("class X;"), or the brace never came.
            pending = None
        depth += opens - closes
        while scopes and depth <= scopes[-1][2]:
            scopes.pop()
        i += 1
    return docs


# Bindings


class Binding:
    def __init__(self, pyName, cppType, namespaces):
        self.pyName = pyName
        self.cppType = cppType
        self.namespaces = namespaces
        # Python member -> C++ member names to look for, in order.
        self.members = {}


_BIND_CLASS_RE = re.compile(
    r"nb::(class_|enum_)\s*<\s*([\w:]+)[^>]*>\s*\(\s*\w+\s*,\s*\"(\w+)\"")
_BIND_MEMBER_RE = re.compile(
    r"\.(def|def_static|def_prop_ro|def_prop_rw|def_rw|def_ro|value)\s*\(\s*\"(\w+)\"\s*,\s*([^,)]*)")
_MODULE_FUNC_RE = re.compile(r"\bm\s*\.\s*def\s*\(\s*\"(\w+)\"\s*,")


def _candidates(kind, pyName, target):
    out = []
    m = re.match(r"^\s*&?([\w:]+)::(\w+)\s*$", target)
    if m:
        out.append(m.group(2))
    elif kind == "value":
        m = re.match(r"^\s*[\w:]*::(\w+)\s*$", target)
        if m:
            out.append(m.group(1))
    cap = pyName[:1].upper() + pyName[1:]
    out.append(pyName)
    if kind in ("def_prop_ro", "def_prop_rw", "def_ro", "def_rw"):
        out += ["get" + cap, "is" + cap, "has" + cap, "observe" + cap]
    # A Python name for an observable property whose C++ is observeX.
    if pyName.startswith("observe"):
        out.append(pyName)
    return out


def readBindings(dirs):
    bindings = []
    functions = []
    for top in dirs:
        for root, _, files in os.walk(top):
            for f in sorted(files):
                if f.endswith(".cpp"):
                    _readBinding(os.path.join(root, f), bindings, functions)
    return bindings, functions


def _readBinding(path, bindings, functions):
    with open(path, encoding="utf-8", errors="replace") as f:
        text = f.read()
    namespaces = re.findall(r"using\s+namespace\s+([\w:]+)\s*;", text)
    namespaces += re.findall(r"namespace\s+(\w+)\s*\{", text)
    starts = [(m.start(), m) for m in _BIND_CLASS_RE.finditer(text)]
    for n, (pos, m) in enumerate(starts):
        end = starts[n + 1][0] if n + 1 < len(starts) else len(text)
        b = Binding(m.group(3), m.group(2), namespaces)
        block = text[pos:end]
        for mm in _BIND_MEMBER_RE.finditer(block):
            kind, pyName, target = mm.group(1), mm.group(2), mm.group(3)
            b.members.setdefault(pyName, [])
            for c in _candidates(kind, pyName, target):
                if c not in b.members[pyName]:
                    b.members[pyName].append(c)
        if re.search(r"nb::new_\s*\(\s*&[\w:]+::create", block):
            b.members.setdefault("__init__", ["create"])
        bindings.append(b)
    for m in _MODULE_FUNC_RE.finditer(text):
        # The function bound: the first "&name" in the call, through an
        # overload_cast or not; a lambda has none, and goes by its name.
        rest = text[m.end():m.end() + 400]
        rest = rest[:rest.find(".def") if ".def" in rest else len(rest)]
        target = re.search(r"&\s*([\w:]+)", rest)
        functions.append((m.group(1), target.group(1) if target else m.group(1), namespaces))


def _resolve(docs, cppType, namespaces):
    """The qualified C++ name a binding's type is."""
    if cppType in docs.classes or "::" in cppType and any(
            q.endswith("::" + cppType) or q == cppType for q in docs.classes):
        for q in docs.classes:
            if q == cppType or q.endswith("::" + cppType):
                return q
    short = cppType.split("::")[-1]
    candidates = sorted(docs.byShortName.get(short, ()))
    if not candidates:
        return None
    if len(candidates) == 1:
        return candidates[0]
    # The namespaces the binding file is in and uses.
    for ns in reversed(namespaces):
        for q in candidates:
            if ("::" + ns + "::") in ("::" + q):
                return q
    return candidates[0]


# Stubs


def _docLines(doc, indent):
    doc = doc.replace("\\", "\\\\").replace('"""', '\\"\\"\\"')
    # A quote last would run into the closing ones.
    if doc.endswith('"'):
        doc = doc[:-1] + '\\"'
    pad = " " * indent
    # Paragraphs wrapped to the width of the file, the first line shorter
    # by the quotes beside it; lists and indented lines are left as they
    # are.
    width = max(40, 79 - indent)
    lines = []
    for n, line in enumerate(doc.split("\n")):
        if line.strip() and not re.match(r"^\s*([-*]|\d+\.)\s|^\s", line):
            lead = '"""' if n == 0 else ""
            wrapped = textwrap.wrap(
                line, width, initial_indent=lead,
                break_long_words=False, break_on_hyphens=False)
            wrapped[0] = wrapped[0][len(lead):]
            lines += wrapped
        else:
            lines.append(line)
    if len(lines) == 1 and len(lines[0]) + 6 <= width:
        return [pad + '"""' + lines[0] + '"""']
    out = [pad + '"""' + lines[0]]
    out += [(pad + l) if l.strip() else "" for l in lines[1:]]
    out.append(pad + '"""')
    return out


def _hasDocstring(node):
    return (node.body and isinstance(node.body[0], ast.Expr) and
            isinstance(node.body[0].value, ast.Constant) and
            isinstance(node.body[0].value.value, str))


def _isSetter(node):
    for d in node.decorator_list:
        if isinstance(d, ast.Attribute) and d.attr in ("setter", "deleter"):
            return True
    return False


class Docs:
    def __init__(self, headers, bindings, functions):
        self.headers = headers
        self.byPyName = {}
        for b in bindings:
            self.byPyName.setdefault(b.pyName, []).append(b)
        self.functions = {}
        for pyName, target, namespaces in functions:
            self.functions.setdefault(pyName, []).append((target, namespaces))

    # The classes feather-tk's binding templates make for a value type,
    # named after it (ftk/CorePy/Observable*Inline.h): the template each is.
    _OBSERVABLES = [
        (r"^.+ListObserver$", "ftk::ListObserver"),
        (r"^.+MapObserver$", "ftk::MapObserver"),
        (r"^.+Observer$", "ftk::Observer"),
        (r"^IObservable.+List$", "ftk::IObservableList"),
        (r"^IObservable.+Map$", "ftk::IObservableMap"),
        (r"^IObservable.+$", "ftk::IObservable"),
        (r"^Observable.+List$", "ftk::ObservableList"),
        (r"^Observable.+Map$", "ftk::ObservableMap"),
        (r"^Observable.+$", "ftk::Observable"),
    ]

    def _binding(self, pyName, module):
        found = self.byPyName.get(pyName, [])
        if not found:
            for pattern, cppType in self._OBSERVABLES:
                if re.match(pattern, pyName) and cppType in self.headers.classes:
                    return Binding(pyName, cppType, ["ftk"])
        if len(found) > 1:
            # The same Python name in two modules: the one whose C++
            # namespaces have the module's name.
            last = module.split(".")[-1]
            for b in found:
                if last in b.namespaces or last in b.cppType:
                    return b
        return found[0] if found else None

    def classDoc(self, pyName, module):
        b = self._binding(pyName, module)
        if not b:
            return None, None
        q = _resolve(self.headers, b.cppType, b.namespaces)
        return (self.headers.classes.get(q) if q else None), (b, q)

    def memberDoc(self, ctx, pyName):
        b, q = ctx
        if not b or not q:
            return None
        names = b.members.get(pyName, [pyName])
        # The class, then the classes it derives from.
        seen = set()
        todo = [q]
        while todo:
            cls = todo.pop(0)
            if cls in seen:
                continue
            seen.add(cls)
            for name in names:
                doc = self.headers.members.get((cls, name))
                if doc:
                    return doc
            ns = cls.split("::")[:-1]
            for base in self.headers.bases.get(cls, []):
                r = _resolve(self.headers, base, ns + b.namespaces)
                if r:
                    todo.append(r)
        return None

    def functionDoc(self, pyName):
        for target, namespaces in self.functions.get(pyName, []):
            parts = target.split("::")
            name = parts[-1]
            for (scope, member), doc in self.headers.members.items():
                if member != name:
                    continue
                if len(parts) > 1 and not scope.endswith("::".join(parts[:-1])):
                    continue
                return doc
        return None


def docStub(path, docs, report):
    with open(path, encoding="utf-8") as f:
        source = f.read()
    tree = ast.parse(source)
    lines = source.split("\n")
    module = os.path.splitext(os.path.basename(path))[0]
    if module == "__init__":
        module = os.path.basename(os.path.dirname(path))
    edits = []
    counts = [0, 0]

    def note(name, doc):
        counts[1] += 1
        if doc:
            counts[0] += 1
        else:
            report.append("%s: %s" % (os.path.basename(path), name))

    def addDoc(node, doc, indent):
        # A docstring as the first thing in the body. A one-line
        # "def f(...) -> T: ..." gives up its "..." to it.
        first = node.body[0]
        if (isinstance(first, ast.Expr) and isinstance(first.value, ast.Constant) and
                first.value.value is Ellipsis):
            edits.append((first.lineno, first.col_offset, first.end_col_offset, doc, indent, True))
        else:
            # Before the first statement, and before its decorators, which
            # its line number does not count.
            line = min([first.lineno] + [d.lineno for d in getattr(first, "decorator_list", [])])
            edits.append((line, 0, 0, doc, indent, False))

    def visitClass(node, ctxName):
        doc, ctx = docs.classDoc(node.name, module)
        note(ctxName + node.name, doc)
        if doc and not _hasDocstring(node):
            addDoc(node, doc, node.col_offset + 4)
        for i, child in enumerate(node.body):
            if isinstance(child, ast.FunctionDef):
                if _isSetter(child) or (child.name.startswith("__") and child.name != "__init__"):
                    continue
                d = docs.memberDoc(ctx, child.name) if ctx else None
                if child.name == "__init__" and not d:
                    continue
                note(ctxName + node.name + "." + child.name, d)
                if d and not _hasDocstring(child):
                    addDoc(child, d, child.col_offset + 4)
            elif isinstance(child, ast.ClassDef):
                visitClass(child, ctxName + node.name + ".")
            elif isinstance(child, (ast.Assign, ast.AnnAssign)):
                target = child.targets[0] if isinstance(child, ast.Assign) else child.target
                if isinstance(target, ast.Name) and not target.id.startswith("_"):
                    d = docs.memberDoc(ctx, target.id) if ctx else None
                    nxt = node.body[i + 1] if i + 1 < len(node.body) else None
                    already = (isinstance(nxt, ast.Expr) and isinstance(nxt.value, ast.Constant) and
                               isinstance(nxt.value.value, str))
                    if d and not already:
                        # An attribute docstring: a string after it.
                        edits.append((child.end_lineno + 1, 0, 0, d, child.col_offset, "after"))

    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            visitClass(node, "")
        elif isinstance(node, ast.FunctionDef):
            d = docs.functionDoc(node.name)
            note(node.name, d)
            if d and not _hasDocstring(node):
                addDoc(node, d, node.col_offset + 4)

    # From the bottom up, so the line numbers above stay right.
    for lineno, col, endCol, doc, indent, mode in sorted(edits, key=lambda e: (e[0], e[1]), reverse=True):
        new = _docLines(doc, indent)
        if mode == "after":
            lines[lineno - 1:lineno - 1] = new
        elif mode is True:
            line = lines[lineno - 1]
            head = line[:col].rstrip()
            tail = line[endCol:]
            if head.strip():
                lines[lineno - 1:lineno] = [head] + new + ([tail] if tail.strip() else [])
            else:
                lines[lineno - 1:lineno] = new + ([tail] if tail.strip() else [])
        else:
            lines[lineno - 1:lineno - 1] = new

    out = "\n".join(lines)
    ast.parse(out)
    with open(path, "w", encoding="utf-8") as f:
        f.write(out)
    return counts


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--headers", action="append", default=[], help="A directory of headers.")
    parser.add_argument("--bindings", action="append", default=[], help="A directory of binding sources.")
    parser.add_argument("--report", help="Write what has no description to this file.")
    parser.add_argument("stubs", nargs="+")
    args = parser.parse_args()

    headers = readHeaders(args.headers)
    bindings, functions = readBindings(args.bindings)
    docs = Docs(headers, bindings, functions)
    report = []
    total = [0, 0]
    for stub in args.stubs:
        # A stub this cannot document is left as stubgen wrote it, and the
        # build goes on: descriptions are worth less than a wheel.
        try:
            c = docStub(stub, docs, report)
        except Exception as e:
            print("doc_stub.py: warning: %s is left without descriptions: %s" % (stub, e))
            continue
        total[0] += c[0]
        total[1] += c[1]
    print("doc_stub.py: %d of %d described (%d%%)" % (
        total[0], total[1], 100 * total[0] // max(1, total[1])))
    if args.report:
        with open(args.report, "w", encoding="utf-8") as f:
            f.write("\n".join(report) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
