from libc.stdlib cimport malloc, calloc, free

ctypedef struct matrix_2d:
    double *data
    int rows
    int cols


ctypedef struct matrix_3d:
    double *data
    int rows
    int cols
    int planes


cdef class Matrix2D:
    """A 2D row-major matrix with contiguous data storage"""

    cdef matrix_2d *m
    cdef bint owner

    def __cinit__(self):
        self.m = NULL
        self.owner = False

    def __dealloc__(self):
        if self.m is not NULL and self.owner:
            free(self.m)

    def __init__(self, int rows, int cols):
        self.m = <matrix_2d *>malloc(sizeof(matrix_2d))
        if self.m is NULL:
            raise MemoryError()
        self.m.rows = rows
        self.m.cols = cols
        self.m.data = <double *>calloc(rows * cols, sizeof(double))
        self.owner = True

    def __getbuffer__(self, Py_buffer *view, int flags):
        if self.m.data == NULL:
            raise ValueError("uninitialized matrix")
        view.buf = <void *>self.m.data
        view.len = self.m.rows * self.m.cols * sizeof(double)
        view.itemsize = sizeof(double)

        # 2‑D shape & strides
        cdef int ndim = 2
        cdef Py_ssize_t* shape = <Py_ssize_t *>malloc(ndim * sizeof(Py_ssize_t))
        cdef Py_ssize_t* strides = <Py_ssize_t *>malloc(ndim * sizeof(Py_ssize_t))

        shape[0] = self.m.rows
        shape[1] = self.m.cols

        # C‑contiguous layout: last index changes fastest
        strides[1] = view.itemsize
        strides[0] = strides[1]*self.m.cols

        view.shape = shape
        view.strides = strides
        view.suboffsets = NULL  # contiguous
        view.ndim = ndim
        view.format = "d"          # double
        view.readonly = False
        view.internal = NULL

    def __releasebuffer__(self, Py_buffer *view):
        free(view.shape)
        free(view.strides)

    def __repr__(self):
        cdef list rows = []
        cdef list row
        cdef int i, j
        for i in range(self.m.rows):
            row = []
            for j in range(self.m.cols):
                row.append(self.get(i, j))
            rows.append(row)
        return f"Matrix2D({self.m.rows}x{self.m.cols}, {rows})"

    def __getitem__(self, tuple indices):
        if len(indices) != 2:
            raise IndexError("Matrix2D requires exactly 2 indices")
        return self.get(indices[0], indices[1])

    def __setitem__(self, tuple indices, double value):
        if len(indices) != 2:
            raise IndexError("Matrix2D requires exactly 2 indices")
        self.set(indices[0], indices[1], value)

    def __add__(self, Matrix2D other):
        if self.m.rows != other.m.rows or self.m.cols != other.m.cols:
            raise ValueError("Matrix dimensions must match for addition")
        return self.apply(lambda self_val, other_val, i, j: self_val + other_val, other)

    def __sub__(self, Matrix2D other):
        if self.m.rows != other.m.rows or self.m.cols != other.m.cols:
            raise ValueError("Matrix dimensions must match for subtraction")
        return self.apply(lambda self_val, other_val, i, j: self_val - other_val, other)

    def __mul__(self, double scalar):
        return self.apply(lambda self_val, other_val, i, j: self_val * scalar)

    def __matmul__(self, Matrix2D other):
        if self.m.cols != other.m.rows:
            raise ValueError("Matrix dimensions incompatible for multiplication")
        
        cdef Matrix2D result = Matrix2D(self.m.rows, other.m.cols)
        cdef int i, j, k
        cdef double sum_val
        for i in range(self.m.rows):
            for j in range(other.m.cols):
                sum_val = 0.0
                for k in range(self.m.cols):
                    sum_val += self.get(i, k) * other.get(k, j)
                result.set(i, j, sum_val)
        return result

    def __str__(self):
        cdef list rows = []
        cdef list row
        cdef int i, j
        for i in range(self.m.rows):
            row = []
            for j in range(self.m.cols):
                row.append(f"{self.get(i, j):.2f}")
            rows.append("[" + ", ".join(row) + "]")
        return "[\n " + ",\n ".join(rows) + "\n]"

    def __len__(self):
        return self.m.rows * self.m.cols

    def __rmul__(self, double scalar):
        return self.__mul__(scalar)

    def __truediv__(self, double scalar):
        if scalar == 0.0:
            raise ZeroDivisionError("Cannot divide matrix by zero")
        return self.__mul__(1.0 / scalar)

    def __eq__(self, Matrix2D other):
        if self.m.rows != other.m.rows or self.m.cols != other.m.cols:
            return False
        cdef int i, j
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                if abs(self.get(i, j) - other.get(i, j)) > 1e-10:
                    return False
        return True

    def __ne__(self, Matrix2D other):
        return not self.__eq__(other)

    def __lt__(self, Matrix2D other):
        if self.m.rows != other.m.rows or self.m.cols != other.m.cols:
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                if self.get(i, j) >= other.get(i, j):
                    return False
        return True

    def __le__(self, Matrix2D other):
        if self.m.rows != other.m.rows or self.m.cols != other.m.cols:
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                if self.get(i, j) > other.get(i, j):
                    return False
        return True

    def __gt__(self, Matrix2D other):
        if self.m.rows != other.m.rows or self.m.cols != other.m.cols:
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                if self.get(i, j) <= other.get(i, j):
                    return False
        return True

    def __ge__(self, Matrix2D other):
        if self.m.rows != other.m.rows or self.m.cols != other.m.cols:
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                if self.get(i, j) < other.get(i, j):
                    return False
        return True

    def __abs__(self):
        return self.apply(lambda self_val, other_val, i, j: abs(self_val))

    def apply(self, object func, Matrix2D other=None):
        """Apply a function element-wise. Function signature: func(self_value, other_value=None, i, j)"""
        cdef Matrix2D result = Matrix2D(self.m.rows, self.m.cols)
        cdef int i, j
        cdef double self_val, other_val, result_val
        
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                self_val = self.get(i, j)
                if other is not None:
                    other_val = other.get(i, j)
                    result_val = func(self_val, other_val, i, j)
                else:
                    result_val = func(self_val, None, i, j)
                result.set(i, j, result_val)
        return result

    def apply_inplace(self, object func, Matrix2D other=None):
        """Apply a function element-wise in place. Function signature: func(self_value, other_value=None, i, j)"""
        cdef int i, j
        cdef double self_val, other_val, result_val
        
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                self_val = self.get(i, j)
                if other is not None:
                    other_val = other.get(i, j)
                    result_val = func(self_val, other_val, i, j)
                else:
                    result_val = func(self_val, None, i, j)
                self.set(i, j, result_val)

    @property
    def rows(self):
        return self.m.rows

    @property
    def cols(self):
        return self.m.cols

    cdef inline size_t _offset(self, int i, int j):
        """offset = (i_row * cols) + j_col"""
        return <size_t>(i * self.m.cols + j)

    cpdef double get(self, int i, int j):
        return self.m.data[self._offset(i,j)]

    cpdef void set(self, int i, int j, double value):
        self.m.data[self._offset(i,j)] = value



