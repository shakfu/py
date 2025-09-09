import numpy as np

import matrix

def test_matrix_2d_square():
    m2 = matrix.Matrix2D(3,3)

    m2.set(0, 0, 1.5)
    m2.set(1, 1, 1.5)
    m2.set(2, 2, 1.5)

    a = np.array(m2)

    assert m2.get(0, 0) == 1.5
    assert m2.get(1, 1) == 1.5
    assert m2.get(2, 2) == 1.5

    assert np.equal(a, np.array([
        [1.5, 0. , 0. ],
        [0. , 1.5, 0. ],
        [0. , 0. , 1.5]
    ])).all()

def test_matrix_2d_rect():
    m2 = matrix.Matrix2D(2,3)

    m2.set(0, 0, 1.5)
    m2.set(1, 1, 1.5)
    m2.set(1, 2, 1.5)

    a = np.array(m2)

    assert m2.get(0, 0) == 1.5
    assert m2.get(1, 1) == 1.5
    assert m2.get(1, 2) == 1.5

    assert np.equal(a, np.array([
        [1.5, 0. , 0. ],
       [0. , 1.5, 1.5]])).all()


def test_matrix_3d_basic():
    m3 = matrix.Matrix3D(2, 3, 4)
    
    # Test element access
    m3.set(0, 0, 0, 1.5)
    m3.set(1, 1, 2, 2.5)
    m3.set(1, 2, 3, 3.5)
    
    assert m3.get(0, 0, 0) == 1.5
    assert m3.get(1, 1, 2) == 2.5
    assert m3.get(1, 2, 3) == 3.5
    assert m3.get(0, 1, 1) == 0.0  # unset element


def test_matrix_3d_numpy_integration():
    m3 = matrix.Matrix3D(2, 3, 4)
    
    # Set some values
    m3.set(0, 0, 0, 1.0)
    m3.set(0, 1, 1, 2.0)
    m3.set(1, 2, 3, 3.0)
    
    # Convert to NumPy array
    a = np.array(m3)
    
    # Check shape
    assert a.shape == (2, 3, 4)
    
    # Check values
    assert a[0, 0, 0] == 1.0
    assert a[0, 1, 1] == 2.0
    assert a[1, 2, 3] == 3.0
    assert a[0, 0, 1] == 0.0  # unset element


def test_matrix_3d_memoryview():
    m3 = matrix.Matrix3D(2, 2, 2)
    
    # Set diagonal values
    m3.set(0, 0, 0, 1.0)
    m3.set(1, 1, 1, 2.0)
    
    # Test memoryview
    mv = memoryview(m3)
    assert mv.ndim == 3
    assert mv.shape == (2, 2, 2)
    assert mv.readonly == False
    
    # Access via memoryview
    assert mv[0, 0, 0] == 1.0
    assert mv[1, 1, 1] == 2.0


def test_matrix_2d_dunder_methods():
    m1 = matrix.Matrix2D(2, 2)
    m2 = matrix.Matrix2D(2, 2)
    
    # Test __setitem__ and __getitem__
    m1[0, 0] = 1.0
    m1[0, 1] = 2.0
    m1[1, 0] = 3.0
    m1[1, 1] = 4.0
    
    assert m1[0, 0] == 1.0
    assert m1[0, 1] == 2.0
    assert m1[1, 0] == 3.0
    assert m1[1, 1] == 4.0
    
    # Test addition
    m2[0, 0] = 2.0
    m2[1, 1] = 3.0
    m3 = m1 + m2
    assert m3[0, 0] == 3.0  # 1.0 + 2.0
    assert m3[1, 1] == 7.0  # 4.0 + 3.0
    
    # Test subtraction
    m4 = m1 - m2
    assert m4[0, 0] == -1.0  # 1.0 - 2.0
    assert m4[1, 1] == 1.0   # 4.0 - 3.0
    
    # Test scalar multiplication
    m5 = m1 * 2.0
    assert m5[0, 0] == 2.0  # 1.0 * 2.0
    assert m5[1, 1] == 8.0  # 4.0 * 2.0
    
    # Test matrix multiplication
    m6 = matrix.Matrix2D(2, 2)
    m6[0, 0] = 1.0
    m6[0, 1] = 0.0
    m6[1, 0] = 0.0
    m6[1, 1] = 1.0  # identity matrix
    
    m7 = m1 @ m6  # should equal m1
    assert m7[0, 0] == 1.0
    assert m7[0, 1] == 2.0
    assert m7[1, 0] == 3.0
    assert m7[1, 1] == 4.0


