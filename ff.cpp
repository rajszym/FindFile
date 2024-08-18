/******************************************************************************

    FindFile - Copyright (C) 1997 - 2021 Mundi Software.

    FindFile is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation; either version 3 of the License,
    or (at your option) any later version.

    FindFile is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.

******************************************************************************/

#include <windows.h>
#include <string.h>
#include <ctype.h>
#include <cstdio>
#include <algorithm>

#ifdef __GNUC__
int _CRT_glob = 0;
#endif

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#define FA_RDONLY FILE_ATTRIBUTE_READONLY            /* 0x00000001 */
#define FA_HIDDEN FILE_ATTRIBUTE_HIDDEN              /* 0x00000002 */
#define FA_SYSTEM FILE_ATTRIBUTE_SYSTEM              /* 0x00000004 */
#define FA_SUBDIR FILE_ATTRIBUTE_DIRECTORY           /* 0x00000010 */
#define FA_ARCHIV FILE_ATTRIBUTE_ARCHIVE             /* 0x00000020 */
#define FA_DEVICE FILE_ATTRIBUTE_DEVICE              /* 0x00000040 */
#define FA_NORMAL FILE_ATTRIBUTE_NORMAL              /* 0x00000080 */
#define FA_TEMPOR FILE_ATTRIBUTE_TEMPORARY           /* 0x00000100 */
#define FA_SPARSE FILE_ATTRIBUTE_SPARSE_FILE         /* 0x00000200 */
#define FA_REPARS FILE_ATTRIBUTE_REPARSE_POINT       /* 0x00000400 */
#define FA_COMPRS FILE_ATTRIBUTE_COMPRESSED          /* 0x00000800 */
#define FA_OFFLIN FILE_ATTRIBUTE_OFFLINE             /* 0x00001000 */
#define FA_NOINDX FILE_ATTRIBUTE_NOT_CONTENT_INDEXED /* 0x00002000 */
#define FA_ENCRPT FILE_ATTRIBUTE_ENCRYPTED           /* 0x00004000 */
#define FA_VIRTUA FILE_ATTRIBUTE_VIRTUAL             /* 0x00010000 */

#define FA_HIDSYS (FA_HIDDEN | FA_SYSTEM)
#define FA_FLAGS  (FA_HIDSYS | FA_RDONLY | FA_REPARS)

#define _MAX_CMD  (1024)

bool  accept_f = false, // request confirmation
      query_f  = false, // ask for a file
      attrib_f = false, // display attributes
      info_f   = false, // dispaly summary information
      quiet_f  = false, // quiet execution
      watch_f  = true,  // watch the location
      dirs_f   = false, // search only directories
      subdir_f = false, // recurse subdirectories
      others_f = false, // reversed search
      relat_f  = false, // display relative paths
      brief_f  = false, // display brief information
      test_f   = false, // test - do not execute commands
      error_f  = false, // file or directory error
      break_f  = false, // break the confirmation
      unix_f   = false; // unix path style

uint  flags = FA_FLAGS;

uint  screenwidth  = 0,
      screenheight = 0;

char *fn [64] = {},
     *dn [64] = {},
     *cmd[64] = {};

uint  files = 0,
      dirs  = 0,
      cmds  = 0;

ulong elimit = -1UL,
      flimit = -1UL;

ulong xfiles       = 0,
      xdirs        = 0,
      xbytes       = 0,
      xacceptfiles = 0,
      xacceptbytes = 0;

char  path [_MAX_PATH]  = { 0 },
      drive[_MAX_DRIVE] = { 0 },
      dir  [_MAX_DIR]   = { 0 },
      file [_MAX_FNAME] = { 0 },
      ext  [_MAX_EXT]   = { 0 };

inline char* strend (char *s) { s = strchr(s, 0); return s; }

inline char* strlast(char *s) { if (*s) s = strend(s) - 1; return s; }

inline char* strdec (char *s) { *strlast(s) = 0; return s; }

