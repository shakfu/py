#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate api.pyi, the type stub for the `api` built-in module, from api.pyx.

`api` is compiled into the py external, so language servers have no source to
read. The stub gives them its Python-visible classes, functions and constants.

usage:
    gen_api_stub.py [--check] [api.pyx] [api.pyi]

--check exits 1 when the stub differs from what api.pyx generates, and 77
(the ctest skip code) when Cython is not installed. Requires Cython (any 3.x).
"""

import argparse
import ast
import inspect
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
SKIP = 77

PY_TYPES = {
    "bool", "bytearray", "bytes", "complex", "dict", "float", "frozenset",
    "int", "list", "object", "set", "str", "tuple",
}
ALIASES = {"unicode": "str", "basestring": "str", "bint": "bool"}
FLOATS = {"float", "double"}
HIDDEN = {"__cinit__", "__dealloc__"}


class Stub:
    def __init__(self, tree, nodes):
        self.N = nodes
        self.classes = {
            s.class_name for s in tree.body.stats if isinstance(s, nodes.CClassDefNode)
        }
        self.out = []

    # -- types

    def ctype(self, base, declarator):
        """Python type for a Cython base type + declarator, or None if unknown."""
        N = self.N
        if not isinstance(declarator, N.CNameDeclaratorNode):
            return "Any"  # pointer, array, function pointer
        if isinstance(base, N.TemplatedTypeNode):
            return "Any"  # memoryview or C++ template
        if not isinstance(base, N.CSimpleBaseTypeNode) or base.name is None:
            return None
        if base.module_path:
            return "Any"  # a type from an api_*.pxd
        name = ALIASES.get(base.name, base.name)
        if base.is_basic_c_type:
            if name == "void":
                return "None"
            return "float" if name in FLOATS else ("bool" if name == "bool" else "int")
        if name in PY_TYPES or name in self.classes:
            return name
        return "Any"

    def arg(self, a):
        decl = a.declarator
        while not isinstance(decl, self.N.CNameDeclaratorNode):
            decl = decl.base
        name = decl.name
        if not name and isinstance(a.base_type, self.N.CSimpleBaseTypeNode):
            # an untyped C-function argument parses as a type with no name
            return a.base_type.name, a.base_type.name
        if a.annotation is not None:
            t = a.annotation.string.value
        else:
            t = self.ctype(a.base_type, a.declarator)
        default = a.default is not None or decl.default is not None
        if t and t != "Any" and (a.or_none or (default and is_none(a.default or decl.default))):
            t += " | None"
        s = name + (": " + t if t else "")
        if default:
            s += " = ..." if t else "=..."
        return name, s

    def signature(self, args, star=None, starstar=None, kwonly=0):
        parts = [self.arg(a)[1] for a in args]
        if kwonly:
            pos = len(parts) - kwonly
            parts.insert(pos, "*" + (self.star(star) if star else ""))
        elif star:
            parts.append("*" + self.star(star))
        if starstar:
            parts.append("**" + self.star(starstar))
        return ", ".join(parts)

    def star(self, a):
        t = a.annotation.string.value if getattr(a, "annotation", None) else None
        return a.name + (": " + t if t else "")

    # -- emitters

    def doc(self, text, indent):
        if not text:
            return
        body = inspect.cleandoc(str(text)).replace("\\", "\\\\").replace('"""', '\\"\\"\\"')
        body = body.encode("ascii", "backslashreplace").decode()  # ASCII stub, same docstring
        lines = body.split("\n")
        pad = " " * indent
        if len(lines) == 1:
            self.out.append(f'{pad}"""{lines[0]}"""')
            return
        self.out.append(f'{pad}"""{lines[0]}')
        self.out.extend((pad + l).rstrip() for l in lines[1:])
        self.out.append(f'{pad}"""')

    def body(self, doc, indent):
        if doc:
            self.doc(doc, indent + 4)
            self.out.append(" " * (indent + 4) + "...")
        else:
            self.out[-1] += " ..."

    def decorators(self, node, indent):
        names = []
        for d in node.decorators or []:
            e = d.decorator
            if isinstance(e, self.N_expr.NameNode) and e.name in ("property", "staticmethod", "classmethod"):
                names.append(e.name)
            elif isinstance(e, self.N_expr.AttributeNode) and e.attribute in ("setter", "deleter"):
                names.append(f"{e.obj.name}.{e.attribute}")
        for n in names:
            self.out.append(" " * indent + "@" + n)
        return names

    def defnode(self, n, indent):
        if n.name in HIDDEN:
            return
        self.decorators(n, indent)
        ret = n.return_type_annotation.string.value if n.return_type_annotation else None
        sig = self.signature(n.args, n.star_arg, n.starstar_arg, n.num_kwonly_args)
        self.out.append(" " * indent + f"def {n.name}({sig})" + (f" -> {ret}" if ret else "") + ":")
        self.body(n.doc, indent)

    def cpdef(self, n, indent):
        decl = n.declarator
        while not isinstance(decl, self.N.CFuncDeclaratorNode):
            decl = decl.base
        name = decl.base.name
        ret = self.ctype(n.base_type, decl.base)
        args = list(decl.args)
        if indent and not any(isinstance(d.decorator, self.N_expr.NameNode)
                              and d.decorator.name == "staticmethod"
                              for d in getattr(n, "decorators", None) or []):
            self_arg = "self"
            if args and self.arg(args[0])[0] == "self":
                args = args[1:]
        else:
            self_arg = None
        self.decorators(n, indent)
        sig = self.signature(args)
        if self_arg:
            sig = "self" + (", " + sig if sig else "")
        self.out.append(" " * indent + f"def {name}({sig})" + (f" -> {ret}" if ret else "") + ":")
        self.body(n.doc, indent)

    def attrs(self, n, indent):
        for d in n.declarators:
            name = d.name if isinstance(d, self.N.CNameDeclaratorNode) else None
            if name is None:
                continue
            t = self.ctype(n.base_type, d) or "Any"
            pad = " " * indent
            if n.visibility == "readonly":
                self.out += [f"{pad}@property", f"{pad}def {name}(self) -> {t}: ..."]
            else:
                self.out.append(f"{pad}{name}: {t}")

    def cclass(self, n):
        bases = [b.name for b in n.bases.args] if n.bases else []
        self.out += ["", ""]
        self.out.append(f"class {n.class_name}" + (f"({', '.join(bases)})" if bases else "") + ":")
        start = len(self.out)
        self.doc(n.doc, 4)
        for s in flat(n.body, self.N):
            if isinstance(s, self.N.CVarDefNode) and s.visibility in ("public", "readonly"):
                self.attrs(s, 4)
        for s in flat(n.body, self.N):
            if isinstance(s, self.N.DefNode):
                self.defnode(s, 4)
            elif isinstance(s, self.N.CFuncDefNode) and s.overridable:
                self.cpdef(s, 4)
        if len(self.out) == start:
            self.out.append("    ...")

    def assignment(self, n):
        E = self.N_expr
        if not isinstance(n.lhs, E.NameNode):
            return
        rhs = n.rhs
        if (isinstance(rhs, E.SimpleCallNode) and isinstance(rhs.function, E.NameNode)
                and rhs.function.name == "namedtuple" and len(rhs.args) == 2):
            fields = [f.value for f in rhs.args[1].args]
            self.out += ["", "", f"class {n.lhs.name}(NamedTuple):"]
            self.out += [f"    {f}: Any" for f in fields]
            return
        t = {E.IntNode: "int", E.FloatNode: "float", E.UnicodeNode: "str",
             E.BytesNode: "bytes", E.BoolNode: "bool"}.get(type(rhs), "Any")
        self.out.append(f"{n.lhs.name}: {t}")

    def module(self, tree, doc):
        N = self.N
        self.out += [
            "# Generated by scripts/gen_api_stub.py from api.pyx. Do not edit.",
            "# SPDX-License-Identifier: MIT",
        ]
        self.doc(doc, 0)
        self.out += ["", "import pathlib", "from typing import Any, NamedTuple, Optional", ""]
        for s in tree.body.stats:
            if isinstance(s, N.CEnumDefNode) and s.create_wrapper:
                if s.name:
                    self.out += ["", "", f"class {s.name}(int): ..."]
                for item in s.items:
                    self.out.append(f"{item.name}: int")
            elif isinstance(s, N.SingleAssignmentNode):
                self.assignment(s)
        for s in tree.body.stats:
            if isinstance(s, N.DefNode):
                self.out += ["", ""]
                self.defnode(s, 0)
            elif isinstance(s, N.CFuncDefNode) and s.overridable:
                self.out += ["", ""]
                self.cpdef(s, 0)
            elif isinstance(s, N.CClassDefNode):
                self.cclass(s)
        return "\n".join(self.out) + "\n"