def test_matrix_3d_dunder_methods():
    m1 = matrix.Matrix3D(2, 2, 2)
    m2 = matrix.Matrix3D(2, 2, 2)
    
    # Test __setitem__ and __getitem__
    m1[0, 0, 0] = 1.0
    m1[0, 1, 1] = 2.0
    m1[1, 0, 1] = 3.0
    m1[1, 1, 0] = 4.0
    
    assert m1[0, 0, 0] == 1.0
    assert m1[0, 1, 1] == 2.0
    assert m1[1, 0, 1] == 3.0
    assert m1[1, 1, 0] == 4.0
    
    # Test addition
    m2[0, 0, 0] = 2.0
    m2[1, 1, 0] = 3.0
    m3 = m1 + m2
    assert m3[0, 0, 0] == 3.0  # 1.0 + 2.0
    assert m3[1, 1, 0] == 7.0  # 4.0 + 3.0
    
    # Test subtraction
    m4 = m1 - m2
    assert m4[0, 0, 0] == -1.0  # 1.0 - 2.0
    assert m4[1, 1, 0] == 1.0   # 4.0 - 3.0
    
    # Test scalar multiplication
    m5 = m1 * 2.0
    assert m5[0, 0, 0] == 2.0  # 1.0 * 2.0
    assert m5[1, 1, 0] == 8.0  # 4.0 * 2.0


def test_matrix_2d_extended_dunder_methods():
    m1 = matrix.Matrix2D(2, 2)
    m2 = matrix.Matrix2D(2, 2)
    
    # Setup matrices
    m1[0, 0] = 2.0
    m1[0, 1] = 4.0
    m1[1, 0] = 6.0
    m1[1, 1] = 8.0
    
    m2[0, 0] = 1.0
    m2[0, 1] = 2.0
    m2[1, 0] = 3.0
    m2[1, 1] = 4.0
    
    # Test __str__
    str_repr = str(m1)
    assert "2.00" in str_repr
    assert "4.00" in str_repr
    
    # Test __len__
    assert len(m1) == 4  # 2x2 matrix
    
    # Test __rmul__ (right multiplication)
    m3 = 3.0 * m1
    assert m3[0, 0] == 6.0  # 3.0 * 2.0
    
    # Test __truediv__
    m4 = m1 / 2.0
    assert m4[0, 0] == 1.0  # 2.0 / 2.0
    assert m4[1, 1] == 4.0  # 8.0 / 2.0
    
    # Test __eq__ and __ne__
    m5 = matrix.Matrix2D(2, 2)
    m5[0, 0] = 2.0
    m5[0, 1] = 4.0
    m5[1, 0] = 6.0
    m5[1, 1] = 8.0
    
    assert m1 == m5
    assert not (m1 != m5)
    assert m1 != m2
    
    # Test comparison operators
    assert m2 < m1  # all elements of m2 < m1
    assert m2 <= m1
    assert m1 > m2
    assert m1 >= m2
    
    # Test __abs__
    m_neg = matrix.Matrix2D(2, 2)
    m_neg[0, 0] = -2.0
    m_neg[1, 1] = -3.0
    m_abs = abs(m_neg)
    assert m_abs[0, 0] == 2.0
    assert m_abs[1, 1] == 3.0


def test_matrix_3d_extended_dunder_methods():
    m1 = matrix.Matrix3D(2, 2, 2)
    m2 = matrix.Matrix3D(2, 2, 2)
    
    # Setup matrices
    m1[0, 0, 0] = 2.0
    m1[1, 1, 1] = 4.0
    
    m2[0, 0, 0] = 1.0
    m2[1, 1, 1] = 2.0
    
    # Test __str__
    str_repr = str(m1)
    assert "2.00" in str_repr
    
    # Test __len__
    assert len(m1) == 8  # 2x2x2 matrix
    
    # Test __rmul__ (right multiplication)
    m3 = 2.0 * m1
    assert m3[0, 0, 0] == 4.0  # 2.0 * 2.0
    
    # Test __truediv__
    m4 = m1 / 2.0
    assert m4[0, 0, 0] == 1.0  # 2.0 / 2.0
    assert m4[1, 1, 1] == 2.0  # 4.0 / 2.0
    
    # Test __eq__ and __ne__
    m5 = matrix.Matrix3D(2, 2, 2)
    m5[0, 0, 0] = 2.0
    m5[1, 1, 1] = 4.0
    
    assert m1 == m5
    assert not (m1 != m5)
    assert m1 != m2
    
    # Test comparison operators (setup matrices where all elements satisfy condition)
    m_small = matrix.Matrix3D(2, 2, 2)
    m_large = matrix.Matrix3D(2, 2, 2)
    
    # Fill all elements to satisfy comparison
    for i in range(2):
        for j in range(2):
            for k in range(2):
                m_small[i, j, k] = 1.0
                m_large[i, j, k] = 2.0
    
    assert m_small < m_large  # all elements of m_small < m_large
    assert m_small <= m_large
    assert m_large > m_small
    assert m_large >= m_small
    
    # Test __abs__
    m_neg = matrix.Matrix3D(2, 2, 2)
    m_neg[0, 0, 0] = -3.0
    m_neg[1, 1, 1] = -5.0
    m_abs = abs(m_neg)
    assert m_abs[0, 0, 0] == 3.0
    assert m_abs[1, 1, 1] == 5.0


