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

int uoff, voff;

char *clip;
int nclip, aclip;

char *text;
int ntext, atext;

int fd;

Image *selcolor;
Image *fgcolor;
Image *bgcolor;
uchar selcval[4];

short marku[2];
short markv[2];
int mark[2];
int nmark;

/*
 *	Drawtext draws the main text view. it also computes
 *	mark[0] and mark[1] from mouse selection in case they weren't
 *	already set. It is important that the routine covers all of 
 *	dstr, otherwise it may miss hit-testing select.
 */
void
drawtext(Image *dst, Rect dstr, short *sel0x, short *sel1x)
{
	Rect r, lr;
	char *sp;
	short sel0[2], sel1[2];
	int insel, dx;
	int code, off;

	sel0[0] = sel0x[0] < dstr.u0 ? dstr.u0+uoff : (sel0x[0] + uoff);
	sel0[1] = sel0x[1] < dstr.v0 ? dstr.v0+voff : (sel0x[1] + voff);
	sel1[0] = sel1x[0] < dstr.u0 ? dstr.u0+uoff : (sel1x[0] + uoff);
	sel1[1] = sel1x[1] < dstr.v0 ? dstr.v0+voff : (sel1x[1] + voff);

	insel = 0;
	uoff = 0; /* this is ugly */
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
			dx = 3*fontem() - (r.u0-(dstr.u0+uoff)) % (3*fontem());
			lr = rect(r.u0, r.v0, r.u0+dx, r.v0+linespace());
			break;
		default:
			lr = drawchar(dst, r, fgcolor, BlendOver, code);
			break;
		}
		if(nmark < 2 && ptinrect(sel0, &lr)){
			mark[nmark] = sp-text;
			nmark++;
		}
		if(nmark < 2 && ptinrect(sel1, &lr)){
			mark[nmark] = sp-text;
			nmark++;
		}
		if(nmark > 0 && mark[0] == sp-text){
			marku[0] = lr.u0 - uoff;
			markv[0] = lr.v0 - voff;
			insel = 1;
		}
		if(nmark > 1 && mark[1] == sp-text){
			marku[1] = lr.u0 - uoff;
			markv[1] = lr.v0 - voff;
			insel = 0;
			blend2(
				dst,
				rect(lr.u0, lr.v0, lr.u0+2, lr.vend),
				selcolor,
				BlendUnder
			);
		}

		if(insel)
			blend2(dst, cliprect(lr, dstr), selcolor, BlendUnder);

		if(code == '\n'){
			r.u0 = dstr.u0 + uoff;
			r.uend = dstr.uend;
			r.v0 += linespace();
			r.vend += linespace();
		} else {
			r.u0 += rectw(&lr);
		}
	}
	while(nmark < 2){
		marku[nmark] = r.u0 - uoff;
		markv[nmark] = r.v0 - voff;
		mark[nmark] = sp-text;
		nmark++;
	}
	if(mark[0] == sp-text){
		blend2(
			dst,
			rect(r.u0, r.v0, r.u0+2, r.vend),
			selcolor,
			BlendUnder
		);
		/*
		 *	TODO: this is a hack to get appending to the end right,
		 *	I don't really understand why the loop above doesn't cut it
		 */
		marku[0] = r.u0 - uoff;
		markv[0] = r.v0 - voff;
		marku[1] = r.u0 - uoff;
		markv[1] = r.v0 - voff;

	}

	if(ptinrect(pt(marku[0]+uoff, markv[0]+voff), &dstr)){
		blendcircle(
			dst, 
			dstr,
			selcolor,
			BlendUnder,
			pt((marku[0]+uoff)<<4, ((markv[0]+voff)-linespace()/2)<<4),
			(linespace()/2)<<4,
			4
		);
	}

	if(ptinrect(pt(marku[1]+uoff, markv[1]+voff), &dstr)){
		blendcircle(
			dst, 
			dstr,
			selcolor,
			BlendUnder,
			pt((marku[1]+uoff)<<4, ((markv[1]+voff)+linespace()+linespace()/2-1)<<4),
				(linespace()/2)<<4,
				4
		);
	}
}

