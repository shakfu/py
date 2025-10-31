import api

mem = {}


def test_atomarray_init():
    mem["aa"] = api.AtomArray()
    assert mem["aa"] is not None
    api.bang_success()


def test_atomarray_size():
    aa = mem["aa"]
    initial_size = aa.size()
    assert initial_size == 0


def test_atomarray_set_atoms():
    aa = mem["aa"]
    atom = api.Atom([1, 2.5, "hello", "world"])
    aa.set_atoms(atom)
    assert aa.size() == 4


def test_atomarray_get_atoms():
    aa = mem["aa"]
    atom = api.Atom([10, 20.5, "test"])
    aa.set_atoms(atom)
    retrieved = aa.get_atoms()
    assert retrieved is not None
    assert retrieved.size == 3


def test_atomarray_copy_atoms():
    aa = mem["aa"]
    atom = api.Atom([1, 2, 3])
    aa.set_atoms(atom)
    copied = aa.copy_atoms()
    assert copied is not None
    assert copied.size == 3


def test_atomarray_get_atom_from_index():
    aa = mem["aa"]
    atom = api.Atom([100, 200, 300])
    aa.set_atoms(atom)
    first_atom = aa.get_atom_from_index(0)
    assert first_atom is not None


def test_atomarray_append_atom():
    aa = api.AtomArray()
    atom1 = api.Atom([1, 2])
    aa.set_atoms(atom1)
    atom2 = api.Atom(3)
    aa.append_atom(atom2)
    assert aa.size() > 2


def test_atomarray_append_atoms():
    aa = api.AtomArray()
    atom1 = api.Atom([1, 2])
    aa.set_atoms(atom1)
    atom2 = api.Atom([3, 4, 5])
    aa.append_atoms(atom2)
    assert aa.size() >= 5


def test_atomarray_duplicate():
    aa = api.AtomArray()
    atom = api.Atom([1, 2, 3])
    aa.set_atoms(atom)
    dup = aa.duplicate()
    assert dup is not None
    assert dup.size() == aa.size()


def test_atomarray_clone():
    aa = api.AtomArray()
    atom = api.Atom([1, 2, 3])
    aa.set_atoms(atom)
    clone = aa.clone()
    assert clone is not None
    assert clone.size() == aa.size()


def test_atomarray_chuck_index():
    aa = api.AtomArray()
    atom = api.Atom([1, 2, 3, 4, 5])
    aa.set_atoms(atom)
    original_size = aa.size()
    aa.chuck_index(2)
    assert aa.size() == original_size - 1


def test_atomarray_clear():
    aa = api.AtomArray()
    atom = api.Atom([1, 2, 3])
    aa.set_atoms(atom)
    aa.clear()
    assert aa.size() == 0


def test_atomarray_flags():
    aa = api.AtomArray()
    aa.set_flags(0)
    flags = aa.get_flags()
    assert flags is not None
