# FindFile
File search utility for Windows

Syntax:
```
ff [-options | --] [[disc:][directory\] | variable:] ... [mask] ... [; command] ...
```

Options:
```
h   help (this information)    q   quiet execution
a   display attributes         b   brief format of information
v   only visible               d   search only directories
s   search in subdirectories   x   search also directories
n   reversed search (files/directories that do not match the mask)
l   do not search in links     i   display summary information
t   do not execute commands    p   request confirmation
w   do not watch the location  r   display relative paths
f.. file limit (default 1)     e.. disk or directory error limit
```

Using found filenames in commands:
```
!*  full path
!@  disc and directory         !$  filename with extension
!:  disc                       !\  directory
!   filename                   .!  extension
!#  file number                !!  character '!'
```

Examples:
```
ff -sip \*.bak *.tmp ; del "!:\!\!.!"
ff -sq *.tmp ; echo !#: !* ; attrib -R -S -H "!*" ; del "!*"
ff -sq \win*\ \prog*\ *.txt ; type "!*" >> info.txt
dir /b | sort | ff /a
dir /b > tmp & ff < tmp & del tmp
ff -a PATH: | sort /+18 | more
ff /sn *.c *.cpp *.h *.hpp
```
