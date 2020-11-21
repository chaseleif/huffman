#! /usr/bin/env python

import sys

if len(sys.argv)!=2 and len(sys.argv)!=3:
    print('This utility creates a dot file from the HFC output')
    print('Usage:\tpython '+sys.argv[0]+' tree.nfo')
    print('   or:\tpython '+sys.argv[0]+' tree.nfo rootnodename')
    print('A dot file with the same prefix will be created')
    exit(0)

infilename = sys.argv[1]
outfilename = infilename.split('.')[0] + '.dot'

# don't print the valnode shape, enlarge the text
# set to True to enable this, something else to disable
# used this to make better images for squashed down graphs in docs
bigtextnodevals = False

'''
graphviz docs say they take 72 points as an inch...
portrait, 8.5x11, do 7.5x10 which gives .5" margin...
should be W=540 x H=720, this gave me 10"x13.33" or 720x960 pix, and it cut off everything outside
I just made smaller images using the -Gdpi flag
'''

printhex = True

printsymbols = ['~','`','!','@','#','$','%','^','&','*','(',')',' ',',','.','<','>','?',';',':','[',']','{','}','|','-','_','=','+']

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
        outfile.write('digraph hufftree {\ngraph [ splines = \"line\", root = 0')
        outfile.write('];\n')
        midnodes = {}
        for i in valnodes:
            parent = int((i-1)/2)
            label = ''
            if (parent*2)+2==i:
                label = '1'
            else:
                label = '0'
            outfile.write(str(parent)+' -> '+str(i)+' [label = \"'+label+'\", arrowhead = \"inv\"];\n')
            while True:
                if parent in parentlist:
                    parentlist[parent]+=valnodes[i][0]
                else:
                    parentlist[parent]=valnodes[i][0]
                if parent==0:
                    break
                parent = int((parent-1)/2)
            nodedescriptions+=str(i)+' [label = \"'+str(valnodes[i][0])+'\\n'
            testchar = chr(int(valnodes[i][1], 16))
            if printhex is not True and (testchar.isalnum() or testchar in printsymbols):
                nodedescriptions+='\''+testchar+'\''
            else:
                nodedescriptions+='0x'+valnodes[i][1]
            nodedescriptions+='\\n'+valnodes[i][2]+'\"'
            if bigtextnodevals==True:
                nodedescriptions+=', shape=\"none\", fontsize=24'
            nodedescriptions+='];\n'
        for parent in parentlist:
            leftchild = (parent*2)+1
            rightchild = (parent*2)+2
            if leftchild not in valnodes:
                outfile.write(str(parent)+' -> '+str(leftchild)+' [label = \"0\", arrowhead = \"invempty\"];\n')
            if rightchild not in valnodes:
                outfile.write(str(parent)+' -> '+str(rightchild)+' [label= \"1\", arrowhead = \"invempty\"];\n')
            nodedescriptions+=str(parent)+' [label = \"'+str(parentlist[parent])+'\"'
            if parent==0:
                if len(sys.argv)==3:
                    nodedescriptions+=', xlabel = \"'+sys.argv[2]+'\"'
                else:
                    nodedescriptions+=', xlabel = \"Huffman tree root\"'
            nodedescriptions+='];\n'
        outfile.write(nodedescriptions + '}')
    return valnodes

valnodes = makedotfile(infilename,outfilename)
if len(valnodes)>0:
    print('Read '+infilename+' and created '+outfilename)
else:
    print('Hrm')
