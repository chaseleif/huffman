#! /bin/bash

./hfc -b $1 tree.nfo
python makedot.py tree.nfo
dot -Tpng tree.dot -o $1.png
rm tree.nfo
rm tree.dot
echo "Created tree image $1.png from input file $1"
