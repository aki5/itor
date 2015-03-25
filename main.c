/*
 *	Itor is a "what you get is what you get" editor
 */
#include "os.h"
#include "draw3.h"

typedef struct Line Line;
struct Line {
	Line *next;
	Line *prev;
	char *sp, *ep;
	int sel;
	Rect r;
};

int uoff, voff;

Line *line0;
Line *lineend;

char *clip;
int nclip, aclip;

char *text;
int ntext, atext;

Image *selcolor;
Image *fgcolor;
Image *bgcolor;

int mark[2];
int nmark;

void
drawtext(Rect dstr, short *sel0, short *sel1)
{
	int insel, dx;
	Rect r;

	insel = 0;

	Rect lr;
	int code, off;
	char *sp;

	/* add a phony newline, helps with selection */
	if(ntext == atext){
		atext++;
		text = realloc(text, atext);
	}
	//text[ntext++] = '\n';

	r.u0 = dstr.u0 + uoff;
	r.v0 = dstr.v0 + voff;
	r.uend = dstr.uend;
	r.vend = r.v0 + linespace();

	for(sp = text; sp < text+ntext; sp += off){
		code = utf8decode(sp, &off, text+ntext-sp);
		if(code == -1){
			off = 1;
			continue;
		}
		switch(code){
		case '\n':
			lr = r;
			lr.uend = 32767;
			break;
		case '\t':
			dx = 3*fontem();
			lr = rect(r.u0, r.v0, r.u0+dx, r.v0+linespace());
			break;
		default:
			lr = drawchar(&screen, r, code, fgcolor);
			break;
		}
		if(nmark < 2 && ptinrect(sel0, &lr))
			mark[nmark++] = sp-text;
		if(nmark < 2 && ptinrect(sel1, &lr))
			mark[nmark++] = sp-text;

		if(nmark > 0 && mark[0] == sp-text){
			drawrect(&screen, rect(lr.u0-1, lr.v0, lr.u0+1, lr.vend), color(255,255,255,255));
			insel = 1;
		}
		if(nmark > 1 && mark[1] == sp-text)
			insel = 0;

		if(insel)
			blend_add_under(&screen, lr, selcolor);

		if(code == '\n'){
			r.u0 = dstr.u0 + uoff;
			r.uend = dstr.uend;
			r.v0 += linespace();
			r.vend += linespace();
		} else {
			r.u0 += rectw(&lr);
		}
	}
	if(nmark < 2){
		mark[nmark++] = sp-text;
	}
	if(nmark < 2){
		mark[nmark++] = sp-text;
	}
	if(mark[0] == sp-text)
		drawrect(&screen, rect(r.u0-1, r.v0, r.u0+1, r.vend), color(255,255,255,255));
}

enum {
	OpInsert = 1,
	OpErase = 2,
};

struct {
	int op;
	int mark;
	char *str;
	int len;
} *ops;
int nops, aops;

void
addop(int op, int mark, char *str, int len)
{
	if(nops >= aops){
		aops = nops + 128;
		ops = realloc(ops, aops * sizeof ops[0]);
	}
	ops[nops].op = op;
	ops[nops].mark = mark;
	ops[nops].str = malloc(len);
	memcpy(ops[nops].str, str, len);
	ops[nops].len = len;
	nops++;
}

void
insert(int mark0, char *str, int len, int is_undo)
{
	if(!is_undo)
		addop(OpInsert, mark0, str, len);
	if(ntext+len >= atext){
		atext = ntext+len;
		text = realloc(text, atext);
	}
	memmove(text+mark0+len, text+mark0, ntext-mark0);
	ntext += len;
	memcpy(text+mark0, str, len);
}

void
erase(int mark0, int mark1, int is_undo)
{
	if(!is_undo)
		addop(OpErase, mark0, text+mark0, mark1-mark0);
	memmove(text+mark0, text+mark1, ntext-mark1);
	ntext -= mark1-mark0;
}

void
undo(void)
{
	if(nops > 0){
		if(ops[nops-1].op == OpErase)
			insert(ops[nops-1].mark, ops[nops-1].str, ops[nops-1].len, 1);
		if(ops[nops-1].op == OpInsert)
			erase(ops[nops-1].mark, ops[nops-1].mark+ops[nops-1].len, 1);
		nops--;
	}
}

