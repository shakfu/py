import api


def test_atombuf_init():
    buf = api.Atombuf("abc", 1, 1.5)
    api.bang_success()


def test_atombuf_add_text():
    buf = api.Atombuf.new()
    buf.add_text("#N buffer~ buf drumLoop.aif")
    return buf.to_text()


def test_atombuf_to_text():
    buf = api.Atombuf.new()
    buf.add_text("#N buffer~ buf drumLoop.aif")
    return buf.to_text()
    # api.post(buf.to_text())


def test_atombuf_to_list():
    buf = api.Atombuf.new()
    buf.add_text("#N buffer~ buf drumLoop.aif")
    return buf.to_text()
    # api.post(str(buf.to_list()))
