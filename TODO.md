# TODO

- [ ] copy libintl.a from /usr/local/opt/gettext/lib to ../targets/build/lib/.. for
  static linking as building it takes ages.

- [ ] fix references (still pointing to compilled locations).




## PENDING 

To consider whether to provide so many build options

perhaps py.mxo is best.

should try

- --enable-optimizations at configure
- --enable-shared
- freeing memory for PY_STATIC case
- automating sucessful build sequence
    - patch makesetup
- make a less minimal packaging option (with ssl, etc.)
- automate installation of packages
- testing fat dynamic in a package
- codesigning

### Fixing Packaging (SOLVED)

To build standalone:

- add pyjs.mxo in build script

- create a custom package with
    - javascript
    - jsextensions



## BUGS

- [ ] Workflow using the code-editor is not intuitive. Test all scenarios

- [ ] CRITICAL: attempting to reload numpy after the patcher is closed crashes Max (except when you load it through `api` module!)

- [ ] `PyLong_Check` can't pick up `numpy` numbers 

- [ ] `api` object won't reload if a patch is closed (i.e. PyFinalize) and new one opened. Requires a restart of Max. (Python bug which is being worked on).


## Features and Fixes


### Core

- [ ] enhance `call` to allow kwargs [call fn x1 x2 y1=z1 y2=z2]


### Extension

- [ ] add more api wrappers.


### Attributes & Infrastructure

- [ ] add script to install python packages and then remove the cache and `_vendor` libraries of the installer which are large.

- [ ] add set/get for attributes as appropriate to trigger actions or methods calls
      after changes (NO REASON for using this found so far)

### Testing

- [50] convert `py_coll_tester` into bpatcher that can be fed by `py_repl`

- [ ] complete comprehensive test suite
    - [ ] complete c test suite
    - [ ] complete max test suite

### Future Experiments

- [ ] Try to integrate an ipython shell somehow 




