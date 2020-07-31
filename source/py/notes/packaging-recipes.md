# Packaging Recipes

## bin-homebrew-sys (PARTIAL SUCCESS)

```
$ otool -L py.mxo/Contents/MacOS/py

py.mxo/Contents/MacOS/py:
	/usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/Python (compatibility version 3.8.0, current version 3.8.0)
	
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.0.0)
	
	/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1673.126.0)

$ otool -L pyjs.mxo/Contents/MacOS/pyjs

pyjs.mxo/Contents/MacOS/pyjs:
	/usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/Python (compatibility version 3.8.0, current version 3.8.0)
	
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.0.0)
	
	/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1673.126.0)

```
### size

```
--- $HOME/Documents/Max 8/Packages/py/externals ----------------------------
  220.0 KiB [##########] /py.mxo
   60.0 KiB [##        ] /pyjs.mxo
```

### tests

- [x] py loads
- [x] pyjs loads
- [x] standalone build (inc. pyjs.mxo + add custom pkg of jsextensions and javascript)
- [ ] independent



## bin-homebrew-pkg (FAIL)

```

$ otool -L py.mxo/Contents/MacOS/py

py.mxo/Contents/MacOS/py:
	@loader_path/../../../../support/python3.8/libpython3.8.dylib (compatibility version 3.8.0, current version 3.8.0)
	
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.0.0)
	
	/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1673.126.0)

$ otool -L pyjs.mxo/Contents/MacOS/pyjs
pyjs.mxo/Contents/MacOS/pyjs:
	@loader_path/../../../../support/python3.8/libpython3.8.dylib (compatibility version 3.8.0, current version 3.8.0)
	
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.0.0)
	
	/System/Library/Frameworks/CoreFoundation.framework/Versions/A/CoreFoundation (compatibility version 150.0.0, current version 1673.126.0)
```

### size

```
--- $HOME/Documents/Max 8/Packages/py/externals ----------------------------
  220.0 KiB [##########] /py.mxo
   60.0 KiB [##        ] /pyjs.mxo

--- $HOME/Documents/Max 8/Packages/py/support ------------------------------
    9.0 MiB [##########] /python3.8
```

### tests

- [ ] py loads
- [ ] pyjs loads
- [ ] standalone build (inc. pyjs.mxo + add custom pkg of jsextensions and javascript)
- [ ] independent


## bin-homebrew-ext (FAIL)

- compilation errors
 
- not working in max



## bin-framework-pkg (FAIL)

- crash


## src-shared-pkg (FAIL)

- bundle error but works







