/*
 *	It is quite interesting that if we enable animating on libdraw3,
 *	there's noticeably more sluggishness on selecting and typing.
 *
 *	I'd like to blame X11 for that, but as long as X11 is the only
 *	back-end we have, there's really nothing to compare it with.
 *
 *	It seems very important that input events are timely. High frame rate
 *	is not very useful if your input isn't seen by your program until many
 *	frames later. I'd rather take 20fps and instant reaction than 60fps
 *	with a 3 frame delay. But that's not the kind of tradeoff this is.
 *	Just making a note that latency is probably more important than
 *	throughput in this area too.
 */
#include "os.h"
#include "draw3.h"
#include "textedit.h"
#include "dragborder.h"

enum {
	Bord = 10,
	Pad = 10
};

static Dragborder *bords;
static Rect *rects;
int nbords;

char *
tryfont(char *fontname)
{
	int fd;
	if((fd = open(fontname, O_RDONLY)) == -1)
		fontname = NULL;
	else
		close(fd);
	return fontname;
}

int
main(int argc, char *argv[])
{
	Input *inp, *inep;
	char *fontname = NULL;
	int fontsize;
	int i, opt, fgci;

	Image *bordcolor;
	uchar bordcval[4];
	
	if(fontname == NULL)
		fontname = tryfont("/System/Library/Fonts/Monaco.dfont");
	if(fontname == NULL)
		fontname = tryfont("/home/aki/Monaco.dfont");
	if(fontname == NULL)
		fontname = tryfont("/opt/X11/share/fonts/TTF/Vera.ttf");
	if(fontname == NULL)
		fontname = tryfont("/opt/X11/share/fonts/TTF/VeraMono.ttf");
	if(fontname == NULL)
		fontname = tryfont("/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf");
	if(fontname == NULL)
		fontname = tryfont("/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansCondensed.ttf");
	if(fontname == NULL)
		fontname = tryfont("/usr/share/fonts/truetype/droid/DroidSans.ttf");

	fontsize = 11; 

	initdrawstr(fontname); 
	setfontsize(fontsize);

	if(drawinit(60*fontem(),800) == -1){
		fprintf(stderr, "drawinit failed\n");
		exit(1);	
	}

	bordcval[0] = 80; bordcval[1] = 200; bordcval[2] = 150; bordcval[3] = 255;
	bordcolor = allocimage(rect(0,0,1,1), bordcval);


	for(;;){
		Input *p;
		inp = drawevents(&inep);
		drawrect(&screen, screen.r, color(0, 0, 0, 0));

		for(i = 0; i < nbords; i++){
			Rect tmpr;
			tmpr = rects[i];
			tmpr.u0 -= Pad+1;
			tmpr.v0 -= Pad+1;
			tmpr.uend += Pad+1;
			tmpr.vend += Pad+1;
			drawrect(&screen, tmpr, color(0, 0, 0, 0));
			rects[i] = dragborder(bords+i, rects[i], bordcolor, Bord, Pad, inp, inep);
		}

		for(p = inp; p < inep; p++){
			if(mousebegin(p) == Mouse1){
				nbords++;
				bords = realloc(bords, nbords * sizeof bords[0]);
				memset(bords+nbords-1, 0, sizeof bords[0]);
				rects = realloc(rects, nbords * sizeof rects[0]);
				rects[nbords-1] = rect(p->xy[0], p->xy[1], p->xy[0], p->xy[1]);
				rects[nbords-1] = dragborder(
					bords+nbords-1,
					rects[nbords-1],
					bordcolor, Bord, Pad, inp, inep
				);
			}
		}
	}

	return 0;
}
