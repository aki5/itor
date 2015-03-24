
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

void
drawlines(Rect dstr, Line *ln, short *sel0, short *sel1)
{
	int sel;
	Rect r;
	r = dstr;
	r.v0 += voff;

	sel = 0;
	while(ln != NULL && r.v0 < dstr.vend){
		Rect lr;
		char *sp;
		r.u0 = dstr.u0 + uoff;
		for(sp = ln->sp; sp < ln->ep; sp++){
			lr = drawstr(&screen, r, sp, 1, color(150, 200, 80, 255));
			if(ptinrect(sel0, &lr))
				sel ^= 1;
			if(sel)
				drawblend_back(&screen, lr, selcolor);
			if(ptinrect(sel1, &lr))
				sel ^= 1;
			r.u0 += rectw(&lr);
		}
		r.v0 += linespace();
		ln = ln->next;
	}
	if(ln != NULL){
		ln->r = drawstr(&screen, r, ln->sp, ln->ep - ln->sp, color(150, 200, 80, 255));
		r.v0 += linespace();
	}
}

void
appendline(char *sp, char *ep)
{
	Line *ln;

	ln = malloc(sizeof ln[0]);
	memset(ln, 0, sizeof ln[0]);
	ln->sp = sp;
	ln->ep = ep;
	if(line0 == NULL){
		line0 = lineend = ln;
		return;
	}
	ln->prev = lineend;
	lineend->next = ln;
	lineend = ln;
}

void
initlayout(void)
{
	char *sp, *p, *ep;
	ep = text + ntext;
	for(sp = p = text; p < ep; p++){
		if(*p == '\n'){
			appendline(sp, p);
			sp = p+1;
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
	setfontsize(10);

	selcolor = allocimage(rect(0,0,1,1), color(50,50,50,255));

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

	initlayout();

	int drag = 0;
	int select = 0;
	int repaint;
	int dragu0, dragv0;
	int mouse, m;

	short sel0[2];
	short sel1[2];

//drawanimate(1);
	for(;;){
		repaint = 0;
		for(inp = drawevents(&einp); inp < einp; inp++){
			if(keystr(inp, "q"))
				exit(0);
			if(select == 0 && mousebegin(inp) == Mouse1){
				printf("select begin %d,%d!\n", inp->xy[0], inp->xy[1]);
				sel0[0] = inp->xy[0];
				sel0[1] = inp->xy[1];
				sel1[0] = inp->xy[0];
				sel1[1] = inp->xy[1];
				select = 1;
				repaint = 1;
			}
			if(select && mousemove(inp) == Mouse1){
				sel1[0] = inp->xy[0];
				sel1[1] = inp->xy[1];
				repaint = 1;
			}
			if(select && mouseend(inp) == Mouse1){
				sel1[0] = inp->xy[0];
				sel1[1] = inp->xy[1];
				printf("select end: %d,%d,%d,%d!\n", sel0[0], sel0[1], sel1[0], sel1[1]);
				select = 0;
				repaint = 1;
			}
			if(mousebegin(inp) == Mouse2){
				printf("mouse2!\n");
			}

			if(drag == 0 && (m = mousebegin(inp)) == Mouse3){
				mouse = m;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				drag = 1;
				repaint = 1;
			}
			if(drag && mousemove(inp) == Mouse3){
				uoff += inp->xy[0] - dragu0;
				voff += inp->xy[1] - dragv0;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				repaint = 1;
			}
			if(drag && mouseend(inp) == Mouse3){
				uoff += inp->xy[0] - dragu0;
				voff += inp->xy[1] - dragv0;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				drag = 0;
				repaint = 1;
			}

			if(mousebegin(inp) == Mouse4){
				voff -= 30;
				repaint = 1;
			}
			if(mousebegin(inp) == Mouse5){
				voff += 30;
				repaint = 1;
			}
			if(redraw(inp))
				repaint = 1;
		}
		if(repaint){
			//drawrect(&screen, screen.r, color(150, 200, 80, 255));
			drawrect(&screen, screen.r, color(0, 0, 0, 0));
			//drawcircle(&screen, screen.r, pt(500<<4,300<<4), 100<<4, 4, color(150, 200, 80, 255)); //color(75,100,40,255)
			Rect textr;
			textr = screen.r;
			textr.u0 += 10;
			textr.v0 += 10;
			textr.uend -= 10;
			textr.vend -= 10;
			drawlines(textr, line0, sel0, sel1);
		}
	}

	return 0;
}

