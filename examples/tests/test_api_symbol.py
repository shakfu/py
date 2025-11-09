import api


def test_symbol_creation():
    """Test creating a symbol."""
    sym = api.Symbol("hello")
    assert sym is not None
    assert not sym.is_null()
    assert sym.name == "hello"
    api.post(f"Created symbol: {sym}")
    api.bang_success()


def test_symbol_empty():
    """Test creating an empty symbol."""
    sym = api.Symbol()
    assert sym is not None
    assert not sym.is_null()
    assert sym.name == ""
    api.post(f"Empty symbol: {sym}")
    api.bang_success()


def test_symbol_gensym():
    """Test the gensym function."""
    sym = api.gensym("world")
    assert sym is not None
    assert not sym.is_null()
    assert sym.name == "world"
    api.post(f"gensym result: {sym}")
    api.bang_success()


def test_symbol_interning():
    """Test that symbols are interned (same name = same object)."""
    sym1 = api.Symbol("test")
    sym2 = api.Symbol("test")
    sym3 = api.Symbol("other")

    # Same name should have same pointer
    assert sym1.pointer() == sym2.pointer()
    # Different name should have different pointer
    assert sym1.pointer() != sym3.pointer()

    api.post(f"sym1 pointer: {sym1.pointer():#x}")
    api.post(f"sym2 pointer: {sym2.pointer():#x}")
    api.post(f"sym3 pointer: {sym3.pointer():#x}")
    api.bang_success()


def test_symbol_equality():
    """Test symbol equality comparison."""
    sym1 = api.Symbol("foo")
    sym2 = api.Symbol("foo")
    sym3 = api.Symbol("bar")

    # Same symbols should be equal
    assert sym1 == sym2
    # Different symbols should not be equal
    assert not (sym1 == sym3)

    api.post("Symbol equality tests passed")
    api.bang_success()


def test_symbol_string_comparison():
    """Test comparing symbols with strings."""
    sym = api.Symbol("test")

    # Should equal the same string
    assert sym == "test"
    # Should not equal different string
    assert not (sym == "other")

    api.post("Symbol-string comparison tests passed")
    api.bang_success()


def test_symbol_str():
    """Test converting symbol to string."""
    sym = api.Symbol("mystring")
    str_result = str(sym)

    assert str_result == "mystring"
    assert isinstance(str_result, str)

    api.post(f"str(symbol): {str_result}")
    api.bang_success()


def test_symbol_repr():
    """Test symbol representation."""
    sym = api.Symbol("test")
    repr_str = repr(sym)

    api.post(f"Symbol repr: {repr_str}")
    assert "Symbol" in repr_str
    assert "test" in repr_str
    api.bang_success()


def test_symbol_hash():
    """Test that symbols can be hashed (used as dict keys)."""
    sym1 = api.Symbol("key1")
    sym2 = api.Symbol("key2")
    sym3 = api.Symbol("key1")  # Same as sym1

    # Create a dict with symbol keys
    d = {sym1: "value1", sym2: "value2"}

    # Should be able to look up with the same symbol
    assert d[sym1] == "value1"
    assert d[sym2] == "value2"

    # Same-name symbol should access same value
    assert d[sym3] == "value1"

    api.post("Symbol hashing tests passed")
    api.bang_success()


def test_symbol_pointer():
    """Test getting raw pointer from symbol."""
    sym = api.Symbol("pointer_test")
    ptr = sym.pointer()

    assert isinstance(ptr, int)
    assert ptr > 0

    api.post(f"Symbol pointer: {ptr:#x}")
    api.bang_success()


def test_symbol_name_property():
    """Test the name property."""
    sym = api.Symbol("property_test")
    name = sym.name

    assert name == "property_test"
    assert isinstance(name, str)

    api.post(f"Symbol name: {name}")
    api.bang_success()


def test_symbol_special_chars():
    """Test symbols with special characters."""
    symbols = [
        "with-dashes",
        "with_underscores",
        "with.dots",
        "with/slashes",
        "123numbers",
        "MixedCase",
    ]

    for name in symbols:
        sym = api.Symbol(name)
        assert sym.name == name
        api.post(f"Symbol '{name}': {sym}")

    api.bang_success()


def test_symbol_inequality():
    """Test symbol inequality with non-symbol types."""
    sym = api.Symbol("test")

    # Should not equal numbers
    assert not (sym == 42)
    assert not (sym == 3.14)

    # Should not equal None
    assert not (sym == None)

    # Should not equal lists
    assert not (sym == [])

    api.post("Symbol inequality tests passed")
    api.bang_success()


def test_symbol_common_max_symbols():
    """Test creating common Max symbols."""
    common = ["bang", "int", "float", "list", "symbol", "anything"]

    for name in common:
        sym = api.Symbol(name)
        assert sym.name == name
        api.post(f"Max symbol: {sym}")

    api.bang_success()


def test_symbol_interning_gensym():
    """Test that Symbol() and gensym() return same interned object."""
    sym1 = api.Symbol("intern_test")
    sym2 = api.gensym("intern_test")

    assert sym1.pointer() == sym2.pointer()
    assert sym1 == sym2

    api.post("Symbol and gensym interning verified")
    api.bang_success()
