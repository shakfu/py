# tests top-level api functions only

import api


def test_bang():
    api.bang()
    # Verify bang can be called without error


def test_success_bang():
    api.bang_success()
    # Verify success_bang can be called without error


def test_failure_bang():
    api.bang_failure()
    # Verify failure_bang can be called without error


def test_out_sym():
    api.out("hello outlet!")
    # Verify string output works


def test_out_int():
    api.out(100)
    # Verify int output works


def test_out_float():
    api.out(12.75)
    # Verify float output works


def test_out_list():
    api.out([1, "a", "c", 4, 5])
    # Verify list output works


def test_out_dict():
    api.out({"a": [1, 2, "a"], "b": 1.3, "c": 100, "d": "e"})
    # Verify dict output works


def test_api_send():
    api.send("intobj", 100)
    # Verify send works


def test_lookup():
    result = api.lookup("mrfloat")
    # Lookup may return None if object doesn't exist


def test_post():
    api.post("something post")
    # Verify post to Max window works


def test_error():
    api.error("an error")
    # Verify error message works


def test_resources_dir():
    path = api.resources_dir()
    assert path is not None
    assert isinstance(path, str)


def test_support_dir():
    path = api.support_dir()
    assert path is not None
    assert isinstance(path, str)