def flat(node, N):
    if isinstance(node, N.StatListNode):
        for s in node.stats:
            yield from flat(s, N)
    else:
        yield node


def is_none(node):
    return type(node).__name__ == "NoneNode"


def generate(pyx: Path) -> str:
    from Cython.Compiler import ExprNodes, Nodes
    from Cython.Compiler.TreeFragment import parse_from_strings

    source = pyx.read_text()
    tree = parse_from_strings("api", source)
    stub = Stub(tree, Nodes)
    stub.N_expr = ExprNodes
    text = stub.module(tree, module_doc(source))
    ast.parse(text)  # the stub must be valid Python
    return text


def module_doc(source: str):
    """First string literal of the file: Cython keeps no module doc on the tree."""
    s = source.lstrip()
    if s.startswith('"""'):
        end = s.find('"""', 3)
        if end > 0:
            return s[3:end]
    return None


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--check", action="store_true", help="exit 1 if the stub is stale")
    p.add_argument("pyx", nargs="?", type=Path, default=HERE.parent / "api.pyx")
    p.add_argument("pyi", nargs="?", type=Path, default=HERE.parent / "api.pyi")
    a = p.parse_args()
    try:
        import Cython  # noqa: F401
    except ImportError:
        print("gen_api_stub: Cython is not installed", file=sys.stderr)
        return SKIP if a.check else 1
    text = generate(a.pyx)
    if a.check:
        if not a.pyi.exists() or a.pyi.read_text() != text:
            print(f"gen_api_stub: {a.pyi} is stale; run scripts/gen_api_stub.py", file=sys.stderr)
            return 1
        return 0
    a.pyi.write_text(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
