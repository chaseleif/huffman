#! /bin/bash

./hfc -b $1 _temptree.nfo
# name the root $3, if no $3 is passed the root will be "Huffman tree root"
python makedot.py _temptree.nfo $3
# full regular output size, don't specify -Gdpi
dot -Tpng _temptree.dot -o $2.png
# 50 results in smaller images, the Poe text fits in a landscape image
#dot -Tpng -Gdpi=50 _temptree.dot -o $2.png
# with 60 the Grimm text fits in a landscape image
#dot -Tpng -Gdpi=60 _temptree.dot -o $2.png
rm -f _temptree.nfo _temptree.dot
echo "Created tree image $2.png from input file $1"