inline char* strinc (char *s, char c) { *(word*)strend(s) = (word)(byte)c; return s; }

inline char* strins (char *s, char c) { s = strend(s); *s = c; return s + 1; }

inline char* strrep (char *s, char c, char r) {	for (char *p = s; (p = strchr(p, c)); *p = r); return s; }

inline char* strtrim(char *s) { while (isspace(*s)) s++; for (char *p = strlast(s); isspace(*p); *p-- = 0); return s; }

void getscreensize()
{
	CONSOLE_SCREEN_BUFFER_INFO c;
	HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
	GetConsoleScreenBufferInfo(h, &c);
	screenwidth  = static_cast<uint>(c.dwSize.X);
	screenheight = static_cast<uint>(c.dwSize.Y);
}

void clreol()
{
	CONSOLE_SCREEN_BUFFER_INFO c;
	HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
	GetConsoleScreenBufferInfo(h, &c);
	DWORD x = static_cast<DWORD>(c.dwSize.X - c.dwCursorPosition.X);
	FillConsoleOutputCharacter(h, ' ', x, c.dwCursorPosition, &x);
}

void dellines()
{
	CONSOLE_SCREEN_BUFFER_INFO c;
	HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
	GetConsoleScreenBufferInfo(h, &c);
	DWORD x = static_cast<DWORD>(c.dwSize.X * 2);
	c.dwCursorPosition.X = 0; c.dwCursorPosition.Y -= 1;
	SetConsoleCursorPosition(h, c.dwCursorPosition);
	FillConsoleOutputCharacter(h, ' ', x, c.dwCursorPosition, &x);
}

char getch()
{
	DWORD x;
	INPUT_RECORD i;
	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	while (!ReadConsoleInput(h, &i, 1, &x) || i.EventType != KEY_EVENT || !i.Event.KeyEvent.bKeyDown);
	return i.Event.KeyEvent.uChar.AsciiChar;
}

void syntax()
{
	fprintf(stderr,
		"\n"
		"Mundi Software 1997..2021 - FindFile - Freeware Version 4.6\n"
		"Syntax: ff [-options | --] [[disc:][directory\\] | variable:] ... [mask] ... [; command] ...\n"
		"Options:\n"
		"   h   help (this information)    q   quiet execution\n"
		"   a   display attributes         b   brief format of information\n"
		"   v   only visible               d   search only directories\n"
		"   s   recurse subdirectories     x   search also directories\n"
		"   n   reversed search (files/directories that do not match the mask)\n"
		"   l   do not search in links     i   display summary information\n"
		"   t   do not execute commands    p   request confirmation\n"
		"   w   do not watch the location  r   display relative paths\n"
		"   f.. file limit (default 1)     e.. disk or directory error limit\n"
		"Using found filenames in commands:\n"
		"   !*  full path\n"
		"   !@  disc and directory         !$  filename with extension\n"
		"   !:  disc                       !\\  directory\n"
		"   !   filename                   .!  extension\n"
		"   !#  file number                !!  character '!'\n"
		"Examples:\n"
		"   ff -sip \\*.bak *.tmp ; del \"!:\\!\\!.!\"\n"
		"   ff -sq *.tmp ; echo !#: !* ; attrib -R -S -H \"!*\" ; del \"!*\"\n"
		"   ff -sq \\win*\\ \\prog*\\ *.txt ; type \"!*\" >> info.txt\n"
		"   dir /b | sort | ff /a\n"
		"   dir /b > tmp & ff < tmp & del tmp\n"
		"   ff -a PATH: | sort /+18 | more\n"
		"   ff /sn *.c *.cpp *.h *.hpp\n"
	);
	exit(EXIT_FAILURE);
}

void errorinfo(const char *s, char *i)
{
	if ( elimit) elimit--;
	if (!elimit || info_f) fprintf(stderr, "Error: %s \"%s\"\n", s, i);
	if (!elimit) exit(EXIT_FAILURE);
	error_f = true;
}

