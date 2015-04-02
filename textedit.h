
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

	short marku[2];
	short markv[2];
	int mark[2];
	int nmark;

	Editop *ops;
	int nops, aops;

	int fontsize;

	int drag;
	int select;
	short dragu0;
	short dragv0;

	short seloff[2];
	short sel0[2];
	short sel1[2];
};

void textedit(Textedit *texp, Input *inp, Input *inep);
void inittextedit(Textedit *texp, Image *dst, Rect dstr, int pad, char *text, int ntext);
