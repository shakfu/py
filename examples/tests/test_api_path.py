import api


mem = {}


def test_path_init_filename():
    mem["p"] = api.Path("test_api_path.py")
    p = mem["p"]
    api.bang_success()


def test_path_init_pathname():
    mem["p"] = api.Path("~/Downloads/max-dev")
    # mem['p'] = api.Path('~/Downloads/hello.py')
    p = mem["p"]
    api.bang_success()


def test_path_filename():
    p = mem["p"]
    return p.filename


def test_path_pathname():
    p = mem["p"]
    return p.pathname


def test_path_absolute():
    p = mem["p"]
    return p.absolute


def test_path_type():
    p = mem["p"]
    return p.type


def test_path_creator():
    p = mem["p"]
    return p.creator


def test_path_open():
    with api.Path("/tmp/test.txt").open("w") as f:
        f.write("hello world")


def test_path_maxapp_dir():
    return api.Path.maxapp_dir().pathname


def test_path_temp_dir():
    return api.Path.temp_dir().pathname


def test_path_desktop_dir():
    return api.Path.desktop_dir().pathname


def test_path_userdoc_dir():
    return api.Path.userdoc_dir().pathname


def test_path_usermax_dir():
    return api.Path.usermax_dir().pathname


def test_path_support_dir():
    return api.Path.support_dir().pathname


def test_path_default_dir():
    return api.Path.default_dir().pathname


def test_path_mod_date():
    p = mem["p"]
    return p.mod_date


def test_path_to_potential_name():
    p = mem["p"]
    return p.to_potential_name()
