"""
Test multichannel buffer support for api.Buffer

Tests for GitHub issue #19: np.asarray(buf) returns incorrect number of
samples for multichannel buffers.

These tests verify:
1. np.asarray(buf) returns correct shape (frames, channels) for multichannel
2. buf.n_samples returns framecount * channelcount (total samples)
3. buf.n_frames returns framecount (samples per channel)
4. get_samples() returns all samples across all channels
5. set_samples() correctly handles multichannel interleaved data
6. Buffer protocol (memoryview) provides correct shape and data
7. Data integrity across all channels
"""

import math
import numpy as np
import api


def create_buffer(name, frames, channels):
    """Helper to create a buffer with specified frames and channels."""
    buf = api.Buffer(name, "", channels=channels)
    buf.framecount = frames
    return buf


# ---------------------------------------------------------------------------
# Test n_samples and n_frames properties
# ---------------------------------------------------------------------------

def test_n_samples_mono():
    """Test n_samples equals framecount for mono buffer."""
    name = "test_mono"
    channels = 1
    frames = 100
    buf = create_buffer(name, frames, channels)

    assert buf.n_samples == buf.framecount
    assert buf.n_frames == buf.framecount
    assert buf.channelcount == 1
    api.post(f"[PASS] mono: n_samples={buf.n_samples}, framecount={buf.framecount}")
    api.bang_success()

def test_n_samples_stereo():
    """Test n_samples equals framecount * 2 for stereo buffer."""
    name = "test_stereo"
    channels = 2
    frames = 100
    buf = create_buffer(name, frames, channels)

    assert buf.n_samples == buf.framecount * channels
    assert buf.n_frames == buf.framecount
    assert buf.channelcount == 2
    api.post(f"[PASS] stereo: n_samples={buf.n_samples}, framecount={buf.framecount}, "
             f"expected={buf.framecount * channels}")
    api.bang_success()


def test_n_samples_multichannel():
    """Test n_samples for various channel counts (4, 6, 8 channels)."""
    frames = 100

    for channels in [4, 6, 8]:
        name = f"test_{channels}ch"
        buf = create_buffer(name, frames, channels)

        expected_n_samples = buf.framecount * channels
        assert buf.n_samples == expected_n_samples, \
            f"{channels}ch: n_samples={buf.n_samples}, expected={expected_n_samples}"
        assert buf.n_frames == buf.framecount
        assert buf.channelcount == channels
        api.post(f"[PASS] {channels}ch: n_samples={buf.n_samples}, "
                 f"framecount={buf.framecount}")
    api.bang_success()


# ---------------------------------------------------------------------------
# Test np.asarray shape
# ---------------------------------------------------------------------------

def test_asarray_shape_mono():
    """Test np.asarray returns 1D array for mono buffer."""
    name = "test_mono_shape"
    channels = 1
    frames = 100
    buf = create_buffer(name, frames, channels)

    arr = np.asarray(buf)
    expected_shape = (buf.framecount,)

    assert arr.shape == expected_shape, \
        f"mono shape: got {arr.shape}, expected {expected_shape}"
    assert arr.dtype == np.float32
    api.post(f"[PASS] mono np.asarray shape: {arr.shape}")
    api.bang_success()

def test_asarray_shape_stereo():
    """Test np.asarray returns 2D array (frames, 2) for stereo buffer."""
    name = "test_stereo_shape"
    channels = 2
    frames = 100
    buf = create_buffer(name, frames, channels)

    arr = np.asarray(buf)
    expected_shape = (buf.framecount, channels)

    assert arr.shape == expected_shape, \
        f"stereo shape: got {arr.shape}, expected {expected_shape}"
    assert arr.dtype == np.float32
    api.post(f"[PASS] stereo np.asarray shape: {arr.shape}")
    api.bang_success()


