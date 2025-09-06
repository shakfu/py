# https://docs.cycling74.com/max8/vignettes/messages_to_max#Sending_a_message_to_the_Max_application

import api

mem = {}


def test_max_init():
    mem["m"] = api.Max()


def test_max_midilist():
    app = mem["m"]
    app.midilist()


def test_max_clean():
    app = mem["m"]
    app.clean()


def test_max_maxwindow():
    app = mem["m"]
    app.maxwindow()


def test_max_clearmaxwindow():
    app = mem["m"]
    app.clearmaxwindow()


def test_max_paths():
    app = mem["m"]
    app.paths()


def test_max_externaleditor():
    app = mem["m"]
    app.externaleditor("Sublime Text.app")
