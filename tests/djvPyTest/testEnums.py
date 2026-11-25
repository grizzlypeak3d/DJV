# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the feather-tk project.

import feather_tk
import djv

import unittest

# Bitmasks: the values are flags, not a run.
BITMASKS = set()

class EnumsTest(unittest.TestCase):

    def test_contiguous(self):
        """
        Every bound enum covers 0 through N-1 with no gaps: a gap means
        a value was added to the C++ enum and not to a hand-written
        binding (this is how Key was found missing F13-F24).
        """
        seen = set()
        def walk(mod, prefix):
            for name in dir(mod):
                obj = getattr(mod, name)
                if isinstance(obj, type) and hasattr(obj, "__members__"):
                    if id(obj) in seen or name in BITMASKS:
                        continue
                    seen.add(id(obj))
                    values = sorted(int(v.value) for v in obj.__members__.values())
                    self.assertEqual(
                        values,
                        list(range(len(values))),
                        f"{prefix}.{name} has gaps: {values}")
                elif isinstance(obj, type(djv)) and obj.__name__.startswith("djv."):
                    walk(obj, f"{prefix}.{name}")
        walk(djv, "djv")
        self.assertGreater(len(seen), 0)

    def test_complete(self):
        """
        Every value of every bound enum, up to the C++ Count: each
        get<Enum>Enums() lists them all, and fails on one the binding
        does not have. The gaps test above cannot see a value missing
        at the end (this is how ImageType was found missing RGB_F16_P).
        """
        count = 0
        seen = set()
        def walk(mod, prefix):
            for name in dir(mod):
                obj = getattr(mod, name)
                if name.startswith("get") and name.endswith("Enums") and callable(obj):
                    nonlocal count
                    count += 1
                    try:
                        obj()
                    except ValueError as e:
                        self.fail(f"{prefix}.{name}(): {e}")
                elif isinstance(obj, type(djv)) and obj.__name__.startswith("djv") and id(obj) not in seen:
                    seen.add(id(obj))
                    walk(obj, f"{prefix}.{name}")
        walk(djv, "djv")
        self.assertGreater(count, 0)

if __name__ == "__main__":
    unittest.main()
