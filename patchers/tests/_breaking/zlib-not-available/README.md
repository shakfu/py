# The Zlib Not Available Critical Bug

## The Issue

Upon opening the `py.maxhelp` file, one would get an immediate error, which would prevent `py` from importing any modules:

```text
py: [ERROR] (bob) failed to initialize python builtins: ZipImportError("can't decompress data; zlib not available")
```

## Occurred In

The bug only occurred in almost all dynamically-linked build `py` build variants with the exception of `shared-tiny-ext`:

Did not occur with

- [x] `make` or` make default`
- [x] `make projects`
- [x] `make framework-pkg`
- [x] `make homebrew-pkg`
- [x] `make shared-pkg`
- [x] `make shared-tiny-ext`
- [x] `make static-ext`
- [x] `make static-tiny-ext`
- [x] `make relocatable-pkg`

Occurred with

- [ ] `make framework-ext`
- [ ] `make homebrew-ext`
- [ ] `make shared-ext`

Worked after fix

- [x] `make framework-ext`
- [x] `make homebrew-ext`
- [x] `make shared-ext`

## The Fix

After testing, it looked like this issue was specific to dynamically linked build variants, but after checking the build configuration of the failing builds, it turned out they were correctly building the zlib dependency and included an `-lz` ldflag.

After some testing, it was discovered that the issue only occurred with the `py.maxhelp file`. The final fix was to remove the `[js]` and `[v8]` objects which were recently added show comparative performance between `py`, `js` and `v8`. The removal of both these objects solved the problem.

## Why did it happen?

Possible causes which need to be validated. 

- The problem is duplicated on Max8 in x86 so it's not recently introduced by Max 9

- Insufficient guards in the initialization code.

- The complexity of the file and addition of the `[js]` and `[v8]` objects, with the latter having a higher thread priority may have resulted in sequencing issues.

- Too many sends and receives?

- Nonetheless, a simple patcher page with the `py`, `js` and `v8` objects did not trigger the problem, so it is likely due to patch complexity and the some sequencing issues due to threading / prioritization.



