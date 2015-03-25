/*
 *	It is quite interesting that if we enable animating on libdraw3,
 *	there's noticeably more sluggishness on selecting and typing.
 *
 *	I'd like to blame X11 for that, but as long as X11 is the only
 *	back-end we have, there's really nothing to compare it with.
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

int mark[2];
int nmark;

/*
 *	drawtext draws the main text panel view. it also computes
 *	mark[0] and mark[1] from mouse selection in case they weren't
 *	already set. It is important that the routine covers all of 
 *	dstr, otherwise it may miss hit-testing on the mouse select.
 */
void
drawtext(Rect dstr, short *sel0x, short *sel1x)
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
			lr = drawchar(&screen, r, code, fgcolor);
			break;
		}
		if(nmark < 2 && ptinrect(sel0, &lr))
			mark[nmark++] = sp-text;
		if(nmark < 2 && ptinrect(sel1, &lr))
			mark[nmark++] = sp-text;

		if(nmark > 0 && mark[0] == sp-text){
			blend2(
				&screen,
				rect(lr.u0-1, lr.v0, lr.u0+1, lr.vend),
				fgcolor,
				BlendUnder
			);
			insel = 1;
		}
		if(nmark > 1 && mark[1] == sp-text)
			insel = 0;

		if(insel)
			blend2(&screen, lr, selcolor, BlendUnder);

		if(code == '\n'){
			r.u0 = dstr.u0 + uoff;
			r.uend = dstr.uend;
			r.v0 += linespace();
			r.vend += linespace();
		} else {
			r.u0 += rectw(&lr);
		}
	}
	while(nmark < 2)
		mark[nmark++] = sp-text;
	if(mark[0] == sp-text)
		blend2(
			&screen,
			rect(r.u0-1, r.v0, r.u0+1, r.vend),
			fgcolor,
			BlendUnder
		);
}

/*
 *	the undo mechanism follows; it's just an array of operations
 *	that were done, with enough data to reverse (or forward) them.
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

int
main(int argc, char *argv[])
{
	Input *inp, *einp;
	char *fontname;
	int fontsize;
	int opt, fgci;

	//fontname = "/usr/share/fonts/truetype/droid/DroidSans.ttf";
	fontname = "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf";
	fontsize = 9;
	fgci = 2;
	while((opt = getopt(argc, argv, "f:s:c:")) != -1){
		switch(opt){
		case 'c':
			fgci = atoi(optarg);
			break;
		case 'f':
			fontname = optarg;
			break;
		case 's':
			fontsize = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: %s [-f fontfile] [-s fontsize] [-c coloridx] file1\n", argv[0]);
			exit(1);
		}
	}

	drawinit(640,800);
	initdrawstr(fontname); 
	setfontsize(fontsize);

#if 0
	fgcolor = allocimage(rect(0,0,1,1), color(0, 0, 0, 255));
	bgcolor = allocimage(rect(0,0,1,1), color(255, 240, 240, 255));
	selcolor = allocimage(rect(0,0,1,1), color(80,90,70,127));
#else
	uchar cval[4];
	// this may be my new favorite green: color(150, 200, 80, 255));
	idx2color(fgci, cval);
	fgcolor = allocimage(rect(0,0,1,1), cval);
	bgcolor = allocimage(rect(0,0,1,1), color(0,40,40,255));
	cval[0] /= 2;
	cval[1] /= 2;
	cval[2] /= 2;
	cval[3] /= 2;
	selcolor = allocimage(rect(0,0,1,1), cval);
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

	//drawanimate(1);

	for(;;){
		repaint = 0;
		for(inp = drawevents(&einp); inp < einp; inp++){
			if(nmark == 2 && mark[0] > mark[1])
				abort();

			if(keystr(inp, "s") && (inp->on & KeyControl) != 0)
				save();

			if(keystr(inp, "z") && (inp->on & KeyControl) != 0)
				undo();

			if((keystr(inp, "=") || keystr(inp, "+")) && (inp->on & KeyControl) != 0){
				fontsize++;
				setfontsize(fontsize);
			}

			if(keystr(inp, "-") && (inp->on & KeyControl) != 0){
				if(fontsize > 4){
					fontsize--;
					setfontsize(fontsize);
				}
			}

			if(keystr(inp, "c") && (inp->on & KeyControl) != 0){
				if(aclip < mark[1]-mark[0]){
					aclip = mark[1]-mark[0];
					clip = realloc(clip, aclip);
				}
				memcpy(clip, text+mark[0], mark[1]-mark[0]);
				nclip = mark[1]-mark[0];
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

			if(keystr(inp, "v") && (inp->on & KeyControl) != 0)
				insert(mark[0], clip, nclip, 0);

			if((keystr(inp, "q")) && (inp->on & KeyControl) != 0)
				exit(0);

			if((inp->on & KeyControl) == 0){
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
			static double st = 0.0, et = 0.0;
			/* clear to transparent */
			drawrect(&screen, screen.r, color(0, 0, 0, 0));

			Rect textr;
			textr = insetrect(screen.r, 7);
			drawtext(textr, sel0, sel1);

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
				msg,
				n,
				fgcolor
			);

			st = et;

			/*commented out because it's a bit of a dog on the raspberry */
			//blend2(&screen, textr, bgcolor, BlendRadd);
		}
	}

	return 0;
}

