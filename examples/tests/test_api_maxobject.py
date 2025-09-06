"""

The `api.Maxobject` extension class is a generic wrapper around `t_object` types

In this test, the global `memory` dict serves as a kind of module-scope namespace
which allows for a basic way to keep Maxobject instances alive for the duration
of the test, and can be used to manage the lifescycle of instances on a granular basis
"""

import api

mem = {}


def test_maxobject_init():
    mem["buf"] = api.MaxObject("buffer~", "buf", "jongly.aif")


def test_maxobject_classname():
    return mem["buf"].classname


def test_maxobject_isinstance_object():
    return isinstance(mem["buf"], object)


def test_maxobject_from_str():
    mem["buf"] = api.MaxObject.from_str("buffer~", "buf cello-f2.aif")


def test_maxobject_registration():
    buf1 = mem["buf"].register("buf1")
    return buf1.get_namespace_and_name()


def test_maxobject_call_replace():
    mem["buf"].call("replace", "vibes-a1.aif")


def test_maxobject_call_open():
    mem["buf"].call("open")


def test_maxobject_call_wclose():
    mem["buf"].call("wclose")


def test_maxobject_method_exists():
    if mem["buf"].method_exists("wclose"):
        api.post("wclose method exists")


def test_maxobject_help():
    mem["buf"].help()


def test_maxobject_refpage():
    mem["buf"].open_refpage()


def test_maxobject_query():
    mem["buf"].open_query()


def test_maxobject_delete():
    del mem["buf"]


def test_maxobject_add_attribute():
    mem["buf"].add_attribute("long", "part1")
    api.bang_success()


def test_maxobject_remove_attribute():
    mem["buf"].remove_attribute("part1")
    api.bang_success()


def test_maxobject_set_attr_value():
    mem["buf"].set_attr_value("part1", 500)
    api.bang_success()


def test_maxobject_get_attr_value():
    return mem["buf"].get_attr_value("part1")
