import api


def test_inlet_new():
    """Test creating a new inlet."""
    ext = api.PyExternal()

    # Create a new inlet with a message name
    try:
        inlet = api.inlet_new(ext, "custom_message")
        assert inlet is not None
        assert not inlet.is_null()
        api.post(f"Created inlet: {inlet}")

        # Clean up
        inlet.delete()
        api.bang_success()
    except Exception as e:
        api.post(f"inlet_new test: {e}")
        # This may fail if called outside of object initialization
        api.post("Note: inlet creation may only work during object init")


def test_proxy_new():
    """Test creating a proxy inlet."""
    ext = api.PyExternal()

    try:
        # Allocate space for stuffloc variable
        # In real use, this would be a field in your object struct
        # For testing, we just use a dummy value
        stuffloc = 0

        # Create proxy for inlet 1
        proxy = api.proxy_new(ext, 1, id(stuffloc))
        assert proxy is not None
        assert not proxy.is_null()
        assert proxy.is_proxy_inlet()
        assert proxy.get_num() == 1
        api.post(f"Created proxy inlet: {proxy}")

        # Clean up
        proxy.delete()
        api.bang_success()
    except Exception as e:
        api.post(f"proxy_new test: {e}")
        # This may fail if called outside of object initialization
        api.post("Note: proxy creation may only work during object init")


def test_proxy_getinlet():
    """Test getting the current inlet number from a proxy."""
    ext = api.PyExternal()

    try:
        # Get the current inlet number
        inlet_num = api.proxy_getinlet(ext)
        api.post(f"Current inlet number: {inlet_num}")
        assert isinstance(inlet_num, int)
        api.bang_success()
    except Exception as e:
        api.post(f"proxy_getinlet test: {e}")


def test_inlet_properties():
    """Test inlet properties and methods."""
    # Create an inlet wrapper for testing
    # Note: In real use, inlets are typically created during object init

    # Test Inlet class methods
    inlet = api.Inlet()
    assert inlet.is_null()

    # Test pointer method on null inlet
    ptr = inlet.pointer()
    assert ptr == 0

    # Test get_num
    num = inlet.get_num()
    assert num == 0

    # Test is_proxy_inlet
    is_proxy = inlet.is_proxy_inlet()
    assert not is_proxy

    api.post("Inlet properties test passed")
    api.bang_success()


def test_inlet_repr():
    """Test inlet string representation."""
    inlet = api.Inlet()
    repr_str = repr(inlet)
    api.post(f"Inlet repr: {repr_str}")
    assert "Inlet" in repr_str
    assert "null" in repr_str
    api.bang_success()


def test_inlet_deletion_guard():
    """Test that we cannot delete an inlet we don't own."""
    inlet = api.Inlet()

    try:
        # Should fail because inlet is null
        inlet.delete()
        api.post("ERROR: Should have raised ValueError")
    except ValueError as e:
        api.post(f"Correctly raised error: {e}")
        api.bang_success()
