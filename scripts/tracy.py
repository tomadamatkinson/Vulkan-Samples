#!/usr/bin/env python

# Copyright 2023, Thomas Atkinson
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 the "License";
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
from shutil import which
import shutil
import stat
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.join(SCRIPT_DIR, "..")
TRACY_SOURCE = os.path.join(ROOT_DIR, "third_party", "tracy")
TRACY_DIR = os.path.join(ROOT_DIR, ".tracy")

TRACY_BUILT_UNIX_EXEC = os.path.join(
    TRACY_SOURCE, "profiler", "build", "unix", "Tracy-release"
)
TRACY_TARGET_EXEC = (
    os.path.join(TRACY_DIR, "tracy")
    if os.name == "posix"
    else os.path.join(TRACY_DIR, "tracy.exe")
)


def create_tracy_dir():
    if not os.path.exists(TRACY_DIR):
        os.mkdir(TRACY_DIR)


def clean_tracy_third_party_changes():
    if os.path.exists(TRACY_SOURCE):
        subprocess.run(
            ["git", "reset", "--hard"],
            cwd=TRACY_SOURCE,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )


def clean_tracy():
    clean_tracy_third_party_changes()

    if os.path.exists(TRACY_DIR):
        shutil.rmtree(TRACY_DIR)


def build_tracy_unix() -> bool:
    if not which("pkg-config"):
        print("pkg-config not found in PATH")
        clean_tracy()
        return False

    if not which("make"):
        print("make not found in PATH")
        clean_tracy()
        return False

    print()
    print()
    print(
        "you may need to install libglfw3-dev, libcapstone-dev, libdbus-glib-1-dev, libfreetype-dev"
    )
    print("on ubuntu, you can run:")
    print(
        "sudo apt install libglfw3-dev libcapstone-dev libdbus-glib-1-dev libfreetype-dev"
    )
    print()
    print()

    make_returncode = subprocess.call(
        [
            "make",
            "LEGACY=1",
            "-C",
            os.path.join(TRACY_SOURCE, "profiler", "build", "unix"),
        ],  # ignore wayland build
        stdin=subprocess.PIPE,
    )

    if make_returncode != 0:
        print("make failed to build tracy")
        clean_tracy()
        return False

    if os.path.exists(TRACY_BUILT_UNIX_EXEC):
        create_tracy_dir()
        shutil.copyfile(TRACY_BUILT_UNIX_EXEC, TRACY_TARGET_EXEC)

    clean_tracy_third_party_changes()
    return True


def build_tracy():
    if os.name == "posix":
        return build_tracy_unix()
    else:
        print("tracy not supported on windows, yet!")
        exit(1)


def run_tracy():
    if not os.path.exists(TRACY_TARGET_EXEC):
        print("tracy not found - building tracy")
        if not build_tracy():
            print("failed to build tracy")
            exit(1)

    print("tracy found at {}".format(TRACY_TARGET_EXEC))
    st = os.stat(TRACY_TARGET_EXEC)
    os.chmod(TRACY_TARGET_EXEC, st.st_mode | stat.S_IEXEC)
    subprocess.call([TRACY_TARGET_EXEC])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="Tracy Helper",
        description="A utility to build and run the tracy profiler",
    )

    subparsers = parser.add_subparsers()

    ## Clean Tracy
    clean_tracy_command = subparsers.add_parser("clean", help="clean tracy")
    clean_tracy_command.set_defaults(func=clean_tracy)

    ## Build Tracy
    build_tracy_command = subparsers.add_parser("build", help="build tracy")
    build_tracy_command.set_defaults(func=build_tracy)

    ## Run Tracy
    run_tracy_command = subparsers.add_parser("run", help="run tracy")
    run_tracy_command.set_defaults(func=run_tracy)

    args = parser.parse_args()

    if len(sys.argv) == 1:
        run_tracy()
    else:
        args.func()
