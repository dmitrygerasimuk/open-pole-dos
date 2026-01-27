#!/bin/bash
gftp -n <<EOF
open 192.168.1.8
user dmitry
binary
prompt
cd e/dos/polec/
mput *.exe
mput *.EXE
mput openpole.sym 
mput POLE.OVL
EOF
