// bin2c.c
//
// convert a binary file into a C source vector
//
// THE "BEER-WARE LICENSE" (Revision 3.1415):
// sandro AT sigala DOT it wrote this file. As long as you retain this notice you can do
// whatever you want with this stuff.  If we meet some day, and you think this stuff is
// worth it, you can buy me a beer in return.  Sandro Sigala
//
// syntax:  bin2c [-c] [-z] <input_file> <output_file>
//
//          -c    add the "const" keyword to definition
//          -z    terminate the array with a zero (useful for embedded C strings)
//
// examples:
//     bin2c -c myimage.png myimage_png.cpp
//     bin2c -z sometext.txt sometext_txt.cpp

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
    #define PATH_MAX 1024
#endif

int useconst = 0;
int zeroterminated = 0;

int myfgetc(FILE *f)
{
    int c = fgetc(f);
    if (c == EOF && zeroterminated)
    {
        zeroterminated = 0;
        return 0;
    }
    return c;
}

char *str2upr(char *s)
{
    char *r = (char *)calloc(strlen(s) + 1, sizeof(char));

    int i = 0;
    while(*s)
    {
        r[i] = (char)toupper(*s);
        ++i;
        ++s;
    }
    return r;
}

void process(const char *ifname, const char *ofname)
{
    FILE *ifile;
    FILE *ofile;
    if (fopen_s(&ifile, ifname, "rb") != 0)
    {
        fprintf(stderr, "cannot open %s for reading\n", ifname);
        exit(1);
    }
    if (fopen_s(&ofile, ofname, "wb") != 0)
    {
        fprintf(stderr, "cannot open %s for writing\n", ofname);
        exit(1);
    }
    char buf[PATH_MAX];
    char *p;
    const char *cp;
    if ((cp = strrchr(ifname, '/')) != NULL)
    {
        ++cp;
    }
    else
    {
        if ((cp = strrchr(ifname, '\\')) != NULL)
        {
            ++cp;
        }
        else
        {
            cp = ifname;
        }
    }
    strcpy_s(buf, PATH_MAX, cp);
    for (p = buf; *p != '\0'; ++p)
    {
        if (!isalnum(*p))
        {
            *p = '_';
        }
    }
    fprintf(ofile, "#ifndef %s_H\n#define %s_H\nstatic %sunsigned char %s[] = {\n    ",
            str2upr(buf), str2upr(buf), useconst ? "const " : "", buf);
    int c;
    int col = 5;
    while ((c = myfgetc(ifile)) != EOF)
    {
        if (col >= 78 - 5)
        {
            fprintf(ofile, "\n    ");
            col = 5;
        }
        fprintf(ofile, "0x%.2x,", c);
        col += 5;
    }
    fprintf(ofile, "\n};\n#endif\n");

    fclose(ifile);
    fclose(ofile);
}

void usage(void)
{
    fprintf(stderr, "usage: bin2c [-c|-z] <input_file> <output_file>\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    while (argc > 3)
    {
        if (!strcmp(argv[1], "-c"))
        {
            useconst = 1;
            --argc;
            ++argv;
        }
        else if (!strcmp(argv[1], "-z"))
        {
            zeroterminated = 1;
            --argc;
            ++argv;
        }
        else
        {
            usage();
        }
    }
    if (argc != 3)
    {
        usage();
    }
    process(argv[1], argv[2]);
    return 0;
}
