#!/usr/bin/env python3
"""Compile core tests and the actual checkRun body, without a GD installation."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parent.parent
compiler = os.environ.get('CXX', 'g++')
with tempfile.TemporaryDirectory(prefix='baconsistent-tests-') as directory:
    temporary = Path(directory)
    source = (root / 'src/store/GlobalStore.cpp').read_text()
    begin = source.index('int GlobalStore::checkRun(')
    end = source.index('// ! --- Search API --- !', begin)
    (temporary / 'check_run_body.inc').write_text(source[begin:end])
    for name in ('training', 'check_run'):
        executable = temporary / name
        subprocess.run([compiler, '-std=c++23', '-Wall', '-Wextra', '-Werror',
            '-fsanitize=undefined', '-g', '-I'+str(temporary),
            str(root / f'tests/{name}.cpp'), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
