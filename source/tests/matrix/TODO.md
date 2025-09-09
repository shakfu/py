
## Optional: expose a NumPy‑style constructor

If you want to initialise from an existing NumPy array:

```cython
cpdef Matrix3D.from_numpy(np.ndarray[np.double_t, ndim=3] arr):
    cdef Matrix m = Matrix(arr.shape[0], arr.shape[1], arr.shape[2])
    cdef Py_ssize_t i,j,k, offset
    for i in range(arr.shape[0]):
        for j in range(arr.shape[1]):
            for k in range(arr.shape[2]):
                offset = (i*arr.strides[0] + j*arr.strides[1] +
                          k*arr.strides[2]) // sizeof(double)
                m.m.data[offset] = arr[i,j,k]
    return m
```