# -----------------------------------------------------------------------------
# 3D Matrix


cdef class Matrix3D:
    cdef matrix_3d *m
    cdef bint owner

    def __cinit__(self):
        self.m = NULL
        self.owner = False

    def __dealloc__(self):
        if self.m is not NULL and self.owner:
            free(self.m)

    def __init__(self, int rows, int cols, int planes):
        self.m = <matrix_3d *>malloc(sizeof(matrix_3d))
        if self.m is NULL:
            raise MemoryError()
        self.m.rows = rows
        self.m.cols = cols
        self.m.planes = planes
        self.m.data = <double *>calloc(rows * cols * planes, sizeof(double))
        self.owner = True

    def __getbuffer__(self, Py_buffer *view, int flags):
        if self.m.data == NULL:
            raise ValueError("uninitialized matrix")
        view.buf = <void *>self.m.data
        view.len = self.m.rows * self.m.cols * self.m.planes * sizeof(double)
        view.itemsize = sizeof(double)

        # 3‑D shape & strides
        cdef int ndim = 3
        cdef Py_ssize_t* shape = <Py_ssize_t *>malloc(ndim * sizeof(Py_ssize_t))
        cdef Py_ssize_t* strides = <Py_ssize_t *>malloc(ndim * sizeof(Py_ssize_t))

        shape[0] = self.m.rows
        shape[1] = self.m.cols
        shape[2] = self.m.planes

        # C‑contiguous layout: last index changes fastest
        strides[2] = view.itemsize
        strides[1] = strides[2]*self.m.planes
        strides[0] = strides[1]*self.m.cols

        view.shape = shape
        view.strides = strides
        view.suboffsets = NULL  # contiguous
        view.ndim = ndim
        view.format = "d"          # double
        view.readonly = False
        view.internal = NULL

    def __releasebuffer__(self, Py_buffer *view):
        free(view.shape)
        free(view.strides)

    def __repr__(self):
        cdef list planes = []
        cdef list rows
        cdef list row
        cdef int i, j, k
        for i in range(self.m.rows):
            rows = []
            for j in range(self.m.cols):
                row = []
                for k in range(self.m.planes):
                    row.append(self.get(i, j, k))
                rows.append(row)
            planes.append(rows)
        return f"Matrix3D({self.m.rows}x{self.m.cols}x{self.m.planes}, {planes})"

    def __getitem__(self, tuple indices):
        if len(indices) != 3:
            raise IndexError("Matrix3D requires exactly 3 indices")
        return self.get(indices[0], indices[1], indices[2])

    def __setitem__(self, tuple indices, double value):
        if len(indices) != 3:
            raise IndexError("Matrix3D requires exactly 3 indices")
        self.set(indices[0], indices[1], indices[2], value)

    def __add__(self, Matrix3D other):
        if (self.m.rows != other.m.rows or 
            self.m.cols != other.m.cols or 
            self.m.planes != other.m.planes):
            raise ValueError("Matrix dimensions must match for addition")
        return self.apply(lambda self_val, other_val, i, j, k: self_val + other_val, other)

    def __sub__(self, Matrix3D other):
        if (self.m.rows != other.m.rows or 
            self.m.cols != other.m.cols or 
            self.m.planes != other.m.planes):
            raise ValueError("Matrix dimensions must match for subtraction")
        return self.apply(lambda self_val, other_val, i, j, k: self_val - other_val, other)

    def __mul__(self, double scalar):
        return self.apply(lambda self_val, other_val, i, j, k: self_val * scalar)

    def __str__(self):
        cdef list planes = []
        cdef list rows
        cdef list row
        cdef int i, j, k
        for i in range(self.m.rows):
            rows = []
            for j in range(self.m.cols):
                row = []
                for k in range(self.m.planes):
                    row.append(f"{self.get(i, j, k):.2f}")
                rows.append("[" + ", ".join(row) + "]")
            planes.append("[" + ",\n  ".join(rows) + "]")
        return "[\n " + ",\n\n ".join(planes) + "\n]"

    def __len__(self):
        return self.m.rows * self.m.cols * self.m.planes

    def __rmul__(self, double scalar):
        return self.__mul__(scalar)

    def __truediv__(self, double scalar):
        if scalar == 0.0:
            raise ZeroDivisionError("Cannot divide matrix by zero")
        return self.__mul__(1.0 / scalar)

    def __eq__(self, Matrix3D other):
        if (self.m.rows != other.m.rows or 
            self.m.cols != other.m.cols or 
            self.m.planes != other.m.planes):
            return False
        cdef int i, j, k
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                for k in range(self.m.planes):
                    if abs(self.get(i, j, k) - other.get(i, j, k)) > 1e-10:
                        return False
        return True

    def __ne__(self, Matrix3D other):
        return not self.__eq__(other)

    def __lt__(self, Matrix3D other):
        if (self.m.rows != other.m.rows or 
            self.m.cols != other.m.cols or 
            self.m.planes != other.m.planes):
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j, k
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                for k in range(self.m.planes):
                    if self.get(i, j, k) >= other.get(i, j, k):
                        return False
        return True

    def __le__(self, Matrix3D other):
        if (self.m.rows != other.m.rows or 
            self.m.cols != other.m.cols or 
            self.m.planes != other.m.planes):
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j, k
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                for k in range(self.m.planes):
                    if self.get(i, j, k) > other.get(i, j, k):
                        return False
        return True

    def __gt__(self, Matrix3D other):
        if (self.m.rows != other.m.rows or 
            self.m.cols != other.m.cols or 
            self.m.planes != other.m.planes):
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j, k
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                for k in range(self.m.planes):
                    if self.get(i, j, k) <= other.get(i, j, k):
                        return False
        return True

    def __ge__(self, Matrix3D other):
        if (self.m.rows != other.m.rows or 
            self.m.cols != other.m.cols or 
            self.m.planes != other.m.planes):
            raise ValueError("Cannot compare matrices of different dimensions")
        cdef int i, j, k
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                for k in range(self.m.planes):
                    if self.get(i, j, k) < other.get(i, j, k):
                        return False
        return True

    def __abs__(self):
        return self.apply(lambda self_val, other_val, i, j, k: abs(self_val))

    def apply(self, object func, Matrix3D other=None):
        """Apply a function element-wise. Function signature: func(self_value, other_value=None, i, j, k)"""
        cdef Matrix3D result = Matrix3D(self.m.rows, self.m.cols, self.m.planes)
        cdef int i, j, k
        cdef double self_val, other_val, result_val
        
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                for k in range(self.m.planes):
                    self_val = self.get(i, j, k)
                    if other is not None:
                        other_val = other.get(i, j, k)
                        result_val = func(self_val, other_val, i, j, k)
                    else:
                        result_val = func(self_val, None, i, j, k)
                    result.set(i, j, k, result_val)
        return result

    def apply_inplace(self, object func, Matrix3D other=None):
        """Apply a function element-wise in place. Function signature: func(self_value, other_value=None, i, j, k)"""
        cdef int i, j, k
        cdef double self_val, other_val, result_val
        
        for i in range(self.m.rows):
            for j in range(self.m.cols):
                for k in range(self.m.planes):
                    self_val = self.get(i, j, k)
                    if other is not None:
                        other_val = other.get(i, j, k)
                        result_val = func(self_val, other_val, i, j, k)
                    else:
                        result_val = func(self_val, None, i, j, k)
                    self.set(i, j, k, result_val)

    @property
    def rows(self):
        return self.m.rows

    @property
    def cols(self):
        return self.m.cols

    @property
    def planes(self):
        return self.m.planes

    cdef inline size_t _offset(self, int i, int j, int k):
        """k_plane + planes * (j_col + cols + i_row)"""
        return <size_t>(k + self.m.planes * (j + self.m.cols * i))

    def offset(self, int row, int col, int plane):
        return self._offset(row, col, plane)

    cpdef double get(self, int i, int j, int k):
        return self.m.data[self._offset(i,j,k)]

    cpdef void set(self, int i, int j, int k, double value):
        self.m.data[self._offset(i,j,k)] = value