def test_asarray_shape_multichannel():
    """Test np.asarray returns 2D array (frames, n) for n-channel buffers."""
    frames = 100

    for channels in [3, 4, 5, 6, 8]:
        name = f"test_{channels}ch_shape"
        buf = create_buffer(name, frames, channels)

        arr = np.asarray(buf)
        expected_shape = (buf.framecount, channels)

        assert arr.shape == expected_shape, \
            f"{channels}ch shape: got {arr.shape}, expected {expected_shape}"
        api.post(f"[PASS] {channels}ch np.asarray shape: {arr.shape}")
    api.bang_success()

# ---------------------------------------------------------------------------
# Test get_samples
# ---------------------------------------------------------------------------

def test_get_samples_mono():
    """Test get_samples returns all samples for mono buffer."""
    name = "test_get_mono"
    channels = 1
    frames = 100
    buf = create_buffer(name, frames, channels)

    samples = buf.get_samples()
    assert len(samples) == buf.n_samples
    assert len(samples) == buf.framecount
    api.post(f"[PASS] mono get_samples: {len(samples)} samples")


def test_get_samples_stereo():
    """Test get_samples returns all samples for stereo buffer."""
    name = "test_get_stereo"
    channels = 2
    frames = 100
    buf = create_buffer(name, frames, channels)

    samples = buf.get_samples()
    expected_count = buf.framecount * channels

    assert len(samples) == expected_count, \
        f"stereo get_samples: got {len(samples)}, expected {expected_count}"
    api.post(f"[PASS] stereo get_samples: {len(samples)} samples "
             f"(frames={buf.framecount}, channels={channels})")
    api.bang_success()


def test_get_samples_multichannel():
    """Test get_samples returns all samples for multichannel buffers."""
    frames = 100

    for channels in [4, 6, 8]:
        name = f"test_get_{channels}ch"
        buf = create_buffer(name, frames, channels)

        samples = buf.get_samples()
        expected_count = buf.framecount * channels

        assert len(samples) == expected_count, \
            f"{channels}ch get_samples: got {len(samples)}, expected {expected_count}"
        api.post(f"[PASS] {channels}ch get_samples: {len(samples)} samples")
    api.bang_success()

# ---------------------------------------------------------------------------
# Test set_samples
# ---------------------------------------------------------------------------

def test_set_samples_stereo():
    """Test set_samples correctly handles stereo interleaved data."""
    name = "test_set_stereo"
    channels = 2
    frames = 100

    # Create buffer with 2 channels
    buf = api.Buffer(name, "", channels=channels)
    buf.framecount = frames

    # Create interleaved stereo data: left=sin, right=-sin
    t = np.linspace(0, 1, frames, endpoint=False, dtype=np.float32)
    left = np.sin(t * 2 * math.pi * 5).astype(np.float32)
    right = -left

    # Interleave: [L0, R0, L1, R1, ...]
    interleaved = np.empty(frames * channels, dtype=np.float32)
    interleaved[0::2] = left
    interleaved[1::2] = right

    buf.set_samples(interleaved)

    # Verify framecount is correct (should still be 100, not 200)
    assert buf.framecount == frames, \
        f"framecount after set_samples: got {buf.framecount}, expected {frames}"

    # Read back and verify
    arr = np.asarray(buf)
    assert arr.shape == (frames, channels), \
        f"shape after set_samples: got {arr.shape}, expected {(frames, channels)}"

    # Check data integrity
    np.testing.assert_array_almost_equal(arr[:, 0], left, decimal=5)
    np.testing.assert_array_almost_equal(arr[:, 1], right, decimal=5)

    api.post(f"[PASS] stereo set_samples: shape={arr.shape}, data verified")
    api.bang_success()

