import api

mem = {}


def test_list_init():
    mem["l"] = api.List()
    assert mem["l"] is not None
    api.bang_success()


def test_list_size():
    lst = mem["l"]
    initial_size = lst.size()
    assert initial_size == 0


def test_list_append():
    lst = mem["l"]
    obj = api.Dictionary(test=1)
    lst.append(obj)
    assert lst.size() == 1


def test_list_get():
    lst = mem["l"]
    obj = api.Dictionary(value=42)
    lst.append(obj)
    retrieved = lst.get(0)
    assert retrieved is not None


def test_list_get_size():
    lst = mem["l"]
    size = lst.get_size()
    assert size >= 0


def test_list_insert():
    lst = api.List()
    obj1 = api.Dictionary(first=1)
    obj2 = api.Dictionary(second=2)
    lst.append(obj1)
    lst.insert(0, obj2)
    assert lst.size() == 2


def test_list_delete_index():
    lst = api.List()
    obj1 = api.Dictionary(a=1)
    obj2 = api.Dictionary(b=2)
    obj3 = api.Dictionary(c=3)
    lst.append(obj1)
    lst.append(obj2)
    lst.append(obj3)
    original_size = lst.size()
    lst.delete_index(1)
    assert lst.size() == original_size - 1


def test_list_chuck_index():
    lst = api.List()
    obj1 = api.Dictionary(x=1)
    obj2 = api.Dictionary(y=2)
    lst.append(obj1)
    lst.append(obj2)
    original_size = lst.size()
    lst.chuck_index(0)
    assert lst.size() == original_size - 1


def test_list_chuck_object():
    lst = api.List()
    obj = api.Dictionary(target=999)
    lst.append(obj)
    lst.chuck_object(obj)


def test_list_delete_object():
    lst = api.List()
    obj = api.Dictionary(target=999)
    lst.append(obj)
    lst.delete_object(obj)


def test_list_clear():
    lst = api.List()
    obj1 = api.Dictionary(a=1)
    obj2 = api.Dictionary(b=2)
    lst.append(obj1)
    lst.append(obj2)
    lst.clear()
    assert lst.size() == 0


def test_list_reverse():
    lst = api.List()
    obj1 = api.Dictionary(order=1)
    obj2 = api.Dictionary(order=2)
    obj3 = api.Dictionary(order=3)
    lst.append(obj1)
    lst.append(obj2)
    lst.append(obj3)
    lst.reverse()
    assert lst.size() == 3


def test_list_rotate():
    lst = api.List()
    obj1 = api.Dictionary(pos=1)
    obj2 = api.Dictionary(pos=2)
    lst.append(obj1)
    lst.append(obj2)
    lst.rotate(1)
    assert lst.size() == 2


def test_list_shuffle():
    lst = api.List()
    for i in range(5):
        lst.append(api.Dictionary(val=i))
    lst.shuffle()
    assert lst.size() == 5


def test_list_swap():
    lst = api.List()
    obj1 = api.Dictionary(a=1)
    obj2 = api.Dictionary(b=2)
    obj3 = api.Dictionary(c=3)
    lst.append(obj1)
    lst.append(obj2)
    lst.append(obj3)
    lst.swap(0, 2)
    assert lst.size() == 3


def test_list_readonly():
    lst = api.List()
    lst.readonly(1)
    lst.readonly(0)


def test_list_flags():
    lst = api.List()
    lst.flags(0)
    flags = lst.getflags()
    assert flags is not None


def test_list_get_index_of_object():
    lst = api.List()
    obj1 = api.Dictionary(first=1)
    obj2 = api.Dictionary(second=2)
    lst.append(obj1)
    lst.append(obj2)
    index = lst.get_index_of_object(obj2)
    assert index >= 0


def test_list_chuck():
    lst = api.List()
    obj1 = api.Dictionary(a=1)
    lst.append(obj1)
    lst.chuck()
    assert lst.size() == 0
