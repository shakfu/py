import api

mem = {}


def test_atombuf_init():
    buf = api.Atombuf("abc", 1, 1.5)
    assert buf is not None
    mem["buf"] = buf
    api.bang_success()


def test_atombuf_new():
    buf = api.Atombuf.new()
    assert buf is not None
    mem["new_buf"] = buf


def test_atombuf_add_text():
    buf = api.Atombuf.new()
    buf.add_text("#N buffer~ buf drumLoop.aif")
    text = buf.to_text()
    assert text is not None
    assert isinstance(text, str)
    assert "buffer~" in text


def test_atombuf_to_text():
    buf = api.Atombuf.new()
    buf.add_text("#N buffer~ buf drumLoop.aif")
    text = buf.to_text()
    assert text is not None
    assert isinstance(text, str)
    return text


def test_atombuf_to_list():
    buf = api.Atombuf.new()
    buf.add_text("#N buffer~ buf drumLoop.aif")
    lst = buf.to_list()
    assert lst is not None
    assert isinstance(lst, list)
    assert len(lst) > 0
    return lst


def test_atombuf_multiple_add_text():
    buf = api.Atombuf.new()
    buf.add_text("#N buffer~ buf1 file1.aif")
    buf.add_text("#N buffer~ buf2 file2.aif")
    text = buf.to_text()
    assert "buf1" in text
    assert "buf2" in text


def test_atombuf_empty():
    buf = api.Atombuf.new()
    text = buf.to_text()
    lst = buf.to_list()
    # Empty atombuf should still return valid values
