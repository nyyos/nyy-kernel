#!/bin/bash

in="$1"
out="$2"

nm -n "$in" | grep -v '.* [aNUw] ' > "$out"