/*
 *	The undo mechanism: it's just an array of operations
 *	that were done, with enough data to reverse (or forward) one.
 *	Every edit is either an insert or an erase. Should be enough for
 *	everyone :)
 */
enum {
	OpInsert = 1,
	OpErase = 2,
};

typedef struct Editop Editop;
struct Editop {
	int opcode;
	int mark;
	char *str;
	int len;
};

Editop *ops;
int nops, aops;

void
addop(int opcode, int mark, char *str, int len)
{
	Editop *op;
	if(nops >= aops){
		aops = nops + 128;
		ops = realloc(ops, aops * sizeof ops[0]);
	}
	op = ops + nops;
	op->opcode = opcode;
	op->mark = mark;
	op->str = malloc(len);
	memcpy(op->str, str, len);
	op->len = len;
	nops++;
}

void
insert(int mark0, char *str, int len, int isundo)
{
	if(isundo == 0)
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
erase(int mark0, int mark1, int isundo)
{
	if(isundo == 0)
		addop(OpErase, mark0, text+mark0, mark1-mark0);
	memmove(text+mark0, text+mark1, ntext-mark1);
	ntext -= mark1-mark0;
}

void
undo(void)
{
	if(nops > 0){
		Editop *op;
		op = ops + nops-1;
		if(op->opcode == OpErase){
			insert(op->mark, op->str, op->len, 1);
			mark[0] = mark[1] = op->mark+op->len;
		}
		if(op->opcode == OpInsert){
			erase(op->mark, op->mark+op->len, 1);
			mark[0] = mark[1] = op->mark;
		}
		/* I guess the following is a bad idea if we ever want redo */
		free(op->str);
		memset(ops+nops-1, 0, sizeof ops[0]);
		nops--;
	}
}

int
save(void)
{
	int off, n;

	lseek(fd, 0, SEEK_SET);
	ftruncate(fd, 0);
	off = 0;
	while(off < ntext){
		if((n = write(fd, text+off, ntext-off)) == -1)
			return -1;
		off += n;
	}
	return 0;
}

char *
tryfont(char *fontname)
{
	if((fd = open(fontname, O_RDONLY)) == -1)
		fontname = NULL;
	else
		close(fd);
	return fontname;
}

int
main(int argc, char *argv[])
{
	Input *inp, *einp;
	char *fontname = NULL;
	int fontsize;
	int opt, fgci;
	
	if(fontname == NULL)
		fontname = tryfont("/System/Library/Fonts/Monaco.dfont");
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

	/*
	 *	fontsize 11 is pretty good on a regular display,
	 *	retina displays could use way more. could also try
	 *	doing fonts with the dpi stuff..
	 */
	fontsize = 11; 
	fgci = 2;
	while((opt = getopt(argc, argv, "f:s:c:")) != -1){
		switch(opt){
		case 'c':
			fgci = atoi(optarg);
			break;
		case 'f':
			fontname = tryfont(optarg);
			break;
		case 's':
			fontsize = atoi(optarg);
			break;
		default:
			fprintf(
				stderr,
				"usage: %s [-f fontfile] [-s fontsize] [-c coloridx] file1\n",
				argv[0]
			);
			exit(1);
		}
	}

	if(fontname == NULL){
		fprintf(stderr, "could not open font\n");
		exit(1);
	}

	initdrawstr(fontname); 
	setfontsize(fontsize);
	drawinit(60*fontem(),800);

#if 0
	fgcolor = allocimage(rect(0,0,1,1), color(0, 0, 0, 255));
	bgcolor = allocimage(rect(0,0,1,1), color(255, 240, 240, 255));
	selcval[0] = 80;
	selcval[1] = 90;
	selcval[2] = 70;
	selcval[3] = 127;
	selcolor = allocimage(rect(0,0,1,1), selcval);
#else
	uchar cval[4];
	// this may be my new favorite green: color(150, 200, 80, 255));
	//cval[0] = 80; cval[1] = 200; cval[2] = 150; cval[3] = 255;
	idx2color(fgci, cval);
	fgcolor = allocimage(rect(0,0,1,1), cval);
	bgcolor = allocimage(rect(0,0,1,1), color(0,40,40,255));
	selcval[0] = cval[0]/2;
	selcval[1] = cval[1]/2;
	selcval[2] = cval[2]/2;
	selcval[3] = cval[3]/2;
	selcolor = allocimage(rect(0,0,1,1), selcval);
#endif

	if(optind < argc){
		int n;
		fd = open(argv[optind], O_RDWR);
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
	}

	int drag = 0;
	int select = 0;
	int repaint;
	int dragu0, dragv0;
	int mouse, m;

	short sel0[2] = {0};
	short sel1[2] = {0};

	short seloff[2] = {0};

	//drawanimate(1);

	for(;;){
		repaint = 0;
		for(inp = drawevents(&einp); inp < einp; inp++){
			if(nmark == 2 && mark[0] > mark[1])
				abort();

			/* Meta is the cmd key on macs */
			if(keystr(inp, "s") && (inp->on & (KeyMeta|KeyControl)) != 0)
				save();

			if(keystr(inp, "z") && (inp->on & (KeyMeta|KeyControl)) != 0)
				undo();

			if((keystr(inp, "=") || keystr(inp, "+")) && (inp->on & (KeyMeta|KeyControl)) != 0){
				fontsize++;
				setfontsize(fontsize);
			}

			if(keystr(inp, "-") && (inp->on & (KeyMeta|KeyControl)) != 0){
				if(fontsize > 4){
					fontsize--;
					setfontsize(fontsize);
				}
			}

			if(keystr(inp, "c") && (inp->on & (KeyMeta|KeyControl)) != 0){
				if(aclip < mark[1]-mark[0]){
					aclip = mark[1]-mark[0];
					clip = realloc(clip, aclip);
				}
				memcpy(clip, text+mark[0], mark[1]-mark[0]);
				nclip = mark[1]-mark[0];
			}

			if(keystr(inp, "x") && (inp->on & (KeyMeta|KeyControl)) != 0){
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

			if(keystr(inp, "v") && (inp->on & (KeyMeta|KeyControl)) != 0){
				if(mark[0] != mark[1])
					erase(mark[0], mark[1], 0);
				insert(mark[0], clip, nclip, 0);
				mark[1] = mark[0] + nclip;
			}

			if((keystr(inp, "q")) && (inp->on & (KeyMeta|KeyControl)) != 0)
				exit(0);

			if((inp->on & (KeyMeta|KeyControl)) == 0){
				if(inp->begin & KeyStr){
					int len;
					len = strlen(inp->str);
					if(mark[0] != mark[1]){
						erase(mark[0], mark[1], 0);
						mark[1] = mark[0];
					}
					insert(mark[0], inp->str, len, 0);
					mark[0] = mark[1] = mark[0]+len;
				}
				if(inp->begin & KeyBackSpace){
					if(mark[0] > 0){
						if(ntext > 0){
							if(mark[0] == mark[1]){
								erase(mark[0]-1, mark[0], 0);
								mark[0] = mark[1] = mark[0]-1;
							} else {
								erase(mark[0], mark[1], 0);
								mark[1] = mark[0];
							}
						}
					}
				}
			}

			if(keypress(inp, KeyLeft)){
				if(mark[0] > 0)
					mark[0]--;
				if((inp->on & KeyShift) == 0)
					mark[1] = mark[0];
			}

			if(keypress(inp, KeyRight)){
				if(mark[1] < ntext)
					mark[1]++;
				if((inp->on & KeyShift) == 0)
					mark[0] = mark[1];
			}

			if(keypress(inp, KeyDown)){
				sel0[0] = marku[0];
				sel0[1] = markv[0] + linespace();
				sel1[0] = marku[1];
				sel1[1] = markv[1] + linespace();
				nmark = 0;
			}
			if(keypress(inp, KeyUp)){
				sel0[0] = marku[0];
				sel0[1] = markv[0] - linespace();
				sel1[0] = marku[1];
				sel1[1] = markv[1] - linespace();
				nmark = 0;
			}
			/* Text selection by a pointing device, also the tweaking blobs. */
			if(select == 0 && mousebegin(inp) == Mouse1){
				if(ptinellipse(
					inp->xy,
					pt(marku[0]+uoff, markv[0]+voff-linespace()/2),
					pt(marku[0]+uoff, markv[0]+voff-linespace()/2),
					linespace()/2
				)){
					seloff[0] = -(marku[0] - (inp->xy[0]-uoff));
					seloff[1] = -(markv[0] - (inp->xy[1]-voff) + linespace()/2);
					sel0[0] = inp->xy[0] - uoff - seloff[0];
					sel0[1] = inp->xy[1] - voff - seloff[1];
					sel1[0] = marku[1];
					sel1[1] = markv[1];
					select = 2;
				} else if(ptinellipse(
					inp->xy,
					pt(marku[1]+uoff, markv[1]+voff+3*linespace()/2-1),
					pt(marku[1]+uoff, markv[1]+voff+3*linespace()/2-1),
					linespace()/2
				)){
					seloff[0] = -(marku[1] - (inp->xy[0]-uoff));
					seloff[1] = -(markv[1] - (inp->xy[1]-voff)+linespace()/2);
					sel0[0] = inp->xy[0] - uoff - seloff[0];
					sel0[1] = inp->xy[1] - voff - seloff[1];
					sel1[0] = marku[0];
					sel1[1] = markv[0];
					select = 3;
				} else {
					seloff[0] = 0;
					seloff[1] = 0;
					sel0[0] = inp->xy[0]-uoff;
					sel0[1] = inp->xy[1]-voff;
					sel1[0] = inp->xy[0]-uoff;
					sel1[1] = inp->xy[1]-voff;
					select = 1;
					nmark = 0;
				}
			}

			if(select != 0 && ((mousemove(inp) == Mouse1) || (mouseend(inp) == Mouse1))){
				if(select == 1){
					sel1[0] = inp->xy[0]-uoff;
					sel1[1] = inp->xy[1]-voff;
				} else if(select == 2){
					short tmp;
					sel0[0] = inp->xy[0] - uoff - seloff[0];
					tmp = inp->xy[1] - voff - seloff[1];
					if(tmp <= sel1[1]+linespace()-1)
						sel0[1] = tmp;
				} else if(select == 3){
					short tmp;
					sel0[0] = inp->xy[0] - uoff - seloff[0];
					tmp = inp->xy[1] - voff - seloff[1];
					if(tmp >= sel1[1])
						sel0[1] = tmp; 
				}
				nmark = 0;
				if(mouseend(inp) == Mouse1)
					select = 0;
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

			/*
			 *	These are the mouse wheel actions, linespace seems a little too fast
			 *	on a mac, yet it seems a little too slow for linux.. the mice just work 
			 *	very differently by default. A stupid compromise that gives unsatisfactory
			 *	but usable scroll on both. Sigh.
			 */
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
			static double st = 0.0, et = 0.0;
			/* clear to transparent */
			drawrect(&screen, screen.r, color(0, 0, 0, 0));

			Rect textr;
			textr = insetrect(screen.r, 7);
			drawtext(&screen, textr, sel0, sel1);

			et = timenow();
			char msg[64];
			int n;
			n = snprintf(msg, sizeof msg, "%dx%d %.2f fps", rectw(&screen.r), recth(&screen.r), 1.0 / (et-st));
			drawstr(
				&screen,
				rect(
					textr.uend-12*fontem(),
					textr.v0,
					textr.uend,
					textr.v0+linespace()
				),
				fgcolor,
				BlendOver,
				msg,
				n
			);

			st = et;

			/*commented out because it's a bit of a dog on the raspberry */
			//blend2(&screen, textr, bgcolor, BlendUnder);
		}
	}

	return 0;
}
