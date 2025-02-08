#!/bin/bash

cd build && cmake -DCMAKE_C_FLAGS="-Wall -Wextra -std=c99 -g" .. && cmake --build .
