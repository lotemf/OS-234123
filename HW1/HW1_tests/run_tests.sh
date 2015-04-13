#!/bin/bash

#
# To run:
# 1. Put all your test cases under tests_in dir
# 2. Put all your test results under tests_out. IMPORTANT:
#    If test_x.in is a filename of a specific testcase,
#    test_x.out must be the filename of the corresponding out file.
# 3. Rename the test runner (i.e. the executable) to "test_runner".
#    The script assumes it lies in the same directory.
# 4. Test results will be found in tests_res (if the directory doesn't exist
#    the script creates it).
#
#    Good Luck!
#

if [ ! -d "tests_res" ]; then
    mkdir tests_res
fi

for test_file in "tests_in"/*in
do
  test_file_b=$(basename "$test_file")
  test_file_b="${test_file_b%.*}"
  echo "Running test for $test_file_b..."
  ./test_runner < "$test_file" > "tests_res/$test_file_b.res"
  diff "tests_res/$test_file_b.res" "tests_out/$test_file_b.out" > /dev/null
  if [ $? -ne 0 ]; then
      echo "> Test $test_file_b had issues."
      echo "> To see issue report run:"
      echo ">   diff tests_res/$test_file_b.res tests_out/$test_file_b.out"
  fi
done