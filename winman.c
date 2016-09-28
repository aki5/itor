#include "os.h"
#include "draw3.h"
#include "textedit.h"
#include "dragborder.h"

enum {
	Bord = 12,
	Pad = 12 
};

static Border *bords;
static Rect *rects;
static Image **colors;
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
main(void)
{
	Input *inp;
	char *fontname = NULL;
	int ninp;
	int fontsize;
	int i;

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
		inp = drawevents(&ninp);
		int washit = 0;
restart:
		drawrect(&screen, screen.r, color(0, 0, 0, 0));
		for(i = nbords-1; i >= 0; i--){
			Rect tmpr;
			int hit;

			tmpr = rects[i];
			tmpr.u0 -= Pad;
			tmpr.v0 -= Pad;
			tmpr.uend += Pad;
			tmpr.vend += Pad;

			hit = dragborder(bords+i, rects+i, inp, ninp);
			blend2(&screen, tmpr, colors[i], pt(0,0), BlendUnder);

			if(washit == 0 && (hit&2) != 0){
				memmove(rects+i, rects+i+1, (nbords-(i+1)) * sizeof rects[0]);
				memmove(bords+i, bords+i+1, (nbords-(i+1)) * sizeof bords[0]);
				memmove(colors+i, colors+i+1, (nbords-(i+1)) * sizeof colors[0]);
				nbords--;
				goto restart;
			} else if(washit == 0 && (hit&1) != 0){
				Border tmpbord;
				Image *tmpcolor;

				tmpr = rects[i];
				memmove(rects+i, rects+i+1, (nbords-(i+1)) * sizeof rects[0]);
				rects[nbords-1] = tmpr;

				tmpbord = bords[i];
				memmove(bords+i, bords+i+1, (nbords-(i+1)) * sizeof bords[0]);
				bords[nbords-1] = tmpbord;

				tmpcolor = colors[i];
				memmove(colors+i, colors+i+1, (nbords-(i+1)) * sizeof colors[0]);
				colors[nbords-1] = tmpcolor;

				if(washit == 0){
					washit = 1;
					goto restart;
				}

			}
		}
		for(i = 0; i < ninp; i++){
			p = inp + i;
			if((p->on & KeyControl) != 0 && keystr(p, "q"))
				exit(0);
		}
	
		if(!washit){
			for(i = 0; i < ninp; i++){
				p = inp + i;
				if(mousebegin(p) == Mouse1){
					uchar cval[4];
					nbords++;

					bords = realloc(bords, nbords * sizeof bords[0]);
					rects = realloc(rects, nbords * sizeof rects[0]);
					colors = realloc(colors, nbords * sizeof colors[0]);

					initborder(bords+nbords-1, bordcolor, Bord, Pad);
					rects[nbords-1] = rect(p->xy[0]+Bord+Pad, p->xy[1]+Bord+Pad, p->xy[0]+Bord+Pad, p->xy[1]+Bord+Pad);
					dragborder(
						bords+nbords-1,
						rects+nbords-1,
						inp, ninp
					);
					idx2color(nbords-1, cval);
					cval[3] = 224;
					cval[0] = (cval[0]*cval[3])/255;
					cval[1] = (cval[1]*cval[3])/255;
					cval[2] = (cval[2]*cval[3])/255;
					colors[nbords-1] = allocimage(rect(0,0,1,1), cval);

					bords[nbords-1].drag = 4;
					bords[nbords-1].xy[0] += (Bord+Pad)+Pad;
					bords[nbords-1].xy[1] += (Bord+Pad)+Pad;

					Rect tmpr;
					tmpr = rects[nbords-1];
					tmpr.u0 -= Pad;
					tmpr.v0 -= Pad;
					tmpr.uend += Pad;
					tmpr.vend += Pad;

				}
			}
		}
	}

	return 0;
}
