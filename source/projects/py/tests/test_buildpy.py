"""regression tests for python version support in buildpy"""
import pathlib
import sys

import pytest


@pytest.mark.parametrize("version", ["3.14.3", "3.15.2"])
def test_buildpy_supports_version(version):
    sys.path.insert(0, str(pathlib.Path(__file__).parents[3] / "scripts"))
    import buildpy
    assert buildpy.PythonBuilder(version=version).get_config() is not None


def test_buildpy_315_builds_expat_through_configure():
    sys.path.insert(0, str(pathlib.Path(__file__).parents[3] / "scripts"))
    import buildpy
    cfg = buildpy.PythonBuilder(version="3.15.2").get_config().cfg
    assert "-DXML_DEV_URANDOM" not in cfg["extensions"]["pyexpat"]
    assert "_math_integer" in cfg["shared"]
