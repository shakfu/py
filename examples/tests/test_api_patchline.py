import api


def test_patchline_from_patcher():
    """Test getting patchlines from a patcher."""
    p = api.get_patcher()
    patchline = p.get_firstline()

    if patchline is None:
        api.post("No patchlines found in patcher")
        return

    assert patchline is not None
    assert not patchline.is_null()
    api.post(f"First patchline: {patchline}")
    api.bang_success()


def test_patchline_boxes():
    """Test getting source and destination boxes from a patchline."""
    p = api.get_patcher()
    patchline = p.get_firstline()

    if patchline is None:
        api.post("No patchlines found in patcher")
        return

    box1 = patchline.get_box1()
    box2 = patchline.get_box2()

    assert box1 is not None
    assert box2 is not None

    api.post(f"Source box: {box1.classname}")
    api.post(f"Dest box: {box2.classname}")
    api.bang_success()


def test_patchline_inlets_outlets():
    """Test getting inlet/outlet numbers from a patchline."""
    p = api.get_patcher()
    patchline = p.get_firstline()

    if patchline is None:
        api.post("No patchlines found in patcher")
        return

    outlet_num = patchline.get_outletnum()
    inlet_num = patchline.get_inletnum()

    api.post(f"Outlet number: {outlet_num}")
    api.post(f"Inlet number: {inlet_num}")

    assert isinstance(outlet_num, int)
    assert isinstance(inlet_num, int)
    api.bang_success()


def test_patchline_points():
    """Test getting start and end points of a patchline."""
    p = api.get_patcher()
    patchline = p.get_firstline()

    if patchline is None:
        api.post("No patchlines found in patcher")
        return

    start = patchline.get_startpoint()
    end = patchline.get_endpoint()

    assert start is not None
    assert end is not None
    assert len(start) == 2
    assert len(end) == 2

    api.post(f"Start point: {start}")
    api.post(f"End point: {end}")
    api.bang_success()


def test_patchline_hidden():
    """Test getting and setting hidden state of a patchline."""
    p = api.get_patcher()
    patchline = p.get_firstline()

    if patchline is None:
        api.post("No patchlines found in patcher")
        return

    # Get current state
    hidden = patchline.get_hidden()
    api.post(f"Hidden state: {hidden}")

    # Toggle it
    patchline.set_hidden(not hidden)
    new_hidden = patchline.get_hidden()
    assert new_hidden == (not hidden)

    # Restore original
    patchline.set_hidden(hidden)
    final_hidden = patchline.get_hidden()
    assert final_hidden == hidden

    api.bang_success()


def test_patchline_iteration():
    """Test iterating through all patchlines in a patcher."""
    p = api.get_patcher()
    patchline = p.get_firstline()

    if patchline is None:
        api.post("No patchlines found in patcher")
        return

    count = 0
    while patchline is not None:
        count += 1
        box1 = patchline.get_box1()
        box2 = patchline.get_box2()
        api.post(f"Patchline {count}: {box1.classname} -> {box2.classname}")
        patchline = patchline.get_nextline()

    api.post(f"Total patchlines: {count}")
    assert count > 0
    api.bang_success()


def test_patchline_pointer():
    """Test getting raw pointer from patchline."""
    p = api.get_patcher()
    patchline = p.get_firstline()

    if patchline is None:
        api.post("No patchlines found in patcher")
        return

    ptr = patchline.pointer()
    assert isinstance(ptr, int)
    assert ptr > 0
    api.post(f"Patchline pointer: {ptr:#x}")
    api.bang_success()
