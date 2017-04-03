# FindFile
File search utility for Windows

Mundi Software 1997..2016 - FindFile - Freeware Version 4.2
Syntax: ff [-option] [[disc:][directory\] | variable:] ... [mask] ... [; command] ...
Options:
   h   help (this information)    q   quiet execute
   a   display attributes         b   brief format of information
   v   only visible               d   search only directory names
   s   search in subdirectories   x   Search also directory names
   l   do not search in links     i   display summary information
   t   do not execute commands    p   request confirmation
   w   do not watch the location  r   display relative paths
   f.. file limit (default 1)     e.. disk or directory error limit
Using found filenames in commands:
   !*  full path
   !@  disc and directory         !$  filename with extension
   !:  disc                       !\  directory
   !   filename                   .!  extension
   !#  file number                !!  character '!'
Examples:
   ff -sip \*.bak *.tmp ; del "!:\!\!.!"
   ff -sq *.tmp ; echo !#: !* ; attrib -R -S -H "!*" ; del "!*"
   ff -sq \win*\ \prog*\ *.txt ; type "!*" >> info.txt
   dir /b | sort | ff /a
   dir /b > tmp & ff < tmp & del tmp
   ff -a PATH: | sort /+18 | more
