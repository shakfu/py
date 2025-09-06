# tests top-level api functions only

import api


def test_bang():
    api.bang()


def success_bang():
    api.success_bang()


def failure_bang():
    api.failure_bang()


def test_out_sym():
    api.out("hello outlet!")


def test_out_int():
    api.out(100)


def test_out_float():
    api.out(12.75)


def test_out_list():
    api.out([1, "a", "c", 4, 5])


def test_out_dict():
    api.out({"a": [1, 2, "a"], "b": 1.3, "c": 100, "d": "e"})


def test_api_send():
    api.send("intobj", 100)


def test_lookup():
    api.lookup("mrfloat")


def test_post():
    api.post("something post")


def test_error():
    api.error("an error")
