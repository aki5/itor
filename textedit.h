
typedef struct Textedit Textedit;
typedef struct Editop Editop;

struct Editop {
	int opcode;
	int mark;
	char *str;
	int len;
};

struct Textedit {
	Image *dst;

	Rect dstr;

	int pad;

	char *text;
	int ntext, atext;

	int uoff, voff;

	char *clip;
	int nclip, aclip;

	Image *selcolor;
	Image *fgcolor;

	intcoord marku[2];
	intcoord markv[2];
	int mark[2];
	int nmark;

	Editop *ops;
	int nops, aops;

	int fontsize;

	int drag;
	int select;
	intcoord dragu0;
	intcoord dragv0;

	intcoord seloff[2];
	intcoord sel0[2];
	intcoord sel1[2];
};

void textedit(Textedit *texp, Input *inp, Input *inep);
void inittextedit(Textedit *texp, Image *dst, Rect dstr, int pad, char *text, int ntext);
