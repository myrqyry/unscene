import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("verify_native", Path(__file__).with_name("verify-native.py"))
native = importlib.util.module_from_spec(spec)
spec.loader.exec_module(native)

class NativeContract(unittest.TestCase):
    def test_layouts(self):
        self.assertEqual(native.module_relative_path("windows-x64", "32.2.0").as_posix(), "unscene/bin/64bit/unscene.dll")
        self.assertEqual(native.module_relative_path("windows-x64", "33.0.3").as_posix(), "unscene/unscene.dll")
        for target in ("macos-arm64", "macos-x86_64"):
            self.assertEqual(native.module_relative_path(target, "33.0.3").as_posix(), "unscene.plugin/Contents/MacOS/unscene")
        with self.assertRaises(ValueError):
            native.sdk_pin("latest")

    def test_windows_stage(self):
        for version in native.PINS:
            with self.subTest(version=version), tempfile.TemporaryDirectory() as temp:
                stage = Path(temp)
                module = stage / native.module_relative_path("windows-x64", version)
                module.parent.mkdir(parents=True)
                # Header fixture validates the stage gate, not a real PE build.
                image = bytearray(72)
                image[:2] = b"MZ"
                image[60:64] = (64).to_bytes(4, "little")
                image[64:68] = b"PE\0\0"
                image[68:70] = (0x8664).to_bytes(2, "little")
                module.write_bytes(image)
                (stage / "unscene/LICENSE").write_bytes((native.ROOT / "LICENSE").read_bytes())
                self.assertEqual(native.verify_stage(stage, "windows-x64", version), module)
                extra = stage / "unscene/extra.pdb"
                extra.write_bytes(b"extra")
                with self.assertRaises(RuntimeError):
                    native.verify_stage(stage, "windows-x64", version)
                extra.unlink()
                image[68:70] = (0x14c).to_bytes(2, "little")
                module.write_bytes(image)
                with self.assertRaises(RuntimeError):
                    native.verify_stage(stage, "windows-x64", version)

if __name__ == "__main__":
    unittest.main()