void errorexit(const char *s)
{
	fprintf(stderr, "Error: %s\n", s);
	exit(EXIT_FAILURE);
}

void putattributes(WIN32_FIND_DATA *fd, bool v = true)
{
	DWORD x = fd->dwFileAttributes;
	char a[] = "rhsvda            "; if (x & FA_REPARS) a[4] = 'l';
	for (int i = 0; i < 6; i++, x >>= 1) if (!(x & 1)) a[i] = '-';
	if (!(fd->dwFileAttributes & FA_SUBDIR)) sprintf(a + 7, "%10lu ", fd->nFileSizeLow);
	if (v) printf(        "%s", a);
	else  fprintf(stderr, "%s", a);
}

void putpath(bool v = true)
{
	char p[_MAX_PATH]; strcpy(p, path);
	size_t n = strlen(p);
	if (unix_f)
	{
		std::replace(p, p + n, '\\', '/');
		if (n >= 2 && p[1] == ':') { p[1] = p[0]; p[0] = '/'; }
	}
	++n;
	if (v && (attrib_f || brief_f)) n += 18;
	if (n > screenwidth) n = n - screenwidth + 3; else n = 0;
	if (v) printf(        "...%s\n" + (n ? 0 : 3), p + n);
	else  fprintf(stderr, "...%s\r" + (n ? 0 : 3), p + n);
}

ulong getnum(char **s)
{
	ulong l = 0;
	do l = 10 * l + (ulong)(*(*s)++ - '0'); while (isdigit(**s));
	return l;
}

bool acceptfile(WIN32_FIND_DATA *fd) // nazwa pliku w zmiennej 'path'
{
	if (attrib_f) putattributes(fd, false);
	putpath(false);
	fprintf(stderr, "\nConfirmation (Yes/No/All/Break) ?");
	for (;;)
		switch (toupper(getch()))
		{
		case 'P': flimit = xfiles; break_f = true; [[fallthrough]];
		case 'N': dellines();        return false;
		case 'W': query_f = false;                 [[fallthrough]];
		case 'T': dellines();        return  true;
		}
}

void createcommand(char *s)
{
	bool dot;
	char temp[_MAX_CMD];
	s = strtrim(s);
	for (dot = false, *temp = 0; *s; s++)
	{
		if (*s == '!')
			switch (*++s)
			{
			case '!': strcat(temp, "!");   break;
			case ':': strcat(temp, drive); break;
			case '/': [[fallthrough]];
			case '\\':
				strcat(temp, dir);
				if (dir[1] && strchr(" \t,:;", s[1])) strdec(temp);
				break;
			case '*': strcat(temp, path); break;
			case '@':
				strcat(temp, drive);
				strcat(temp, dir);
				if (dir[1] || s[1] == '\\') strdec(temp);
				break;
			case '$':
				strcat(temp, file);
				strcat(temp, ext);
				break;
			case '#': sprintf(strend(temp), "%lu", xfiles); break;
			default:
				s--;
				if (dot) strcat(strdec(temp), ext);
				else     strcat(temp, file);
				break;
			}
		else
			strinc(temp, *s);
		dot = (*s == '.');
	}
	if (*temp)
	{
		if (!quiet_f) fprintf(stderr, "[%s]\n", temp);
		if (!test_f ) system(temp);
	}
}

void execute(WIN32_FIND_DATA *fd) // filename in 'path' variable
{
	if (accept_f)
	{
		if (query_f && !acceptfile(fd)) return;
		xacceptfiles++; xacceptbytes += fd->nFileSizeLow;
	}
	if (!quiet_f && !brief_f)
	{
		if (attrib_f) putattributes(fd);
		putpath();
	}
	_splitpath(path, drive, dir, file, ext);
	for (uint i = 0; i < cmds; i++)
	{
		createcommand(cmd[i]);
	}
}

