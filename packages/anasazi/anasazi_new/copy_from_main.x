#!/bin/sh
#
# This script must be run from this directory
#

# Copy files from packages/belos to packages/anasazi/anasazi_new

cp ../configure* .
cp ../Makefile* .

cp ../src/*.hpp ./src/.
cp ../src/*.cpp ./src/.
cp ../src/Makefile* ./src/.
cp ../src/*.in ./src/.

cp ../example/Makefile* ./example/.
cp ../example/ModeLaplace/Makefile* ./example/ModeLaplace/.
cp ../example/ModeLaplace/*.cpp ./example/ModeLaplace/.
cp ../example/ModeLaplace/*.h ./example/ModeLaplace/.
cp ../example/BlockKrylovSchur/Makefile* ./example/BlockKrylovSchur/.
cp ../example/BlockKrylovSchur/*.cpp ./example/BlockKrylovSchur/.

cp ../test/Makefile* ./test/.
cp ../test/ModalSolverUtils/Makefile* ./test/ModalSolverUtils/.
cp ../test/ModalSolverUtils/*.cpp ./test/ModalSolverUtils/.

# Remove files from main so that they are not commited
./remove_from_main.x
