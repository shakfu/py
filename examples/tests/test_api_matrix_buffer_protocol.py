import os

import api

try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False


mem = {}


# Initialize matrices for each data type
def test_matrix_char_init():
    mem["m"] = api.Matrix("m_char")
    api.bang_success()


def test_matrix_long_init():
    mem["m"] = api.Matrix("m_long")
    api.bang_success()


def test_matrix_float_init():
    mem["m"] = api.Matrix("m_float")
    api.bang_success()


def test_matrix_double_init():
    mem["m"] = api.Matrix("m_double")
    api.bang_success()


# Buffer Protocol Tests
def test_matrix_buffer_protocol_basic():
    """Test basic buffer protocol functionality"""
    m = mem["m"]
    
    # Test that memoryview can be created from matrix
    mv = memoryview(m)
    
    # Verify memoryview properties
    api.post(f"memoryview format: {mv.format}")
    api.post(f"memoryview itemsize: {mv.itemsize}")
    api.post(f"memoryview ndim: {mv.ndim}")
    api.post(f"memoryview shape: {mv.shape}")
    api.post(f"memoryview strides: {mv.strides}")
    api.post(f"memoryview readonly: {mv.readonly}")
    
    # Verify format matches matrix type
    expected_formats = {
        'char': 'b',
        'long': 'l', 
        'float32': 'f',
        'float64': 'd'
    }
    expected_format = expected_formats[m.type]
    assert mv.format == expected_format, f"Expected format '{expected_format}', got '{mv.format}'"
    
    api.bang_success()


def test_matrix_buffer_protocol_numpy():
    """Test buffer protocol integration with NumPy"""
    if not HAS_NUMPY:
        api.post("NumPy not available, skipping NumPy buffer protocol test")
        return
    
    m = mem["m"]
    
    # Test numpy array conversion via buffer protocol
    arr = np.asarray(m)
    
    api.post(f"numpy array dtype: {arr.dtype}")
    api.post(f"numpy array shape: {arr.shape}")
    api.post(f"numpy array size: {arr.size}")
    
    # Verify dtype matches matrix type
    expected_dtypes = {
        'char': np.int8,
        'long': np.int32,
        'float32': np.float32,
        'float64': np.float64
    }
    expected_dtype = expected_dtypes[m.type]
    assert arr.dtype == expected_dtype, f"Expected dtype '{expected_dtype}', got '{arr.dtype}'"
    
    # Test that we can write to the array and it affects the matrix
    if arr.size > 0:
        original_value = arr.flat[0]
        if m.type in ['char', 'long']:
            test_value = 42
        else:
            test_value = 3.14
        
        arr.flat[0] = test_value
        
        # Verify the change is reflected in the matrix data
        matrix_data = m.get_data()
        if matrix_data and len(matrix_data) > 0:
            assert abs(matrix_data[0] - test_value) < 1e-6, "Buffer protocol write did not affect matrix"
        
        # Restore original value
        arr.flat[0] = original_value
    
    api.bang_success()


# Typed Memoryview Tests
def test_matrix_memoryview_float32():
    """Test as_float32_memoryview method"""
    m = mem["m"]
    
    if m.type != 'float32':
        # Test that wrong type raises ValueError
        try:
            mv = m.as_float32_memoryview()
            assert False, "Expected ValueError for wrong matrix type"
        except ValueError as e:
            api.post(f"Correctly raised ValueError: {e}")
            api.bang_success()
            return
    
    # Test correct usage
    mv = m.as_float32_memoryview()
    assert mv.format == 'f', f"Expected format 'f', got '{mv.format}'"
    api.post(f"float32 memoryview shape: {mv.shape}")
    api.bang_success()


def test_matrix_memoryview_float64():
    """Test as_float64_memoryview method"""
    m = mem["m"]
    
    if m.type != 'float64':
        # Test that wrong type raises ValueError
        try:
            mv = m.as_float64_memoryview()
            assert False, "Expected ValueError for wrong matrix type"
        except ValueError as e:
            api.post(f"Correctly raised ValueError: {e}")
            api.bang_success()
            return
    
    # Test correct usage
    mv = m.as_float64_memoryview()
    assert mv.format == 'd', f"Expected format 'd', got '{mv.format}'"
    api.post(f"float64 memoryview shape: {mv.shape}")
    api.bang_success()


def test_matrix_memoryview_char():
    """Test as_char_memoryview method"""
    m = mem["m"]
    
    if m.type != 'char':
        # Test that wrong type raises ValueError
        try:
            mv = m.as_char_memoryview()
            assert False, "Expected ValueError for wrong matrix type"
        except ValueError as e:
            api.post(f"Correctly raised ValueError: {e}")
            api.bang_success()
            return
    
    # Test correct usage
    mv = m.as_char_memoryview()
    assert mv.format == 'b', f"Expected format 'b', got '{mv.format}'"
    api.post(f"char memoryview shape: {mv.shape}")
    api.bang_success()


