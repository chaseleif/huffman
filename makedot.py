#! /usr/bin/env python

import sys

if len(sys.argv)!=2:
    print('This utility creates a dot file from the HFC output')
    print(f'Usage:\tpython {sys.argv[0]} tree.nfo')
    print('A dot file with the same prefix will be created')
    exit(0)

infilename = sys.argv[1]
outfilename = infilename.split('.')[0] + '.dot'

def makedotfile(infilename='tree.nfo',outfilename='out.dot'):
    valnodes = {}
    for line in open(infilename,'r'):
        vals = line.rstrip().split(' ')
        nodei=0
        # get the index of each value node as an array
        for c in vals[2]:
            if c=='0': # go left
                nodei = (nodei<<1)+1
            elif c=='1': # go right
                nodei = (nodei<<1)+2
        valnodes[nodei] = (int(vals[0]),str(vals[1]),str(vals[2])) #valnodes[treeindex] = (freq,0xFF,010101101)
    parentlist = {}
    nodedescriptions = ''
    with open(outfilename,'w') as outfile:
        outfile.write('digraph hufftree {\ngraph [ splines = \"line\", root = 0];\n')
        midnodes = {}
        for i in valnodes:
            parent = int((i-1)/2)
            label = ''
            if (parent*2)+2==i:
                label = '1'
            else:
                label = '0'
            outfile.write(f'{parent} -> {i} [label = \"{label}\"];\n') #arrowType = "invempty",
            while True:
                if parent in parentlist:
                    parentlist[parent]+=valnodes[i][0]
                else:
                    parentlist[parent]=valnodes[i][0]
                if parent==0:
                    break
                parent = int((parent-1)/2)
            nodedescriptions+=f'{i} [label = \"{valnodes[i][0]}\\n0x{valnodes[i][1]}\\n{valnodes[i][2]}\"];\n'
        for parent in parentlist:
            leftchild = (parent*2)+1
            rightchild = (parent*2)+2
            if leftchild not in valnodes:
                outfile.write(f'{parent} -> {leftchild} [label = \"0\"];\n')
            if rightchild not in valnodes:
                outfile.write(f'{parent} -> {rightchild} [label= \"1\"];\n')
            nodedescriptions+=f'{parent} [label = \"{parentlist[parent]}\"];\n'
        outfile.write(nodedescriptions + '}')
    return valnodes

valnodes = makedotfile(infilename,outfilename)
if len(valnodes)>0:
    print(f'Read {infilename} and created {outfilename}')
else:
    print('Hrm')