bool findvalid(WIN32_FIND_DATA *fd)
{
	if (xfiles >= flimit) return false;

	if ((fd->dwFileAttributes & FA_HIDSYS) && !(flags & FA_HIDSYS)) return false;
	if ((fd->dwFileAttributes & FA_SUBDIR) && !(flags & FA_SUBDIR)) return false;
	if ( fd->dwFileAttributes & FA_DEVICE) return false;
	if ( fd->dwFileAttributes & FA_SUBDIR)
	{
		if ((*(ulong*)(fd->cFileName) & 0x00FFFFFF) == 0x00002E2E) return false; // ".."
		if ((*(ulong*)(fd->cFileName) & 0x0000FFFF) == 0x0000002E) return false; // "."
	}
	else
	{
		if (dirs_f) return false;
	}

	return true;
}

bool filevalid(char *p, char *f)
{
	HANDLE fh;
	WIN32_FIND_DATA fd;
	for (uint i = 0; i < files; i++)
	{
		strcpy(p, fn[i]);
		fh = FindFirstFile(path, &fd);
		if (fh != INVALID_HANDLE_VALUE)
		{
			do
				if (findvalid(&fd) && strcmp(fd.cFileName, f) == 0)
				{
					FindClose(fh);
					return false;
				}
			while (FindNextFile(fh, &fd));
			FindClose(fh);
		}
	}
	return true;
}

void findreversed(char *p) // p: nazwa pliku w zmiennej 'path'
{
	HANDLE fh;
	WIN32_FIND_DATA fd;
	strcpy(p, "*");
	fh = FindFirstFile(path, &fd);
	if (fh != INVALID_HANDLE_VALUE)
	{
		do
			if (findvalid(&fd) && filevalid(p, fd.cFileName))
			{
				xfiles++; xbytes += fd.nFileSizeLow;
				strcpy(p, fd.cFileName);
				execute(&fd);
			}
		while (FindNextFile(fh, &fd));
		FindClose(fh);
	}
}

void findfile(char *p) // p: nazwa pliku w zmiennej 'path'
{
	HANDLE fh;
	WIN32_FIND_DATA fd;
	for (uint i = 0; i < files; i++)
	{
		strcpy(p, fn[i]);
		fh = FindFirstFile(path, &fd);
		if (fh != INVALID_HANDLE_VALUE)
		{
			do
				if (findvalid(&fd))
				{
					xfiles++; xbytes += fd.nFileSizeLow;
					strcpy(p, fd.cFileName);
					execute(&fd);
				}
			while (FindNextFile(fh, &fd));
			FindClose(fh);
		}
	}
}

void findloop()
{
	HANDLE fh;
	WIN32_FIND_DATA fd;
	char  *p = strend(path);
	ulong xf = xfiles, xb = xbytes;
	if (!quiet_f && watch_f) { clreol(); putpath(false); }
	if (others_f) findreversed(p);
	else          findfile(p);
	if (xfiles > xf)
	{
		xdirs++;
		if (!quiet_f && brief_f)
		{
			printf("%6lu%c%10lu ", xfiles - xf, xfiles == flimit ? '?' : ' ', xbytes - xb);
			*p = 0; putpath();
		}
	}
	if (subdir_f && xfiles < flimit)
	{
		strcpy(p, "*");
		if ((fh = FindFirstFile(path, &fd)) != INVALID_HANDLE_VALUE)
		{
			do
				if (fd.dwFileAttributes & FA_SUBDIR)
				{
					if ((*(ulong*)(fd.cFileName) & 0x00FFFFFF) == 0x00002E2E) continue; // ".."
					if ((*(ulong*)(fd.cFileName) & 0x0000FFFF) == 0x0000002E) continue; // "."

					if ((fd.dwFileAttributes & FA_REPARS) && !(flags & FA_REPARS)) continue;

					strcpy(p, fd.cFileName);
					strinc(p, '\\');
					findloop();
				}
			while (FindNextFile(fh, &fd) && xfiles != flimit);
			FindClose(fh);
		}
	}
	if (watch_f) clreol();
}

