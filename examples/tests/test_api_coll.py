import api

mem = {}


def test_coll_init():
    mem["c"] = api.Coll("mycoll")
    api.bang_success()


def test_coll_bang():
    """Retrieve the next data set"""
    c = mem["c"]
    c.bang()


def test_coll_get_by_int():
    """Retrieve data by index"""
    c = mem["c"]
    c.get(2)


def test_coll_get_by_float():
    """Retrieve data by index"""
    c = mem["c"]
    c.get(2.0)


def test_coll_get_by_symbol():
    """Retrieve data by index"""
    c = mem["c"]
    c.get("abc")


def test_coll_set():
    """Store index and data"""
    c = mem["c"]
    c.set(1, 4, 1, 2.2, "nice one")


def test_coll_append():
    """Add item associated with an index"""
    c = mem["c"]
    c.append("xyz")
    api.bang_success()


def test_coll_assoc():
    """Associate a name with an index"""
    c = mem["c"]
    c.assoc("abc", 1)


def test_coll_clear():
    """Clear all data"""
    c = mem["c"]
    c.clear()


def test_coll_deassoc():
    """De-associate a name with an index"""
    c = mem["c"]
    c.deassoc("abc", 1)


def test_coll_delete():
    """Remove data and renumber"""
    c = mem["c"]
    c.delete(1)


def test_coll_dump():
    """Output all data"""
    c = mem["c"]
    c.dump()


def test_coll_end():
    """Move to last address"""
    c = mem["c"]
    c.end()


def test_coll_filetype():
    """Set the recognized file types"""
    c = mem["c"]
    c.filetype()


def test_coll_embed():
    """Set the file-save flag"""
    c = mem["c"]
    c.embed(0)


def test_coll_goto():
    """Move to an index"""
    c = mem["c"]
    c.goto(1)


def test_coll_insert():
    """Insert data at a specific address"""
    c = mem["c"]
    c.insert(0, "new insert")


def test_coll_insert2():
    """Insert data at a specific address"""
    c = mem["c"]
    c.insert2(0, "new insert2")


def test_coll_length():
    """Retrieve the number of entries"""
    c = mem["c"]
    c.length()


def test_coll_max():
    """Return the highest numeric value"""
    c = mem["c"]
    c.max()


def test_coll_merge():
    """Merge data at an existing address"""
    c = mem["c"]
    c.merge(3, "merged")


def test_coll_min():
    """Return the lowest numeric value"""
    c = mem["c"]
    c.min()


def test_coll_next():
    """Move to the next address"""
    c = mem["c"]
    c.next()


def test_coll_nstore():
    """Store data with both number and symbol index"""
    c = mem["c"]
    c.nstore(1, "aaa", 1, 2, 121.21, "good food!")


def test_coll_nsub():
    """Replace a single data element"""
    c = mem["c"]
    c.nsub(2, 2, "snubed!")


def test_coll_nth():
    """Return a single data element"""
    c = mem["c"]
    c.nth(1, 2)


def test_coll_open():
    """Open a data editing window"""
    c = mem["c"]
    c.open()


def test_coll_prev():
    """Move to the previous address"""
    c = mem["c"]
    c.prev()


def test_coll_read():
    """Choose a file to load"""
    c = mem["c"]
    name = c.name
    c.read(f"/tmp/coll-{name}.txt")


def test_coll_readagain():
    """Reload a file"""
    c = mem["c"]
    c.readagain()


def test_coll_refer():
    """Change data reference"""
    c = mem["c"]
    c.refer("mycoll2")


def test_coll_remove():
    """Remove an entry"""
    c = mem["c"]
    c.remove(1)


def test_coll_renumber():
    """Renumber entries"""
    c = mem["c"]
    c.renumber(1)


def test_coll_renumber2():
    """Increment indices by one"""
    c = mem["c"]
    c.renumber2(1)


def test_coll_separate():
    """Creates an open entry index"""
    c = mem["c"]
    c.separate(1)


def test_coll_sort():
    """Sort the data"""
    c = mem["c"]
    c.sort()


def test_coll_start():
    """Move to the first entry"""
    c = mem["c"]
    c.start()


def test_coll_store():
    """Store data at a symbolic index"""
    c = mem["c"]
    c.store("triad", 0, 4, 7)


def test_coll_sub():
    """Replace a data element, output data"""
    c = mem["c"]
    c.nsub(2, 2, "subed!")


def test_coll_subsym():
    """Changes an index symbol"""
    c = mem["c"]
    c.subsym("abd", "def")


def test_coll_swap():
    """Swap two indices"""
    c = mem["c"]
    c.swap(1, 2)


def test_coll_wclose():
    """Close the data editing window"""
    c = mem["c"]
    c.wclose()


def test_coll_write():
    """Write the data to a disk file"""
    c = mem["c"]
    name = c.name
    c.write(f"/tmp/coll-{name}.txt")


def test_coll_writeagain():
    """Rewrite a file"""
    c = mem["c"]
    c.writeagain()
