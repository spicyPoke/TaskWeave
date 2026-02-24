#!/usr/bin/env python3
"""Run clang-format against all C++ files in src/ directory."""

import subprocess
import sys
from pathlib import Path


def main():
    root_dir = Path(__file__).parent.parent
    dirs_to_format = [root_dir / "src", root_dir / "tests"]
    extensions = {".h", ".cpp", ".hpp", ".cc", ".cxx"}

    files = [
        str(f) for d in dirs_to_format for f in d.rglob("*")
        if f.is_file() and f.suffix in extensions
    ]

    if not files:
        print("No C++ files found in src/")
        return 0

    print(f"Formatting {len(files)} file(s)...")
    for f in files:
        print(f"  {f}")

    try:
        subprocess.run(["clang-format", "-i"] + files, check=True)
        print("Done!")
        return 0
    except FileNotFoundError:
        print("Error: clang-format not found in PATH", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as e:
        print(f"Error: clang-format failed with code {e.returncode}", file=sys.stderr)
        return e.returncode


if __name__ == "__main__":
    sys.exit(main())