int
main(int argc, char *argv[])
{
	Input *inp, *einp;

	drawinit(800,800);
	initdrawstr("/usr/share/fonts/truetype/droid/DroidSans.ttf"); //-Bold
	//initdrawstr("/usr/share/fonts/truetype/freefont/FreeMono.ttf"); 
	//initdrawstr("/usr/share/fonts/truetype/freefont/FreeSans.ttf"); 
	setfontsize(12);

//	fgcolor = allocimage(rect(0,0,1,1), color(0, 0, 0, 255));
//	fgcolor = allocimage(rect(0,0,1,1), color(170, 170, 170, 255));
//	fgcolor = allocimage(rect(0,0,1,1), color(255, 255, 255, 255));

//	bgcolor = allocimage(rect(0,0,1,1), color(255,255,255,255));
//	bgcolor = allocimage(rect(0,0,1,1), color(40,40,40,255));
	fgcolor = allocimage(rect(0,0,1,1), color(150, 200, 80, 255));
	selcolor = allocimage(rect(0,0,1,1), color(90,100,80,255));
	bgcolor = allocimage(rect(0,0,1,1), color(0,40,40,255));

	if(argc > 0){
		int fd;
		int n;
		fd = open(argv[1], O_RDONLY);
		ntext = 0;
		atext = 1024;
		text = malloc(atext);
		while((n = read(fd, text+ntext, atext-ntext)) > 0){
			ntext += n;
			if(ntext >= atext){
				atext += atext;
				text = realloc(text, atext);
			}
		}
		close(fd);
	}

	//initlayout();

	int drag = 0;
	int select = 0;
	int repaint;
	int dragu0, dragv0;
	int mouse, m;

	short sel0[2] = {0};
	short sel1[2] = {0};

//drawanimate(1);
	for(;;){
		repaint = 0;
		for(inp = drawevents(&einp); inp < einp; inp++){
			if(nmark == 2 && mark[0] > mark[1])
				abort();
			if(keystr(inp, "z") && (inp->on & KeyControl) != 0){
				undo();
			}
			if(keystr(inp, "x") && (inp->on & KeyControl) != 0){
				if(mark[0] < mark[1]){
					if(aclip < mark[1]-mark[0]){
						aclip = mark[1]-mark[0];
						clip = realloc(clip, aclip);
					}
					memcpy(clip, text+mark[0], mark[1]-mark[0]);
					nclip = mark[1]-mark[0];

					erase(mark[0], mark[1], 0);

					mark[1] = mark[0];
					sel0[0] = 0;
					sel0[1] = 0;
					sel1[0] = 0;
					sel1[1] = 0;
				}
			}
			if(keystr(inp, "v") && (inp->on & KeyControl) != 0){
				insert(mark[0], clip, nclip, 0);
			}
			if((keystr(inp, "q") || keystr(inp, "c")) && (inp->on & KeyControl) != 0)
				exit(0);

			if(mark[0] == mark[1]){
				if((inp->on & KeyControl) == 0){
					if(inp->begin & KeyStr){
						int len;
						len = strlen(inp->str);
						insert(mark[0], inp->str, len, 0);
						mark[0] = mark[1] = mark[0]+len;
					}
					if(inp->begin & KeyBackSpace){
						if(mark[0] > 0){
							if(ntext > 0){
								erase(mark[0]-1, mark[0], 0);
								mark[0] = mark[1] = mark[0]-1;
							}
						}
					}
				}
				if(keypress(inp, KeyLeft)){
					if(mark[0] > 0)
						mark[0]--;
					mark[1] = mark[0];
				}
				if(keypress(inp, KeyRight)){
					if(mark[0] < ntext)
						mark[0]++;
					mark[1] = mark[0];
				}
			} 

			if(select == 0 && mousebegin(inp) == Mouse1){
				sel0[0] = inp->xy[0]-uoff;
				sel0[1] = inp->xy[1]-voff;
				sel1[0] = inp->xy[0]-uoff;
				sel1[1] = inp->xy[1]-voff;
				select = 1;
				nmark = 0;
			}
			if(select && mousemove(inp) == Mouse1){
				sel1[0] = inp->xy[0]-uoff;
				sel1[1] = inp->xy[1]-voff;
				nmark = 0;
			}
			if(select && mouseend(inp) == Mouse1){
				sel1[0] = inp->xy[0]-uoff;
				sel1[1] = inp->xy[1]-voff;
				select = 0;
				nmark = 0;
			}

			if(mousebegin(inp) == Mouse2){
				printf("mouse2!\n");
			}

			if(drag == 0 && (m = mousebegin(inp)) == Mouse3){
				mouse = m;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				drag = 1;
			}
			if(drag && mousemove(inp) == Mouse3){
				uoff += inp->xy[0] - dragu0;
				voff += inp->xy[1] - dragv0;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
			}
			if(drag && mouseend(inp) == Mouse3){
				uoff += inp->xy[0] - dragu0;
				voff += inp->xy[1] - dragv0;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				drag = 0;
			}

			if(mousebegin(inp) == Mouse4){
				voff -= linespace();
				if(select){
					sel1[1] += linespace();
					nmark = 0;
				}
			}
			if(mousebegin(inp) == Mouse5){
				voff += linespace();
				if(select){
					sel1[1] -= linespace();
					nmark = 0;
				}
			}

		}
		if(1){
			/* clear to transparent */
			drawrect(&screen, screen.r, color(0, 0, 0, 0));

			Rect textr;
			textr = screen.r;
			textr.u0 += 10;
			textr.v0 += 10;
			textr.uend -= 10;
			textr.vend -= 10;
			short sel0fix[2], sel1fix[2];
			sel0fix[0] = sel0[0] < textr.u0 ? textr.u0+uoff : (sel0[0] + uoff);
			sel0fix[1] = sel0[1] < textr.v0 ? textr.v0+voff : (sel0[1] + voff);
			sel1fix[0] = sel1[0] < textr.u0 ? textr.u0+uoff : (sel1[0] + uoff);
			sel1fix[1] = sel1[1] < textr.v0 ? textr.v0+voff : (sel1[1] + voff);
			drawtext(textr, sel0fix, sel1fix);

			/* put background in */
			blend_add_under(&screen, screen.r, bgcolor);
		}
	}

	return 0;
}