def test_set_samples_multichannel():
    """Test set_samples correctly handles n-channel interleaved data."""
    frames = 100

    for channels in [4, 8]:
        name = f"test_set_{channels}ch"

        # Create buffer
        buf = api.Buffer(name, "", channels=channels)
        buf.framecount = frames

        # Create distinct waveform for each channel
        t = np.linspace(0, 1, frames, endpoint=False, dtype=np.float32)
        channel_data = []
        for ch in range(channels):
            # Different frequency and phase for each channel
            freq = 5 + ch
            phase = ch * 0.5
            data = np.sin(t * 2 * math.pi * freq + phase).astype(np.float32)
            channel_data.append(data)

        # Interleave: [ch0_f0, ch1_f0, ..., ch(n-1)_f0, ch0_f1, ...]
        interleaved = np.empty(frames * channels, dtype=np.float32)
        for ch in range(channels):
            interleaved[ch::channels] = channel_data[ch]

        buf.set_samples(interleaved)

        # Verify
        assert buf.framecount == frames
        arr = np.asarray(buf)
        assert arr.shape == (frames, channels)

        # Check each channel's data
        for ch in range(channels):
            np.testing.assert_array_almost_equal(
                arr[:, ch], channel_data[ch], decimal=5,
                err_msg=f"Channel {ch} data mismatch")

        api.post(f"[PASS] {channels}ch set_samples: shape={arr.shape}, all channels verified")
        api.bang_success()

def test_set_samples_validation():
    """Test set_samples raises error when sample count not divisible by channels."""
    name = "test_set_validation"
    channels = 2
    buf = api.Buffer(name, "", channels=channels)
    buf.framecount = 100

    # Try to set odd number of samples to 2-channel buffer
    bad_samples = np.zeros(101, dtype=np.float32)  # Not divisible by 2

    try:
        buf.set_samples(bad_samples)
        api.post("[FAIL] set_samples should have raised ValueError")
        assert False, "Expected ValueError for non-divisible sample count"
    except ValueError as e:
        api.post(f"[PASS] set_samples correctly raised ValueError: {e}")
    api.bang_success()

# ---------------------------------------------------------------------------
# Test memoryview / buffer protocol
# ---------------------------------------------------------------------------

def test_memoryview_mono():
    """Test memoryview returns correct shape for mono buffer."""
    name = "test_mv_mono"
    channels = 1
    frames = 100
    buf = create_buffer(name, frames, channels)

    with memoryview(buf) as mv:
        assert mv.ndim == 1
        assert mv.shape == (frames,)
        assert mv.format == 'f'  # float32
        assert len(mv) == frames
        ndim, shape = mv.ndim, mv.shape
    api.post(f"[PASS] mono memoryview: ndim={ndim}, shape={shape}")
    api.bang_success()


def test_memoryview_stereo():
    """Test memoryview returns 2D shape for stereo buffer."""
    name = "test_mv_stereo"
    channels = 2
    frames = 100
    buf = create_buffer(name, frames, channels)

    with memoryview(buf) as mv:
        assert mv.ndim == 2
        assert mv.shape == (frames, channels)
        assert mv.format == 'f'  # float32
        ndim, shape = mv.ndim, mv.shape
    api.post(f"[PASS] stereo memoryview: ndim={ndim}, shape={shape}")
    api.bang_success()


def test_memoryview_multichannel():
    """Test memoryview returns 2D shape for n-channel buffers."""
    frames = 100

    for channels in [4, 6, 8]:
        name = f"test_mv_{channels}ch"
        buf = create_buffer(name, frames, channels)

        with memoryview(buf) as mv:
            assert mv.ndim == 2
            assert mv.shape == (frames, channels)
            assert mv.format == 'f'
            ndim, shape = mv.ndim, mv.shape
        api.post(f"[PASS] {channels}ch memoryview: ndim={ndim}, shape={shape}")
    api.bang_success()


# ---------------------------------------------------------------------------
# Test round-trip data integrity
# ---------------------------------------------------------------------------

def test_roundtrip_stereo():
    """Test write and read back preserves data for stereo buffer."""
    name = "test_rt_stereo"
    channels = 2
    frames = 100

    buf = api.Buffer(name, "", channels=channels)
    buf.framecount = frames

    # Create test data
    t = np.linspace(0, 1, frames, endpoint=False, dtype=np.float32)
    original = np.column_stack([
        np.sin(t * 2 * math.pi * 5),   # Left channel
        np.cos(t * 2 * math.pi * 5)    # Right channel
    ]).astype(np.float32)

    # Flatten to interleaved format for set_samples
    interleaved = original.flatten()
    buf.set_samples(interleaved)

    # Read back via buffer protocol
    result = np.asarray(buf)

    # Compare
    np.testing.assert_array_almost_equal(result, original, decimal=5)
    api.post(f"[PASS] stereo roundtrip: data integrity verified")
    api.bang_success()


