import time
import api

mem = {}


def test_systhread_sleep():
    start = time.time()
    api.SysThread.sleep(10)
    elapsed = (time.time() - start) * 1000
    # Should sleep approximately 10ms
    assert elapsed >= 5


def test_systhread_self():
    current = api.SysThread.self()
    assert current is not None
    mem["current_thread"] = current


def test_systhread_ismainthread():
    is_main = api.SysThread.ismainthread()
    assert isinstance(is_main, bool)


def test_systhread_istimerthread():
    is_timer = api.SysThread.istimerthread()
    assert isinstance(is_timer, bool)


def test_systhread_isaudiothread():
    is_audio = api.SysThread.isaudiothread()
    assert isinstance(is_audio, bool)


def test_systhread_set_name():
    api.SysThread.set_name("test_thread")
    # Verify set_name can be called


def test_systhread_init():
    def thread_func():
        api.SysThread.sleep(10)
        return 42

    thread = api.SysThread(thread_func)
    assert thread is not None
    mem["thread"] = thread


def test_systhread_join():
    def thread_func():
        api.SysThread.sleep(10)
        return 100

    thread = api.SysThread(thread_func)
    result = thread.join()
    # Thread should complete


def test_systhread_detach():
    def thread_func():
        api.SysThread.sleep(10)

    thread = api.SysThread(thread_func)
    thread.detach()
    # Detach should work


def test_systhread_equal():
    current = api.SysThread.self()
    is_equal = current.equal(current)
    assert isinstance(is_equal, bool)


def test_systhread_priority():
    def thread_func():
        pass

    thread = api.SysThread(thread_func)
    thread.setpriority(5)
    priority = thread.getpriority()
    assert priority is not None


def test_systhread_terminate():
    def thread_func():
        while True:
            api.SysThread.sleep(100)

    thread = api.SysThread(thread_func)
    time.sleep(0.05)
    thread.terminate()
    # Verify terminate can be called


# SysThreadMutex tests


def test_mutex_init():
    mem["mutex"] = api.SysThreadMutex()
    assert mem["mutex"] is not None
    api.bang_success()


def test_mutex_lock_unlock():
    mutex = mem["mutex"]
    mutex.lock()
    mutex.unlock()


def test_mutex_trylock():
    mutex = api.SysThreadMutex()
    result = mutex.trylock()
    if result:
        mutex.unlock()
    assert isinstance(result, bool)


def test_mutex_context_manager():
    mutex = api.SysThreadMutex()
    with mutex:
        # Inside locked section
        pass
    # Should auto-unlock


# SysThreadCond tests


def test_cond_init():
    mem["cond"] = api.SysThreadCond()
    assert mem["cond"] is not None


def test_cond_signal():
    cond = mem["cond"]
    cond.signal()
    # Verify signal can be called


def test_cond_broadcast():
    cond = mem["cond"]
    cond.broadcast()
    # Verify broadcast can be called


def test_cond_wait():
    mutex = api.SysThreadMutex()
    cond = api.SysThreadCond()

    def waiter():
        mutex.lock()
        cond.wait(mutex)
        mutex.unlock()

    def signaler():
        api.SysThread.sleep(10)
        cond.signal()

    # Start waiter thread
    t1 = api.SysThread(waiter)
    t1.detach()

    # Start signaler thread
    t2 = api.SysThread(signaler)
    t2.join()


# SysThreadRWLock tests


def test_rwlock_init():
    mem["rwlock"] = api.SysThreadRWLock()
    assert mem["rwlock"] is not None


def test_rwlock_rdlock():
    rwlock = mem["rwlock"]
    rwlock.rdlock()
    rwlock.rdunlock()


def test_rwlock_wrlock():
    rwlock = mem["rwlock"]
    rwlock.wrlock()
    rwlock.wrunlock()


def test_rwlock_tryrdlock():
    rwlock = api.SysThreadRWLock()
    result = rwlock.tryrdlock()
    if result:
        rwlock.rdunlock()
    assert isinstance(result, bool)


def test_rwlock_trywrlock():
    rwlock = api.SysThreadRWLock()
    result = rwlock.trywrlock()
    if result:
        rwlock.wrunlock()
    assert isinstance(result, bool)


def test_rwlock_spintime():
    rwlock = api.SysThreadRWLock()
    rwlock.setspintime(100)
    spintime = rwlock.getspintime()
    assert spintime is not None


def test_rwlock_multiple_readers():
    rwlock = api.SysThreadRWLock()

    def reader():
        rwlock.rdlock()
        api.SysThread.sleep(10)
        rwlock.rdunlock()

    # Multiple readers should be able to acquire lock
    t1 = api.SysThread(reader)
    t2 = api.SysThread(reader)
    t1.detach()
    t2.detach()
    api.SysThread.sleep(50)


def test_rwlock_writer_exclusive():
    rwlock = api.SysThreadRWLock()

    def writer():
        rwlock.wrlock()
        api.SysThread.sleep(10)
        rwlock.wrunlock()

    t1 = api.SysThread(writer)
    t1.join()
