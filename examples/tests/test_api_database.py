from pathlib import Path

import api


def test_db_init():
    db_path = Path("/tmp/test_db.sqlite3")
    db = api.Database("mydb", str(db_path))
    db.open()
    db.close()
    assert db_path.exists()


def test_db_result():
    db_path = Path("/tmp/test_db.sqlite3")
    db = api.Database("mydb", str(db_path))
    db.open()
    db.query_table_new("mytable")
    result = db.query_direct("SELECT * FROM mytable")
    assert result.numrecords() == 0
    db.close()


def test_db_view():
    db_path = Path("/tmp/test_db.sqlite3")
    db = api.Database("mydb", str(db_path))
    db.open()
    db.query_table_new("mytable")
    view = db.view_create("SELECT * FROM mytable")
    assert view.sql == "SELECT * FROM mytable"
    db.close()


def test_db_view_result():
    db_path = Path("/tmp/test_db.sqlite3")
    db = api.Database("mydb", str(db_path))
    db.open()
    db.query_table_new("mytable")
    view = db.view_create("SELECT * FROM mytable")
    result = view.getresult()
    assert result.numrecords() == 0
    db.close()
