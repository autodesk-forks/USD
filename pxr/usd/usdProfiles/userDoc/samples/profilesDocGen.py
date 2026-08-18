#!/usr/bin/env python3
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license

"""
This sample demonstrates how to generate Markdown, Mermaid, and GraphViz DOT
documentation from the UsdProfiles ProfileRegistry.

Running with no arguments will run against the live Profiles registry and 
generate sample outputs as populated by the installed plugins.

New profile graphs can be tested by passing a path to a json file to test, and
it will be loaded via the _TestLoadFromFile hook.

Configuration via environment variables:

    LOCAL_USD       Path to an OpenUSD install (the directory containing
                    lib/, plugin/, etc.). If set, the script prepends its
                    lib/python to sys.path, points the dynamic loader at
                    its lib/, and adds plugin/usd to PXR_PLUGINPATH_NAME.
                    If unset, the script assumes 'from pxr import ...'
                    resolves via the ambient Python environment.

    USD_PROFILES_DOC_OUT_DIR
                    Directory to write generated output files into.
                    Defaults to the platform temp directory.

Examples:

    # Against an explicit local install:
    LOCAL_USD=/path/to/local-usd \\
        python3 test_profiles_doc_utils.py

    # Against an environment that already has UsdProfiles importable:
    python3 test_profiles_doc_utils.py

    # With a synthetic registry fixture:
    python3 test_profiles_doc_utils.py path/to/fixture.json

Note: on macOS, DYLD_LIBRARY_PATH must be set before the Python
interpreter launches to take effect. If you see import errors when
relying on LOCAL_USD, invoke the script with the variable already in
the parent environment, e.g.:

    DYLD_LIBRARY_PATH="$LOCAL_USD/lib" \\
    LOCAL_USD="$LOCAL_USD" \\
        python3 test_profiles_doc_utils.py
"""

import os
import sys
import tempfile


def _configure_local_usd(local_usd):
    """Configure sys.path and loader/plugin env vars for a USD install
    rooted at local_usd."""
    py_site = os.path.join(local_usd, "lib", "python")
    lib_dir = os.path.join(local_usd, "lib")
    plugin  = os.path.join(local_usd, "plugin", "usd")

    sys.path.insert(0, py_site)

    for var in ("DYLD_LIBRARY_PATH", "LD_LIBRARY_PATH"):
        existing = os.environ.get(var, "")
        if lib_dir not in existing.split(os.pathsep):
            os.environ[var] = (
                lib_dir + (os.pathsep + existing if existing else ""))

    existing_pp = os.environ.get("PXR_PLUGINPATH_NAME", "")
    if plugin not in existing_pp.split(os.pathsep):
        os.environ["PXR_PLUGINPATH_NAME"] = (
            plugin + (os.pathsep + existing_pp if existing_pp else ""))


_local_usd = os.environ.get("LOCAL_USD")
if _local_usd:
    _configure_local_usd(_local_usd)

from pxr import UsdProfiles  # noqa: E402


def main():
    print("Using UsdProfiles from:", UsdProfiles.__file__)

    fixture = sys.argv[1] if len(sys.argv) > 1 else None
    if fixture:
        UsdProfiles.ProfileRegistry._TestClear()
        ok, errors = UsdProfiles.ProfileRegistry._TestLoadFromFile(fixture)
        if not ok:
            print("Failed to load fixture:", fixture)
            for e in errors:
                print("  ", e)
            sys.exit(1)
        print("Loaded fixture:", fixture)
    else:
        print("Using registry as populated by installed plugins")

    caps = UsdProfiles.ProfileRegistry.GetAllCapabilities()
    profiles = UsdProfiles.ProfileRegistry.GetAllProfiles()
    print(f"Capabilities ({len(caps)}):", sorted(caps))
    print(f"Profiles     ({len(profiles)}):", sorted(profiles))

    out_dir = os.environ.get("USD_PROFILES_DOC_OUT_DIR") or tempfile.gettempdir()
    os.makedirs(out_dir, exist_ok=True)

    outputs = [
        ("profiles.md",
         "Plain Markdown (no diagram)",
         lambda: UsdProfiles.ProfilesMarkdown()),
        ("profiles_with_diagram.md",
         "Markdown + embedded Mermaid (registry styles)",
         lambda: UsdProfiles.ProfilesMarkdown({})),
        ("profiles_overrides.md",
         "Markdown with caller style overrides",
         lambda: UsdProfiles.ProfilesMarkdown({
             "core":    "fill:#cce5ff,stroke:#004085",
             "profile": "fill:#d4edda,stroke:#155724",
         })),
        ("profiles.mmd",
         "Standalone Mermaid (registry styles)",
         lambda: UsdProfiles.ProfilesMermaid({})),
        ("profiles.dot",
         "Graphviz DOT",
         lambda: UsdProfiles.ProfilesDot()),
    ]

    for name, desc, fn in outputs:
        path = os.path.join(out_dir, name)
        text = fn()
        with open(path, "w") as f:
            f.write(text)
        print(f"Wrote {path:<60s}  {len(text):>5d} bytes  -- {desc}")

    print("\nAll outputs written under", out_dir)


if __name__ == "__main__":
    main()
