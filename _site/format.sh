#! /bin/bash -eux

for f in $(find . -type f | grep -v .git | grep -v _site | grep ".*md$"); do
    npx --yes prettier@2.8.8 --write ${f}
done
git diff --exit-code
