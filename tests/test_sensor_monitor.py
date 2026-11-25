"""Black-box regression checks for the simulated five-byte sensor protocol."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
FIXED = ROOT / 'build' / 'sensor_monitor'
BUGGY = ROOT / 'build' / 'sensor_monitor_buggy'


def frame(start=0xAA, sensor=1, temp=25, humidity=60, corrupt=False):
    values = [start, sensor, temp & 0xFF, humidity]
    checksum = values[0] ^ values[1] ^ values[2] ^ values[3]
    return ' '.join(f'{v:02X}' for v in values + [checksum ^ int(corrupt)])


def run_frame(line, binary=FIXED):
    with tempfile.TemporaryDirectory() as directory:
        path = pathlib.Path(directory) / 'frames.txt'
        path.write_text(line + '\n', encoding='ascii')
        return subprocess.run([str(binary), str(path)], capture_output=True, text=True, check=True).stdout


class SensorTests(unittest.TestCase):
    def valid(self, line, reading):
        self.assertEqual(run_frame(line), f'OK line=1 {reading}\nSummary: valid=1 rejected=0\n')

    def rejected(self, line, reason):
        self.assertEqual(run_frame(line), f'ERROR line=1 reason={reason}\nSummary: valid=0 rejected=1\n')

    def test_positive(self): self.valid(frame(), 'sensor=1 temp=25C humidity=60%')
    def test_negative(self): self.valid(frame(sensor=2, temp=-5, humidity=50), 'sensor=2 temp=-5C humidity=50%')
    def test_zero(self): self.valid(frame(temp=0, humidity=0), 'sensor=1 temp=0C humidity=0%')
    def test_lower_boundary(self): self.valid(frame(temp=-40), 'sensor=1 temp=-40C humidity=60%')
    def test_upper_boundary(self): self.valid(frame(temp=85, humidity=100), 'sensor=1 temp=85C humidity=100%')
    def test_below_boundary(self): self.rejected(frame(temp=-41), 'temperature-range value=-41C')
    def test_above_boundary(self): self.rejected(frame(temp=86), 'temperature-range value=86C')
    def test_humidity_range(self): self.rejected(frame(humidity=101), 'humidity-range')
    def test_start_byte(self): self.rejected(frame(start=0xAB), 'start-byte')
    def test_checksum(self): self.rejected(frame(corrupt=True), 'checksum')
    def test_sensor_id(self): self.rejected(frame(sensor=0), 'sensor-id')
    def test_malformed(self): self.rejected('AA 01 XX 3C 00', 'malformed-frame')
    def test_missing_byte(self): self.rejected('AA 01 19 3C', 'malformed-frame')
    def test_extra_byte(self): self.rejected(frame() + ' 00', 'malformed-frame')
    def test_byte_overflow(self): self.rejected('AA 100 19 3C 00', 'malformed-frame')
    def test_comments(self):
        self.assertEqual(run_frame('# comment\n\n' + frame()), 'OK line=3 sensor=1 temp=25C humidity=60%\nSummary: valid=1 rejected=0\n')
    def test_missing_argument(self):
        result = subprocess.run([str(FIXED)], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Usage:', result.stderr)
    def test_missing_file(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([str(FIXED), str(pathlib.Path(directory)/'missing')], capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Cannot open frame file', result.stderr)
    def test_bug_reproduction(self):
        self.assertEqual(run_frame(frame(sensor=2, temp=-5, humidity=50), BUGGY), 'ERROR line=1 reason=temperature-range value=251C\nSummary: valid=0 rejected=1\n')


if __name__ == '__main__':
    unittest.main(verbosity=2)
