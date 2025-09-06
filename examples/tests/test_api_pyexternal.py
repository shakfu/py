import api

mem = {}


def test_pyexternal_init():
    ext = mem["ext"] = api.PyExternal()
    ext.out(f"{ext.name} external is initialized")


def test_pyexternal_bang():
    ext = mem["ext"]
    ext.bang()


def test_pyexternal_bang_success():
    ext = mem["ext"]
    ext.bang_success()


def test_pyexternal_bang_failure():
    ext = mem["ext"]
    ext.bang_failure()


def test_pyexternal_box():
    ext = mem["ext"]
    box = ext.get_box()
    ext.log_info(f"rec: {box.get_patching_rect()}")


def test_pyexternal_patcher():
    ext = mem["ext"]
    p = ext.get_patcher()
    return p.filename


def test_pyexternal_scan():
    ext = mem["ext"]
    ext.scan()


def test_pyexternal_lookup():
    ext = mem["ext"]
    ext.lookup("abc")


def test_pyexternal_send():
    ext = mem["ext"]
    ext.send("abc", 100)
