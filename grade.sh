#!/bin/bash

# build the asti first...
mkdir build
cd build
# this line should be changed according to your env
cmake –DLLVM_DIR=$LLVM_DIR -DClang_DIR=$LLVM_DIR/lib/cmake/clang -DCMAKE_BUILD_TYPE=Debug ../.
make
cd .. 
# if no error occurs, we get a executable file
ASTI="./build/ast-interpreter"
LIBCODE="./lib/builtin.c"

TEST_DIR="${1:-./tests}"
file_list=$(ls $TEST_DIR)
# assume all the file in TEST_DIR is ``.c` file
total=$(echo "$file_list"|wc -w)
correct=0
echo "total test cases: $total"
for file in $file_list; do
    echo "testing $file"
    # result given by our interpreter
    filename="$TEST_DIR/$file"
    ccode=$(cat $filename)
    # make $correct as the user input, you can change it if you like
    actual=$(echo $correct|($ASTI "$ccode" 2>&1>/dev/null)) 
    # result given by gcc
    gcc $filename $LIBCODE -o a.out
    expected=$(echo $correct|./a.out)
    if [[ "$actual" = "$expected" ]]; then
        echo "$file passed"
        correct=$(( $correct + 1 ))
    fi
done
echo "$correct/$total"
rm a.out