def test_division_by_zero():
    m = matrix.Matrix2D(2, 2)
    m[0, 0] = 1.0
    
    try:
        result = m / 0.0
        assert False, "Should have raised ZeroDivisionError"
    except ZeroDivisionError:
        pass  # Expected
    
    m3 = matrix.Matrix3D(2, 2, 2)
    m3[0, 0, 0] = 1.0
    
    try:
        result = m3 / 0.0
        assert False, "Should have raised ZeroDivisionError"
    except ZeroDivisionError:
        pass  # Expected


def test_matrix_2d_apply_method():
    m1 = matrix.Matrix2D(2, 2)
    m2 = matrix.Matrix2D(2, 2)
    
    # Setup matrices
    m1[0, 0] = 1.0
    m1[0, 1] = 2.0
    m1[1, 0] = 3.0
    m1[1, 1] = 4.0
    
    m2[0, 0] = 2.0
    m2[0, 1] = 3.0
    m2[1, 0] = 4.0
    m2[1, 1] = 5.0
    
    # Test apply with two matrices (addition function)
    result = m1.apply(lambda self_val, other_val, i, j: self_val + other_val, m2)
    assert result[0, 0] == 3.0  # 1.0 + 2.0
    assert result[0, 1] == 5.0  # 2.0 + 3.0
    assert result[1, 0] == 7.0  # 3.0 + 4.0
    assert result[1, 1] == 9.0  # 4.0 + 5.0
    
    # Test apply with single matrix (square function)
    result = m1.apply(lambda self_val, other_val, i, j: self_val ** 2)
    assert result[0, 0] == 1.0  # 1.0^2
    assert result[0, 1] == 4.0  # 2.0^2
    assert result[1, 0] == 9.0  # 3.0^2
    assert result[1, 1] == 16.0  # 4.0^2
    
    # Test apply_inplace
    m1.apply_inplace(lambda self_val, other_val, i, j: self_val * 2.0)
    assert m1[0, 0] == 2.0  # 1.0 * 2.0
    assert m1[0, 1] == 4.0  # 2.0 * 2.0
    assert m1[1, 0] == 6.0  # 3.0 * 2.0
    assert m1[1, 1] == 8.0  # 4.0 * 2.0


def test_matrix_3d_apply_method():
    m1 = matrix.Matrix3D(2, 2, 2)
    m2 = matrix.Matrix3D(2, 2, 2)
    
    # Setup matrices
    m1[0, 0, 0] = 1.0
    m1[0, 1, 1] = 2.0
    m1[1, 0, 1] = 3.0
    m1[1, 1, 0] = 4.0
    
    m2[0, 0, 0] = 2.0
    m2[0, 1, 1] = 3.0
    m2[1, 0, 1] = 4.0
    m2[1, 1, 0] = 5.0
    
    # Test apply with two matrices (multiplication function)
    result = m1.apply(lambda self_val, other_val, i, j, k: self_val * other_val, m2)
    assert result[0, 0, 0] == 2.0  # 1.0 * 2.0
    assert result[0, 1, 1] == 6.0  # 2.0 * 3.0
    assert result[1, 0, 1] == 12.0  # 3.0 * 4.0
    assert result[1, 1, 0] == 20.0  # 4.0 * 5.0
    
    # Test apply with single matrix (cube function)
    result = m1.apply(lambda self_val, other_val, i, j, k: self_val ** 3)
    assert result[0, 0, 0] == 1.0  # 1.0^3
    assert result[0, 1, 1] == 8.0  # 2.0^3
    assert result[1, 0, 1] == 27.0  # 3.0^3
    assert result[1, 1, 0] == 64.0  # 4.0^3
    
    # Test apply_inplace
    m1.apply_inplace(lambda self_val, other_val, i, j, k: self_val + 10.0)
    assert m1[0, 0, 0] == 11.0  # 1.0 + 10.0
    assert m1[0, 1, 1] == 12.0  # 2.0 + 10.0
    assert m1[1, 0, 1] == 13.0  # 3.0 + 10.0
    assert m1[1, 1, 0] == 14.0  # 4.0 + 10.0


if __name__ == '__main__':

    m3 = matrix.Matrix3D(2, 2, 2)
    
    # Set diagonal values
    m3.set(0, 0, 0, 1.0)
    m3.set(1, 1, 1, 2.0)
    
    # Test memoryview
    mv = memoryview(m3)
    assert mv.ndim == 3
    assert mv.shape == (2, 2, 2)
    assert mv.readonly == False
    
    # Access via memoryview
    assert mv[0, 0, 0] == 1.0
    assert mv[1, 1, 1] == 2.0

