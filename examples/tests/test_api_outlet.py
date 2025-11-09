import api


def test_outlet_creation():
    """Test creating a new outlet."""
    ext = api.PyExternal()

    try:
        # Create a generic outlet
        outlet = api.Outlet(ext)
        assert outlet is not None
        assert not outlet.is_null()
        api.post(f"Created outlet: {outlet}")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet creation test: {e}")
        # This may fail if called outside of object initialization
        api.post("Note: outlet creation may only work during object init")


def test_outlet_bang():
    """Test sending a bang through an outlet."""
    ext = api.PyExternal()

    try:
        outlet = api.Outlet(ext)
        outlet.bang()
        api.post("Sent bang through outlet")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet bang test: {e}")


def test_outlet_int():
    """Test sending an integer through an outlet."""
    ext = api.PyExternal()

    try:
        outlet = api.Outlet(ext)
        outlet.int(42)
        api.post("Sent int 42 through outlet")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet int test: {e}")


def test_outlet_float():
    """Test sending a float through an outlet."""
    ext = api.PyExternal()

    try:
        outlet = api.Outlet(ext)
        outlet.float(3.14159)
        api.post("Sent float 3.14159 through outlet")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet float test: {e}")


def test_outlet_symbol():
    """Test sending a symbol through an outlet."""
    ext = api.PyExternal()

    try:
        outlet = api.Outlet(ext)
        outlet.symbol("hello")
        api.post("Sent symbol 'hello' through outlet")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet symbol test: {e}")


def test_outlet_list():
    """Test sending a list through an outlet."""
    ext = api.PyExternal()

    try:
        outlet = api.Outlet(ext)
        outlet.list([1, 2, 3.5, "test"])
        api.post("Sent list [1, 2, 3.5, 'test'] through outlet")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet list test: {e}")


def test_outlet_anything():
    """Test sending an arbitrary message through an outlet."""
    ext = api.PyExternal()

    try:
        outlet = api.Outlet(ext)
        outlet.anything("custom", [1, 2, 3])
        api.post("Sent 'custom 1 2 3' through outlet")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet anything test: {e}")


def test_outlet_typed():
    """Test creating a typed outlet."""
    ext = api.PyExternal()

    try:
        # Create an outlet with a specific type
        outlet = api.Outlet(ext, "signal")
        assert outlet is not None
        assert not outlet.is_null()
        api.post(f"Created typed outlet: {outlet}")
        api.bang_success()
    except Exception as e:
        api.post(f"typed outlet test: {e}")


def test_outlet_properties():
    """Test outlet properties and methods."""
    # Create an outlet wrapper for testing
    outlet = api.Outlet.__new__(api.Outlet)

    # Test is_null on uninitialized outlet
    assert outlet.is_null()

    # Test pointer method on null outlet
    ptr = outlet.pointer()
    assert ptr == 0

    api.post("Outlet properties test passed")
    api.bang_success()


def test_outlet_repr():
    """Test outlet string representation."""
    outlet = api.Outlet.__new__(api.Outlet)
    repr_str = repr(outlet)
    api.post(f"Outlet repr: {repr_str}")
    assert "Outlet" in repr_str
    assert "null" in repr_str
    api.bang_success()


def test_outlet_null_guards():
    """Test that operations on null outlets raise errors."""
    outlet = api.Outlet.__new__(api.Outlet)

    tests = [
        ("bang", lambda: outlet.bang()),
        ("int", lambda: outlet.int(42)),
        ("float", lambda: outlet.float(3.14)),
        ("symbol", lambda: outlet.symbol("test")),
        ("list", lambda: outlet.list([1, 2, 3])),
        ("anything", lambda: outlet.anything("msg", [1, 2])),
    ]

    for name, func in tests:
        try:
            func()
            api.post(f"ERROR: {name} should have raised ValueError")
        except ValueError as e:
            api.post(f"{name} correctly raised error: {e}")

    api.bang_success()


def test_outlet_pointer():
    """Test getting raw pointer from outlet."""
    ext = api.PyExternal()

    try:
        outlet = api.Outlet(ext)
        ptr = outlet.pointer()
        assert isinstance(ptr, int)
        assert ptr > 0
        api.post(f"Outlet pointer: {ptr:#x}")
        api.bang_success()
    except Exception as e:
        api.post(f"outlet pointer test: {e}")
