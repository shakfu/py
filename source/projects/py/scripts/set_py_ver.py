import os
import platform

env_file = os.getenv('GITHUB_ENV')
#env_file = 'env.log'

python_version = platform.python_version()

if env_file:
    with open(env_file, "a") as fopen:
        fopen.write(f"PY_VERSION={python_version}")
else:
    raise KeyError("'GITHUB_ENV' env var not set")
