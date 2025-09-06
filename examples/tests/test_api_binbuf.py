"""
binbufs are curious, they are in both max and pd as a way of saving objects
to file. This wsa the old way of saving pre-Max 5?

see: http://peabody.sapp.org/class/dmp2/read/WritingMax_MSPExternals.pdf

max v2;
#N vpatcher 50 38 450 338;
#P number 138 67 35 0;
#P number 98 67 35 0;
#P number 98 168 35 0;
#P newex 98 127 30 12 + 100;
#P connect 0 0 1 0;
#P connect 3 0 0 1;
#P connect 2 0 0 0;
#P pop;
"""

eg = """\
max v2;
#N vpatcher 50 38 450 338;
#P number 138 67 35 0;
#P number 98 67 35 0;
#P number 98 168 35 0;
#P newex 98 127 30 12 + 100;
#P connect 0 0 1 0;
#P connect 3 0 0 1;
#P connect 2 0 0 0;
#P pop;
"""


import api


def test_binbuf_init():
    buf = api.Binbuf()


def test_binbuf_append():
    buf = api.Binbuf()
    buf.append("#N", "buffer~", "buf", "cello-f2.aif")
    buf.eval()


def test_binbuf_insert():
    buf = api.Binbuf()
    buf.insert("#N", "buffer~", "buf", "vibes-a1.aif")
    buf.eval()


def test_binbuf_add_text():
    buf = api.Binbuf()
    buf.add_text("#N buffer~ buf drumLoop.aif")
    buf.eval()


def test_binbuf_to_text():
    buf = api.Binbuf()
    buf.add_text("#N buffer~ buf jongly.aif")
    api.post(buf.to_text())


def test_binbuf_add_textblock():
    # does nothing but 'Max > File > New from Clipboard' creates
    # a new patcher window with connected objects!!
    buf = api.Binbuf()
    buf.add_text(eg)
    buf.eval()


def test_binbuf_new_from_clipboard():
    buf = api.Binbuf()
    buf.new_from_clipboard(eg)
