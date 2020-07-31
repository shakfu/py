# Snippets


## Locate path from string

- depends on specifying FOUR_CHAR_CODE


```c
void py_locate_path_from_string(t_py* x, char* s)
{
    char filename[MAX_PATH_CHARS];
    char pathname[MAX_PATH_CHARS];
    short path;
    t_fourcc type = FOUR_CHAR_CODE('TEXT');
    t_filehandle fh;
    t_max_err err;

    strncpy_zero(filename, s, MAX_PATH_CHARS);
    if (locatefile_extended(filename, &path, &type, &type, 1)) {
        // nozero: not found
        py_error(x, "can't find file %s", s);
        return;
    } else {
        pathname[0] = 0;
        err = path_toabsolutesystempath(path, filename, pathname);
        if (err != MAX_ERR_NONE) {
            py_error(x, "can't convert %s to absolutepath", s);
            return;
        }
        py_log(x, "full path is: %s", pathname);
    }
}
```

use it:

```c
py_locate_path_from_string(x, "file.py");
```