void findpath()
{
	HANDLE fh;
	WIN32_FIND_DATA fd;
	char temp[_MAX_PATH], *p, *q, *t;

	if ((t = strpbrk(path, "*?")))
	{
		strcpy(temp, path); t += (uintptr_t)temp - (uintptr_t)path;
		p = strchr(temp, ':'); if (!p++) p = temp;
		while ((q = strchr(p, '\\')) < t) { p = q + 1; } *q = 0;
		if ((fh = FindFirstFile(temp, &fd)) != INVALID_HANDLE_VALUE)
		{
			*p = 0; *q = '\\';
			do
				if ((fd.dwFileAttributes & FA_SUBDIR) && *fd.cFileName != '.')
				{
					strcpy(path, temp);
					strcat(path, fd.cFileName);
					strcat(path, q);
					findpath();
				}
			while (FindNextFile(fh, &fd));
			FindClose(fh);
			return;
		}
	}

	if (GetLongPathName(path, path, _MAX_PATH) &&
	   (relat_f || GetFullPathName(path, _MAX_PATH, path, nullptr)))
		findloop();
	else
		errorinfo("incorrect path", path);
}

void findproc()
{
	error_f = false;
	if ( strchr(":", *strlast(path))) strinc(path, '.' );
	if (*strlast(path) != '\\')       strinc(path, '\\');
	findpath();
}

bool varloop(char *p)
{
	char *s;
	    strtok(p, ":");
	s = getenv(p); if (!s) { errorinfo("unknown variable", p); strins(p, ':'); return false; }
	p = strend(p); if (*++p != '\\') *--p = '\\';
	while (strtok(s, ";"))
	{
		strcpy(path, s); s = strins(s, ';');
		strcat(path, (*strlast(path) != '\\') ? p : p + 1);
		findproc();
	}
	return true;
}

void setcursor(BOOL visible)
{
	static CONSOLE_CURSOR_INFO cci = {};
	static HANDLE Cout = nullptr;
	if (Cout == NULL)
	{
		Cout = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleCursorInfo(Cout, &cci);
	}
	cci.bVisible = visible;
	SetConsoleCursorInfo(Cout, &cci);
}

void searchf()
{
	uint i, k = dirs;
	char temp[_MAX_PATH], *s;

	setcursor(FALSE);

	for (*temp = 0, i = 0; i < k; i++)
	{
		strrep(dn[i], '/', '\\');
		if ((s = strrchr(dn[i], '\\')) || (s = strchr(dn[i], ':')))
		{
			if (!*++s) continue;
			fn[files++] = strcpy(temp, s); *s = 0; i++;
		}
		for (dirs = i; i < k; i++)
			fn[files++] = dn[i];
	}

	if (!dirs ) dn[dirs++ ] = (char*)"";
	if (!files) fn[files++] = (char*)"*";

	for (i = 0; i < dirs && xfiles < flimit; i++)
	{
		s = strchr(dn[i], ':'); if (s > dn[i] + 1 && varloop(dn[i])) continue;
		strcpy(path, dn[i]);
		findproc();
	}

	setcursor(TRUE);
}

char*parseoptions(char *s)
{
	bool f;
	if (*s != '-' && *s != '/') return s;
	for (f = true; !strchr(" \t;", *++s); f = false)
		switch (toupper(*s))
		{
		case '?':
		case 'H': syntax(); [[fallthrough]];
		case 'A': attrib_f = true; break;
		case 'B': brief_f  = true; break;
		case 'I': info_f   = true; break;
		case 'P': accept_f =
		          query_f  = true; break;
		case 'Q': quiet_f  = true; break;
		case 'S': subdir_f = true; break;
		case 'R': relat_f  = true; break;
		case 'N': others_f = true; break;
		case 'V': flags   &= ~(uint)FA_HIDSYS; break;
		case 'D': dirs_f   = true; [[fallthrough]];
		case 'X': flags   |=  (uint)FA_SUBDIR; break;
		case 'L': flags   &= ~(uint)FA_REPARS; break;
		case 'T': test_f   = true; break;
		case 'W': watch_f  = false; break;
		case 'F': flimit   = isdigit(*++s) ? getnum(&s) : 1; s--; break;
		case 'E': elimit   = isdigit(*++s) ? getnum(&s) : 1; s--; break;
		case 'U': unix_f   = true; break;
		case '-': if (f && strchr(" \t;", *++s)) return s; [[fallthrough]];
		default : errorexit("unknown option");
		}
	if (f) errorexit("no options");
	return s;
}

