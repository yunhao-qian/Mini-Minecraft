"""Preprocesses a GLSL shader file to resolve #include directives."""

import re
from argparse import ArgumentParser
from collections.abc import Iterator
from pathlib import Path


def preprocess_shader(shader_path: Path, included_paths: set[Path]) -> Iterator[str]:
    """Takes a shader file, resolves #include directives, and yields the preprocessed shader source
    lines."""

    with shader_path.open(encoding="utf-8") as file:
        for line in file:
            # Strip trailing whitespaces and newlines.
            line = line.rstrip()

            # Check for an #include directive.
            include_match = re.fullmatch(r'#include\s*"([^"]+)"', line)

            if include_match is None:
                # The line is not an #include directive. Yield it as is.
                yield line
                continue

            included_path = shader_path.parent / include_match.group(1)

            # Avoid circular includes.
            resolved_included_path = included_path.resolve()
            if resolved_included_path in included_paths:
                continue
            included_paths.add(resolved_included_path)

            # Process the included file recursively.
            yield from preprocess_shader(included_path, included_paths)


def main() -> None:
    """Main function of the script."""

    # Parse command line arguments.
    parser = ArgumentParser()
    parser.add_argument("input_path", type=Path)
    parser.add_argument("output_path", type=Path)
    args = parser.parse_args()

    # Prevent newline character translation on Windows.
    with args.output_path.open("w", encoding="utf-8", newline="\n") as output_file:
        for line in preprocess_shader(args.input_path, set()):
            output_file.write(line + "\n")


if __name__ == "__main__":
    main()
