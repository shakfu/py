# py_code licensing

`py_code.mxo` is distributed under the GNU General Public License v3.0 only
(GPL-3.0-only), because most of its sources derive from `sc_code` in
[gus74vv/sc3-tilde](https://github.com/gus74vv/sc3-tilde), by Gustavo,
which is GPL-3.0-only. The full license text is in that repository's
`LICENSE`, and at <https://www.gnu.org/licenses/gpl-3.0.txt>.

This directory's license differs from the MIT license of the rest of this
repository. Each file states its own license in its SPDX header:

| Files | License | Origin |
|-|-|-|
| `py_code.cpp`, `py_blink.*`, `py_buffer.*`, `py_clipboard.*`, `py_complete.*`, `py_filedialog.*`, `py_path.*`, `py_scroll.*`, `py_snip.*`, `py_theme.*`, `py_wrap.*` | GPL-3.0-only | derived from `sc_code` |
| `py_syntax.*`, `tests/*` | MIT | new |
| `thirdparty/tree-sitter/` | MIT | [tree-sitter](https://github.com/tree-sitter/tree-sitter) |
| `thirdparty/tree-sitter-python/` | MIT | [tree-sitter-python](https://github.com/tree-sitter/tree-sitter-python) 0.25.0 |

MIT code may be combined into a GPL-3.0 binary; the reverse does not hold.