char*parseheader(char *s)
{
	if (*s != ' ')
	{
		for (bool e = false; !strchr(e ? "" : " \t", *s); s++) if (*s == '\"') e = !e;
		while (isspace(*s)) s++;
		s = parseoptions(s);
	}
	while (isspace(*s)) s++;
	return s;
}

void parsecommandline(char *s)
{
	bool e;
	int  i;
	files = 0;
	dirs  = 0;
	s     = parseheader(s);

	while (!strchr(";", *s))
	{
		dn[dirs++] = s;
		for (e = false, i = 0; !strchr(e ? "" : " \t;", s[i]); s++)
		{
			if (s[i] == '\"' ) { e = !e; s--; i++; continue; }
			if (i) s[0] = s[i];
		}
		if      (i)           { *s = 0; s += i; }
		else if (isspace(*s)) { *s = 0; s += 1; }
		while   (isspace(*s)) s++;
	}

	if (*s)
		for (cmds = 0, i = 0, e = false; *s; s++)
			switch (*s)
			{
			case ';' : if (!e) { *s = 0; cmd[cmds++] = s + 1; } break;
			case '\"': e = !e;
			}
}

void processinput()
{
	char temp[_MAX_CMD * 2];
	char*line = temp;
	while (xfiles < flimit)
	{
		*line = ' ';
		if (!fgets(line + 1, _MAX_CMD - 1, stdin)) return;
		if (*strlast(line) == '\n') strdec(line);
		parsecommandline(line);
		if (dirs) searchf();
		if (cmds) line = temp + (line - temp + _MAX_CMD) % (_MAX_CMD * 2);
	}
}

const char *_end_(ulong x, const char *s1, const char *s2)
{
	return (x == 1) ? s1 : s2;
}

void showinfo()
{
		              fprintf(stderr, "\n");
	if (!xfiles)
	{
		              fprintf(stderr, "No files found");
	}
	else
	{
		{}            fprintf(stderr, "%lu file%s found",   xfiles, _end_(xfiles, "",  "s" ));
		if (subdir_f) fprintf(stderr, " in %lu director%s", xdirs,  _end_(xdirs,  "y", "ies"));
		{}            fprintf(stderr, " (%lu byte%s)",      xbytes, _end_(xbytes, "",  "s" ));
		if (accept_f)
		{
			if (!xacceptfiles)
			{
		              fprintf(stderr, "; %sconfirmed",         (xfiles == 1) ? "not " : "no files ");
			}
			else if (xacceptfiles == xfiles)
			{
		              fprintf(stderr, "; %sconfirmed",         (xfiles == 1) ? "" : "all files ");
			}
			else
			{
		              fprintf(stderr, "; %lu file%s confirmed", xacceptfiles, _end_(xacceptfiles, "", "s"));
		              fprintf(stderr, " (%lu byte%s)",          xacceptbytes, _end_(xacceptbytes, "", "s"));
			}
		}
	}
	if (break_f)
	{
		              fprintf(stderr, "; interrupted search");
	}
	else if (xfiles == flimit)
	{
		              fprintf(stderr, "; limited search");
	}
		              fprintf(stderr, "\n");
}

int main()
{
	getscreensize();
	parsecommandline(GetCommandLine());
	if (dirs)    searchf();
	else    processinput();
	if (info_f) showinfo();
	exit(xfiles ? EXIT_SUCCESS : EXIT_FAILURE);
}
