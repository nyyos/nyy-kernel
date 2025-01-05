#!/bin/fish
for el in $(make all run|rg 0xffffff);
      addr2line -e build/kernel/platform/amd64/nyy $el
end