def test_roundtrip_multichannel():
    """Test write and read back preserves data for n-channel buffer."""
    frames = 100

    for channels in [4, 8]:
        name = f"test_rt_{channels}ch"

        buf = api.Buffer(name, "", channels=channels)
        buf.framecount = frames

        # Create test data - different waveform per channel
        t = np.linspace(0, 1, frames, endpoint=False, dtype=np.float32)
        original = np.column_stack([
            np.sin(t * 2 * math.pi * (5 + ch) + ch * 0.3).astype(np.float32)
            for ch in range(channels)
        ])

        # Flatten to interleaved format
        interleaved = original.flatten()
        buf.set_samples(interleaved)

        # Read back via buffer protocol
        result = np.asarray(buf)

        # Compare
        np.testing.assert_array_almost_equal(result, original, decimal=5)
        api.post(f"[PASS] {channels}ch roundtrip: data integrity verified")
    api.bang_success()

# ---------------------------------------------------------------------------
# Test from GitHub issue #19 scenario
# ---------------------------------------------------------------------------

def test_github_issue_19_scenario():
    """
    Reproduce the exact scenario from GitHub issue #19:

    A 2-channel, 100-frame buffer should:
    - Have n_samples = 200 (not 100)
    - np.asarray(buf) should return shape (100, 2), not (100,) or (200,)
    - All 200 samples should be accessible and correct
    """
    channel_count = 2
    length = 100

    buf = api.Buffer("issue19_test", "", channels=channel_count)
    buf.framecount = length

    # Create test data as in the issue
    t = np.linspace(0, 1, length, endpoint=False, dtype=np.float32)
    xs = np.array([
        np.sin(t * 2 * math.pi * 5) * (-1 if ch % 2 else 1)
        for ch in range(channel_count)
    ], dtype=np.float32).T  # Shape: (100, 2)

    # Flatten for set_samples (interleaved format)
    flattened_data = xs.flatten()
    assert len(flattened_data) == length * channel_count  # 200 samples

    buf.set_samples(flattened_data)

    # Now test the fix
    # 1. n_samples should be 200
    assert buf.n_samples == length * channel_count, \
        f"n_samples: got {buf.n_samples}, expected {length * channel_count}"

    # 2. np.asarray should return (100, 2), not (200,) or (100,)
    result = np.asarray(buf)
    expected_shape = (length, channel_count)
    assert result.shape == expected_shape, \
        f"shape: got {result.shape}, expected {expected_shape}"

    # 3. Data should match original
    np.testing.assert_array_almost_equal(result, xs, decimal=5)

    api.post(f"[PASS] GitHub issue #19 scenario: "
             f"n_samples={buf.n_samples}, shape={result.shape}, data verified")
    api.bang_success()


def test_github_issue_19_total_samples():
    """
    Additional test: verify total sample count is correctly reported.

    Before fix: for 2-channel 100-frame buffer, np.asarray returned only
    100 samples (framecount), missing half the data.

    After fix: should return all 200 samples in proper (100, 2) shape.
    """
    for channels in [2, 4, 8]:
        frames = 100
        name = f"issue19_count_{channels}ch"

        buf = api.Buffer(name, "", channels=channels)
        buf.framecount = frames

        expected_total = frames * channels

        # Check n_samples property
        assert buf.n_samples == expected_total, \
            f"{channels}ch n_samples: got {buf.n_samples}, expected {expected_total}"

        # Check actual array size
        arr = np.asarray(buf)
        actual_total = arr.size  # Total number of elements
        assert actual_total == expected_total, \
            f"{channels}ch array size: got {actual_total}, expected {expected_total}"

        api.post(f"[PASS] {channels}ch total samples: {actual_total} "
                 f"(frames={frames}, shape={arr.shape})")
    api.bang_success()
