#! /bin/bash

./hfc -b $1 _temptree.nfo
python makedot.py _temptree.nfo
dot -Tpng _temptree.dot -o $1.png
rm -f _temptree.nfo _temptree.dot
echo "Created tree image $1.png from input file $1"
