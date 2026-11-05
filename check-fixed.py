#!/usr/bin/env python3
"""Check the documented terminal-text contract, not just the attack payload."""
import subprocess
import sys

binary = sys.argv[1] if len(sys.argv) > 1 else "./bo_fixed"
cases = [
    ("correct password", b"password1\n", True, False, 0),
    ("short wrong password", b"hello\n", False, False, 0),
    ("empty line", b"\n", False, False, 0),
    ("14 input bytes", b"A" * 14 + b"\n", False, False, 0),
    ("15 input bytes", b"A" * 15 + b"\n", False, False, 0),
    ("16 input bytes", b"A" * 16 + b"\n", False, True, 1),
    ("17 input bytes", b"A" * 17 + b"\n", False, True, 1),
    ("32 input bytes", b"A" * 32 + b"\n", False, True, 1),
    ("256 input bytes", b"A" * 256 + b"\n", False, True, 1),
    ("correct prefix plus excess", b"password1" + b"A" * 23 + b"\n", False, True, 1),
    ("correct password at EOF", b"password1", True, False, 0),
    ("15 bytes at EOF", b"A" * 15, False, False, 0),
    ("16 bytes at EOF", b"A" * 16, False, True, 1),
    ("EOF before input", b"", False, False, 1),
]
failed = 0
for label, data, login, oversized, status in cases:
    result = subprocess.run([binary], input=data, capture_output=True, timeout=5)
    output = result.stdout + result.stderr
    ok = ((b"[Privileged action executed]" in output) == login
          and (b"maximum is 15" in output) == oversized
          and result.returncode == status
          and (login or status != 0 or b"You fail!" in output)
          and b"AddressSanitizer" not in output
          and b"runtime error:" not in output)
    print(f"{'PASS' if ok else 'FAIL'}: {label}")
    if not ok:
        failed += 1
        print(f"  status={result.returncode}, output={output!r}")
sys.exit(bool(failed))
