
.PHONY: all build clean test

all: build


build:
	@python3 setup.py build_ext --inplace


clean:
	@rm -rf build *.so __pycache__ .*_cache


test:
	@python3 -m pytest
