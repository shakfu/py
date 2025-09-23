# ============================================================================
# test_extension.py - Unit tests for the extension
# ============================================================================


import unittest
import math_extension

class TestMathExtension(unittest.TestCase):
    
    def setUp(self):
        # Add test functions
        math_extension.add_function("def test_add(x, y): return x + y", "test.py")
        math_extension.add_function("def test_square(x): return x * x", "test.py")
    
    def test_basic_function_call(self):
        result = math_extension.call_function('test_add', 5, 3)
        self.assertEqual(result, 8)
    
    def test_single_argument_function(self):
        result = math_extension.call_function('test_square', 4)
        self.assertEqual(result, 16)
    
    def test_function_existence(self):
        self.assertTrue(math_extension.has_function('test_add'))
        self.assertFalse(math_extension.has_function('nonexistent'))
    
    def test_statistics(self):
        stats = math_extension.get_stats()
        self.assertIn('total_entries', stats)
        self.assertIn('cache_hits', stats)
        self.assertIn('hit_ratio', stats)
    
    def test_error_handling(self):
        with self.assertRaises(KeyError):
            math_extension.call_function('nonexistent_function', 1, 2)
        
        with self.assertRaises(SyntaxError):
            math_extension.add_function("def invalid syntax", "error.py")

if __name__ == '__main__':
    unittest.main()


