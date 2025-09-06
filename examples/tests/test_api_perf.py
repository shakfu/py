import time

import api


def benchmark1():
    LOOP_ITERATIONS = 1000
    ARRAY_SIZE = 4096

    data = []
    for i in range(ARRAY_SIZE):
        data.append(74)

    api.post("starting benchmark1")
    starttime = time.time()
    val = 0

    for i in range(LOOP_ITERATIONS):
        for j in range(ARRAY_SIZE):
            val += data[j]

    elapsedtime = round((time.time() - starttime) * 1000)  # ms
    api.post(f"benchmark results: {LOOP_ITERATIONS} iterations in {elapsedtime} ms")
    api.post(f"arrray accumulation is: {val}")
    api.post("compare with js in 170ms and v8 in 27ms (pre-jit)")
