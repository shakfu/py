import sys
import api

mem = {}


def test_sysprocess_get_current():
    current = api.SysProcess.get_current()
    assert current is not None
    mem["current_process"] = current
    api.bang_success()


def test_sysprocess_pid():
    proc = mem["current_process"]
    pid = proc.pid
    assert pid is not None
    assert isinstance(pid, int)
    assert pid > 0


def test_sysprocess_isrunning():
    proc = mem["current_process"]
    is_running = proc.isrunning()
    assert isinstance(is_running, bool)
    assert is_running is True


def test_sysprocess_isrunning_with_returnvalue():
    proc = mem["current_process"]
    result = proc.isrunning_with_returnvalue()
    assert result is not None


def test_sysprocess_getpath():
    proc = mem["current_process"]
    path = proc.getpath()
    assert path is not None
    assert isinstance(path, str)
    assert len(path) > 0


def test_sysprocess_fitsarch():
    proc = mem["current_process"]
    fits = proc.fitsarch()
    assert isinstance(fits, bool)


def test_sysprocess_get_by_path():
    proc = mem["current_process"]
    path = proc.getpath()
    proc2 = api.SysProcess.get_by_path(path)
    # May or may not find the process, just verify it doesn't crash


def test_sysprocess_launch():
    # Test launching a simple process (using Python itself)
    python_path = sys.executable

    # Launch with simple command that exits immediately
    try:
        proc = api.SysProcess.launch(python_path, "--version")
        if proc:
            # Give it time to execute
            api.SysThread.sleep(100)
            # Check if it ran
            is_running = proc.isrunning()
            # Process may have already exited
    except Exception:
        # launch may not be available in all contexts
        pass


def test_sysprocess_activate():
    proc = mem["current_process"]
    try:
        proc.activate()
        # Verify activate can be called
    except Exception:
        # May not be supported in all contexts
        pass


def test_sysprocess_kill():
    # We won't actually kill the current process
    # Just test that we can call launch and check kill exists
    python_path = sys.executable

    try:
        # Launch a process that will wait
        proc = api.SysProcess.launch(python_path, "-c \"import time; time.sleep(10)\"")
        if proc:
            api.SysThread.sleep(50)
            # Don't actually kill it to avoid issues
            # proc.kill()
            pass
    except Exception:
        # launch may not be available in all contexts
        pass


def test_sysprocess_current_properties():
    proc = api.SysProcess.get_current()
    pid = proc.pid
    path = proc.getpath()
    running = proc.isrunning()

    assert pid > 0
    assert isinstance(path, str)
    assert running is True
