
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
	int sel;
	Rect r;

	sel = 0;

	Rect lr;
	int code, off;
	char *sp;

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
			lr = rect(r.u0, r.v0, r.u0+3*fontem(), r.v0+linespace());
			break;
		default:
			lr = drawchar(&screen, r, code, fgcolor);
			break;
		}
		if(nmark < 2 && ptinrect(sel0, &lr)){
			sel ^= 1;
			mark[nmark++] = sp-text;
		}
		if(nmark < 2 && ptinrect(sel1, &lr)){
			sel ^= 1;
			mark[nmark++] = sp-text;
		}
		if(nmark == 2){
			if(mark[0] == sp-text)
				sel = 1;
			if(mark[1] == sp-text)
				sel = 0;
		}
		if(sel)
			drawblend_back(&screen, lr, selcolor);

		if(code == '\n'){
			r.u0 = dstr.u0 + uoff;
			r.uend = dstr.uend;
			r.v0 += linespace();
			r.vend += linespace();
		} else {
			r.u0 += rectw(&lr);
		}
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
	setfontsize(16);

	fgcolor = allocimage(rect(0,0,1,1), color(150, 200, 80, 255));
//	bgcolor = allocimage(rect(0,0,1,1), color(255,255,255,255));
	bgcolor = allocimage(rect(0,0,1,1), color(50,60,50,255));

	selcolor = allocimage(rect(0,0,1,1), color(90,100,80,255));

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
			if(mark[0] > mark[1])
				abort();
			if(keystr(inp, "x") && (inp->on & KeyControl) != 0){
				if(mark[0] < mark[1]){
					memmove(text+mark[0], text+mark[1], ntext-mark[1]);
					ntext -= mark[1]-mark[0];
					mark[1] = mark[0];
					sel0[0] = 0;
					sel0[1] = 0;
					sel1[0] = 0;
					sel1[1] = 0;
				}
			}
			if(keystr(inp, "v") && (inp->on & KeyControl) != 0)
				printf("PASTE!\n");
			if((keystr(inp, "q") || keystr(inp, "c")) && (inp->on & KeyControl) != 0)
				exit(0);

			if(mark[0] == mark[1]){
				if((inp->on & KeyControl) == 0){
					if(inp->begin & KeyStr){
						int off;
						off = strlen(inp->str);
						if(ntext+off >= atext){
							atext += atext;
							text = realloc(text, atext);
						}
						memmove(text+mark[0]+off, text+mark[0], ntext-mark[0]);
						ntext += off;
						memcpy(text+mark[0], inp->str, off);
						mark[0] = mark[1] = mark[0]+off;
					}
					if(inp->begin & KeyBackSpace){
						if(mark[0] > 0){
							memmove(text+mark[0]-1, text+mark[0], ntext-mark[0]);
							ntext--;
							mark[0] = mark[1] = mark[0]-1;
						}
					}
				}
			} else {
				char *p, *q, *ep;
				if(keystr(inp, "\t")){
					int code, off;
					ep = text+mark[1];
					for(q = p = text+mark[0]; p < ep; p += off){
						code = utf8decode(p, &off, ep-p);
					} 
				}
			}

			if(select == 0 && mousebegin(inp) == Mouse1){
				printf("select begin %d,%d!\n", inp->xy[0], inp->xy[1]);
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
				printf("select end: %d,%d,%d,%d!\n", sel0[0], sel0[1], sel1[0], sel1[1]);
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
				voff -= 30;
			}
			if(mousebegin(inp) == Mouse5){
				voff += 30;
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

			/* blend background in */
			drawblend_back(&screen, screen.r, bgcolor);
		}
	}

	return 0;
}

