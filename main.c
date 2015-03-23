
#include "os.h"
#include "draw3.h"

typedef struct Line Line;
struct Line {
	Line *next;
	Line *prev;
	char *sp, *ep;
	short v0;
	short vend;
};

int uoff, voff;

Line *line0;
Line *lineend;

char *text;
int ntext, atext;

void
drawlines(Rect dstr, Line *ln)
{
	Rect r, sr;
	r = dstr;
	while(ln->prev != NULL && ln->v0 > dstr.v0)
		ln = ln->prev;
	r.v0 += voff;
	r.u0 += uoff;
	while(ln != NULL && r.v0 < dstr.vend){
		sr = drawstr(&screen, r, ln->sp, ln->ep - ln->sp, color(0,0,0,255));
		ln->v0 = r.v0;
		r.v0 += recth(&sr);
		ln = ln->next;
	}
	if(ln != NULL){
		sr = drawstr(&screen, r, ln->sp, ln->ep - ln->sp, color(0,0,0,255));
		r.v0 += recth(&sr);
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
	setfontsize(10);

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
	int repaint;
	int dragu0, dragv0;
	int mouse, m;
//drawanimate(1);
	for(;;){
		repaint = 0;
		for(inp = drawevents(&einp); inp < einp; inp++){
			if(keystr(inp, "q"))
				exit(0);
			if(mousebegin(inp) == 6){
				voff += 30;
			}
			if(mousebegin(inp) == 5){
				voff -= 30;
			}
			if(drag == 0 && (m = mousebegin(inp)) != -1){
				mouse = m;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				drag = 1;
				repaint = 1;
			}
			if(drag && mousemove(inp) == mouse){
				uoff += inp->xy[0] - dragu0;
				voff += inp->xy[1] - dragv0;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				repaint = 1;
			}
			if(drag && mouseend(inp) == mouse){
				uoff += inp->xy[0] - dragu0;
				voff += inp->xy[1] - dragv0;
				dragu0 = inp->xy[0];
				dragv0 = inp->xy[1];
				drag = 0;
				repaint = 1;
			}
			if(redraw(inp))
				repaint = 1;
		}
		if(repaint){
			//drawrect(&screen, screen.r, color(150, 200, 80, 255));
			drawrect(&screen, screen.r, color(255, 255, 255, 255));
			drawcircle(&screen, screen.r, pt(500<<4,300<<4), 100<<4, 4, color(150, 200, 80, 255)); //color(75,100,40,255)
			Rect textr;
			textr = screen.r;
			textr.u0 += 10;
			textr.v0 += 10;
			textr.uend -= 10;
			textr.vend -= 10;
			drawlines(textr, line0);
		}
	}

	return 0;
}

