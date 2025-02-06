#!/bin/bash

cd build && cmake -DCMAKE_C_FLAGS="-g" .. && cmake --build .