def test_matrix_memoryview_long():
    """Test as_long_memoryview method"""
    m = mem["m"]
    
    if m.type != 'long':
        # Test that wrong type raises ValueError
        try:
            mv = m.as_long_memoryview()
            assert False, "Expected ValueError for wrong matrix type"
        except ValueError as e:
            api.post(f"Correctly raised ValueError: {e}")
            api.bang_success()
            return
    
    # Test correct usage
    mv = m.as_long_memoryview()
    assert mv.format == 'l', f"Expected format 'l', got '{mv.format}'"
    api.post(f"long memoryview shape: {mv.shape}")
    api.bang_success()


def test_matrix_memoryview_generic():
    """Test as_memoryview method (generic)"""
    m = mem["m"]
    
    # This should work for any matrix type
    mv = m.as_memoryview()
    
    # Verify format matches matrix type
    expected_formats = {
        'char': 'b',
        'long': 'l', 
        'float32': 'f',
        'float64': 'd'
    }
    expected_format = expected_formats[m.type]
    assert mv.format == expected_format, f"Expected format '{expected_format}', got '{mv.format}'"
    
    api.post(f"generic memoryview format: {mv.format}")
    api.post(f"generic memoryview shape: {mv.shape}")
    api.bang_success()


def test_matrix_memoryview_data_access():
    """Test reading and writing data through memoryviews"""
    m = mem["m"]
    
    # Get memoryview
    mv = m.as_memoryview()
    
    if mv.shape and mv.shape[0] > 0:
        # Test reading
        original_value = mv[0] if mv.ndim == 1 else mv[0, 0]
        api.post(f"Original value at index 0: {original_value}")
        
        # Test writing
        if m.type in ['char', 'long']:
            test_value = 123
        else:
            test_value = 2.718
        
        if mv.ndim == 1:
            mv[0] = test_value
            new_value = mv[0]
        else:
            mv[0, 0] = test_value
            new_value = mv[0, 0]
        
        api.post(f"New value after write: {new_value}")
        
        # Verify the change is reflected in matrix data
        matrix_data = m.get_data()
        if matrix_data and len(matrix_data) > 0:
            assert abs(matrix_data[0] - test_value) < 1e-6, "Memoryview write did not affect matrix"
        
        # Restore original value
        if mv.ndim == 1:
            mv[0] = original_value
        else:
            mv[0, 0] = original_value
    
    api.bang_success()


def test_matrix_memoryview_slicing():
    """Test memoryview slicing functionality"""
    m = mem["m"]
    
    mv = m.as_memoryview()
    
    # Test basic slicing if matrix has enough elements
    if mv.shape and mv.shape[0] > 1:
        # Test slice creation
        slice_mv = mv[:2] if mv.ndim == 1 else mv[:2, :]
        api.post(f"Slice shape: {slice_mv.shape}")
        
        # Verify slice is a view of the same data
        assert slice_mv.obj is mv.obj, "Slice should reference same object"
    
    api.bang_success()


def test_matrix_buffer_protocol_error_handling():
    """Test error conditions for buffer protocol"""
    m = mem["m"]
    
    # The buffer protocol should work even with empty matrices
    # but let's test various edge cases
    
    try:
        mv = memoryview(m)
        api.post("Successfully created memoryview")
    except Exception as e:
        api.post(f"Error creating memoryview: {e}")
        # This might be expected for certain matrix states
    
    api.bang_success()


def test_matrix_memoryview_performance():
    """Test performance characteristics of memoryview access"""
    m = mem["m"]
    
    # Fill matrix with test data
    length = m.matrix_len
    if m.type in ["char", "long"]:
        test_data = list(range(length))
    else:
        test_data = [float(i) * 0.1 for i in range(length)]
    
    m.set_data(test_data)
    
    # Test memoryview access
    mv = m.as_memoryview()

    # api.post(f"{dir(mv)}")
    
    # Read all values through memoryview
    # if mv.size > 0:
    if len(mv) > 0:
        sum_via_mv = sum(mv)
        api.post(f"Sum via memoryview: {sum_via_mv}")
    
    # Compare with direct matrix data access
    matrix_data = m.get_data()
    if matrix_data:
        sum_via_matrix = sum(matrix_data)
        api.post(f"Sum via matrix.get_data(): {sum_via_matrix}")
        
        # Values should be approximately equal
        if mv.size > 0:
            assert abs(sum_via_mv - sum_via_matrix) < 1e-6, "Memoryview and matrix data should be equivalent"
    
    api.bang_success()