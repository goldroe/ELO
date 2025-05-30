import subprocess
import os
import sys

file_name = sys.argv[1]
# byte_file = os.path.splitext(file_name)[0] + '.bc'

process = subprocess.run(['elo', file_name], capture_output=True, text=True)
print(process.stdout, end='')
print(process.stderr, end='')

# Check the return code
if process.returncode == 0:
    process = subprocess.run(['llvm-dis', 'out.bc'], capture_output=True, text=True)
    print(process.stdout, end='')
    print(process.stderr, end='')
    if process.returncode == 0:
        process = subprocess.run(['clang', 'out.ll'], capture_output=True, text=True)
        print(process.stdout, end='')
        print(process.stderr, end='')
