
#include "os.h"
#include "draw3.h"
#include "textedit.h"


enum {
	OpInsert = 1,
	OpErase = 2,
};


/*
 *	The undo mechanism: it's just an array of operations
 *	that were done, with enough data to reverse (or forward) one.
 *	Every edit is either an insert or an erase. Should be enough for
 *	everyone :)
 */

static void
addop(Textedit *texp, int opcode, int mark, char *str, int len)
{
	Editop *op;
	if(texp->nops >= texp->aops){
		texp->aops = texp->nops + 128;
		texp->ops = realloc(texp->ops, texp->aops * sizeof texp->ops[0]);
	}
	op = texp->ops + texp->nops;
	op->opcode = opcode;
	op->mark = mark;
	op->str = malloc(len);
	memcpy(op->str, str, len);
	op->len = len;
	texp->nops++;
}

static void
insert(Textedit *texp, int mark0, char *str, int len, int isundo)
{
	if(isundo == 0)
		addop(texp, OpInsert, mark0, str, len);
	if(texp->ntext+len >= texp->atext){
		texp->atext = texp->ntext+len;
		texp->text = realloc(texp->text, texp->atext);
	}
	memmove(texp->text+mark0+len, texp->text+mark0, texp->ntext-mark0);
	texp->ntext += len;
	memcpy(texp->text+mark0, str, len);
}

static void
erase(Textedit *texp, int mark0, int mark1, int isundo)
{
	if(isundo == 0)
		addop(texp, OpErase, mark0, texp->text+mark0, mark1-mark0);
	memmove(texp->text+mark0, texp->text+mark1, texp->ntext-mark1);
	texp->ntext -= mark1-mark0;
}

static void
undo(Textedit *texp)
{
	if(texp->nops > 0){
		Editop *op;
		op = texp->ops + texp->nops-1;
		if(op->opcode == OpErase){
			insert(texp, op->mark, op->str, op->len, 1);
			texp->mark[0] = texp->mark[1] = op->mark+op->len;
		}
		if(op->opcode == OpInsert){
			erase(texp, op->mark, op->mark+op->len, 1);
			texp->mark[0] = texp->mark[1] = op->mark;
		}
		/* I guess the following is a bad idea if we ever want redo */
		free(op->str);
		memset(texp->ops+texp->nops-1, 0, sizeof texp->ops[0]);
		texp->nops--;
	}
}


void
inittextedit(Textedit *texp, Image *dst, char *text, int ntext)
{
	memset(texp, 0, sizeof texp[0]);
	texp->dst = dst;
	texp->text = text;
	texp->ntext = ntext;
	texp->atext = ntext;
}
/*
 *	textview handles events and draws the view. It also computes
 *	mark[0] and mark[1] from mouse selection in case they weren't
 *	already set. It is important that the drawing routine covers
 *	all of dstr, otherwise it may miss hit-testing for select.
 */
void
textedit(Textedit *texp, Input *inp, Input *inep)
{

	for(; inp < inep; inp++){
		/* Meta is the cmd key on macs */

		if(keystr(inp, "z") && (inp->on & (KeyMeta|KeyControl)) != 0)
			undo(texp);

		if((keystr(inp, "=") || keystr(inp, "+")) && (inp->on & (KeyMeta|KeyControl)) != 0){
			texp->fontsize++;
			setfontsize(texp->fontsize);
		}

		if(keystr(inp, "-") && (inp->on & (KeyMeta|KeyControl)) != 0){
			if(texp->fontsize > 4){
				texp->fontsize--;
				setfontsize(texp->fontsize);
			}
		}

		if(keystr(inp, "c") && (inp->on & (KeyMeta|KeyControl)) != 0){
			if(texp->aclip < texp->mark[1]-texp->mark[0]){
				texp->aclip = texp->mark[1]-texp->mark[0];
				texp->clip = realloc(texp->clip, texp->aclip);
			}
			memcpy(texp->clip, texp->text+texp->mark[0], texp->mark[1]-texp->mark[0]);
			texp->nclip = texp->mark[1]-texp->mark[0];
		}

		if(keystr(inp, "x") && (inp->on & (KeyMeta|KeyControl)) != 0){
			if(texp->mark[0] < texp->mark[1]){
				if(texp->aclip < texp->mark[1]-texp->mark[0]){
					texp->aclip = texp->mark[1]-texp->mark[0];
					texp->clip = realloc(texp->clip, texp->aclip);
				}
				memcpy(texp->clip, texp->text+texp->mark[0], texp->mark[1]-texp->mark[0]);
				texp->nclip = texp->mark[1]-texp->mark[0];

				erase(texp, texp->mark[0], texp->mark[1], 0);

				texp->mark[1] = texp->mark[0];
				texp->sel0[0] = 0;
				texp->sel0[1] = 0;
				texp->sel1[0] = 0;
				texp->sel1[1] = 0;
			}
		}

		if(keystr(inp, "v") && (inp->on & (KeyMeta|KeyControl)) != 0){
			if(texp->mark[0] != texp->mark[1])
				erase(texp, texp->mark[0], texp->mark[1], 0);
			insert(texp, texp->mark[0], texp->clip, texp->nclip, 0);
			texp->mark[1] = texp->mark[0] + texp->nclip;
		}

		if((keystr(inp, "q")) && (inp->on & (KeyMeta|KeyControl)) != 0)
			exit(0);

		if((inp->on & (KeyMeta|KeyControl)) == 0){
			if(inp->begin & KeyStr){
				int len;
				len = strlen(inp->str);
				if(texp->mark[0] != texp->mark[1]){
					erase(texp, texp->mark[0], texp->mark[1], 0);
					texp->mark[1] = texp->mark[0];
				}
				insert(texp, texp->mark[0], inp->str, len, 0);
				texp->mark[0] = texp->mark[1] = texp->mark[0]+len;
			}
			if(inp->begin & KeyBackSpace){
				if(texp->mark[0] > 0){
					if(texp->ntext > 0){
						if(texp->mark[0] == texp->mark[1]){
							erase(texp, texp->mark[0]-1, texp->mark[0], 0);
							texp->mark[0] = texp->mark[1] = texp->mark[0]-1;
						} else {
							erase(texp, texp->mark[0], texp->mark[1], 0);
							texp->mark[1] = texp->mark[0];
						}
					}
				}
			}
		}

		if(keypress(inp, KeyLeft)){
			if(texp->mark[0] > 0)
				texp->mark[0]--;
			if((inp->on & KeyShift) == 0)
				texp->mark[1] = texp->mark[0];
		}

		if(keypress(inp, KeyRight)){
			if(texp->mark[1] < texp->ntext)
				texp->mark[1]++;
			if((inp->on & KeyShift) == 0)
				texp->mark[0] = texp->mark[1];
		}

		if(keypress(inp, KeyDown)){
			texp->sel0[0] = texp->marku[0];
			texp->sel0[1] = texp->markv[0] + linespace();
			texp->sel1[0] = texp->marku[1];
			texp->sel1[1] = texp->markv[1] + linespace();
			texp->nmark = 0;
		}
		if(keypress(inp, KeyUp)){
			texp->sel0[0] = texp->marku[0];
			texp->sel0[1] = texp->markv[0] - linespace();
			texp->sel1[0] = texp->marku[1];
			texp->sel1[1] = texp->markv[1] - linespace();
			texp->nmark = 0;
		}
		/* Text selection by a pointing device, also the tweaking blobs. */
		if(texp->select == 0 && mousebegin(inp) == Mouse1){
			if(ptinellipse(
				inp->xy,
				pt(texp->marku[0]+texp->uoff, texp->markv[0]+texp->voff-linespace()/2),
				pt(texp->marku[0]+texp->uoff, texp->markv[0]+texp->voff-linespace()/2),
				linespace()/2
			)){
				texp->seloff[0] = -(texp->marku[0] - (inp->xy[0]-texp->uoff));
				texp->seloff[1] = -(texp->markv[0] - (inp->xy[1]-texp->voff) + linespace()/2);
				texp->sel0[0] = inp->xy[0] - texp->uoff - texp->seloff[0];
				texp->sel0[1] = inp->xy[1] - texp->voff - texp->seloff[1];
				texp->sel1[0] = texp->marku[1];
				texp->sel1[1] = texp->markv[1];
				texp->select = 2;
			} else if(ptinellipse(
				inp->xy,
				pt(texp->marku[1]+texp->uoff, texp->markv[1]+texp->voff+3*linespace()/2-1),
				pt(texp->marku[1]+texp->uoff, texp->markv[1]+texp->voff+3*linespace()/2-1),
				linespace()/2
			)){
				texp->seloff[0] = -(texp->marku[1] - (inp->xy[0]-texp->uoff));
				texp->seloff[1] = -(texp->markv[1] - (inp->xy[1]-texp->voff)+linespace()/2);
				texp->sel0[0] = inp->xy[0] - texp->uoff - texp->seloff[0];
				texp->sel0[1] = inp->xy[1] - texp->voff - texp->seloff[1];
				texp->sel1[0] = texp->marku[0];
				texp->sel1[1] = texp->markv[0];
				texp->select = 3;
			} else {
				texp->seloff[0] = 0;
				texp->seloff[1] = 0;
				texp->sel0[0] = inp->xy[0]-texp->uoff;
				texp->sel0[1] = inp->xy[1]-texp->voff;
				texp->sel1[0] = inp->xy[0]-texp->uoff;
				texp->sel1[1] = inp->xy[1]-texp->voff;
				texp->select = 1;
				texp->nmark = 0;
			}
		}

		if(texp->select != 0 && ((mousemove(inp) == Mouse1) || (mouseend(inp) == Mouse1))){
			if(texp->select == 1){
				texp->sel1[0] = inp->xy[0]-texp->uoff;
				texp->sel1[1] = inp->xy[1]-texp->voff;
			} else if(texp->select == 2){
				short tmp;
				texp->sel0[0] = inp->xy[0] - texp->uoff - texp->seloff[0];
				tmp = inp->xy[1] - texp->voff - texp->seloff[1];
				if(tmp <= texp->sel1[1]+linespace()-1)
					texp->sel0[1] = tmp;
			} else if(texp->select == 3){
				short tmp;
				texp->sel0[0] = inp->xy[0] - texp->uoff - texp->seloff[0];
				tmp = inp->xy[1] - texp->voff - texp->seloff[1];
				if(tmp >= texp->sel1[1])
					texp->sel0[1] = tmp; 
			}
			texp->nmark = 0;
			if(mouseend(inp) == Mouse1)
				texp->select = 0;
		}

		if(mousebegin(inp) == Mouse2){
			printf("mouse2!\n");
		}

		if(texp->drag == 0 && mousebegin(inp) == Mouse3){
			texp->dragu0 = inp->xy[0];
			texp->dragv0 = inp->xy[1];
			texp->drag = 1;
		}

		if(texp->drag && mousemove(inp) == Mouse3){
			texp->uoff += inp->xy[0] - texp->dragu0;
			texp->voff += inp->xy[1] - texp->dragv0;
			texp->dragu0 = inp->xy[0];
			texp->dragv0 = inp->xy[1];
		}

		if(texp->drag && mouseend(inp) == Mouse3){
			texp->uoff += inp->xy[0] - texp->dragu0;
			texp->voff += inp->xy[1] - texp->dragv0;
			texp->dragu0 = inp->xy[0];
			texp->dragv0 = inp->xy[1];
			texp->drag = 0;
		}

		/*
		 *	These are the mouse wheel actions, linespace seems a little too fast
		 *	on a mac, yet it seems a little too slow for linux.. the mice just work 
		 *	very differently by default. A stupid compromise that gives unsatisfactory
		 *	but usable scroll on both. Sigh.
		 */
		if(mousebegin(inp) == Mouse4){
			texp->voff -= linespace();
			if(texp->select){
				texp->sel1[1] += linespace();
				texp->nmark = 0;
			}
		}

		if(mousebegin(inp) == Mouse5){
			texp->voff += linespace();
			if(texp->select){
				texp->sel1[1] -= linespace();
				texp->nmark = 0;
			}
		}
	}

	Rect r, lr, dstr;
	char *sp;
	int insel, dx;
	int code, off;
	short sel0[2];
	short sel1[2];

	dstr = texp->dst->r;

	sel0[0] = texp->sel0[0] < dstr.u0 ? dstr.u0+texp->uoff : (texp->sel0[0] + texp->uoff);
	sel0[1] = texp->sel0[1] < dstr.v0 ? dstr.v0+texp->voff : (texp->sel0[1] + texp->voff);
	sel1[0] = texp->sel1[0] < dstr.u0 ? dstr.u0+texp->uoff : (texp->sel1[0] + texp->uoff);
	sel1[1] = texp->sel1[1] < dstr.v0 ? dstr.v0+texp->voff : (texp->sel1[1] + texp->voff);

	insel = 0;
	texp->uoff = 0; /* this is ugly */
	r.u0 = dstr.u0 + texp->uoff;
	r.v0 = dstr.v0 + texp->voff;
	r.uend = dstr.uend;
	r.vend = r.v0 + linespace();

	for(sp = texp->text; sp < texp->text+texp->ntext; sp += off){
		code = utf8decode(sp, &off, texp->text+texp->ntext-sp);
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
			dx = 3*fontem() - (r.u0-(dstr.u0+texp->uoff)) % (3*fontem());
			lr = rect(r.u0, r.v0, r.u0+dx, r.v0+linespace());
			break;
		default:
			lr = drawchar(texp->dst, r, texp->fgcolor, BlendOver, code);
			break;
		}
		if(texp->nmark < 2 && ptinrect(sel0, &lr)){
			texp->mark[texp->nmark] = sp-texp->text;
			texp->nmark++;
		}
		if(texp->nmark < 2 && ptinrect(sel1, &lr)){
			texp->mark[texp->nmark] = sp-texp->text;
			texp->nmark++;
		}
		if(texp->nmark > 0 && texp->mark[0] == sp-texp->text){
			texp->marku[0] = lr.u0 - texp->uoff;
			texp->markv[0] = lr.v0 - texp->voff;
			insel = 1;
		}
		if(texp->nmark > 1 && texp->mark[1] == sp-texp->text){
			texp->marku[1] = lr.u0 - texp->uoff;
			texp->markv[1] = lr.v0 - texp->voff;
			insel = 0;
			blend2(
				texp->dst,
				rect(lr.u0, lr.v0, lr.u0+2, lr.vend),
				texp->selcolor,
				BlendUnder
			);
		}

		if(insel)
			blend2(texp->dst, cliprect(lr, dstr), texp->selcolor, BlendUnder);

		if(code == '\n'){
			r.u0 = dstr.u0 + texp->uoff;
			r.uend = dstr.uend;
			r.v0 += linespace();
			r.vend += linespace();
		} else {
			r.u0 += rectw(&lr);
		}
	}
	while(texp->nmark < 2){
		texp->marku[texp->nmark] = r.u0 - texp->uoff;
		texp->markv[texp->nmark] = r.v0 - texp->voff;
		texp->mark[texp->nmark] = sp - texp->text;
		texp->nmark++;
	}
	if(texp->mark[0] == sp-texp->text){
		blend2(
			texp->dst,
			rect(r.u0, r.v0, r.u0+2, r.vend),
			texp->selcolor,
			BlendUnder
		);
		/*
		 *	TODO: this is a hack to get appending to the end right,
		 *	I don't really understand why the loop above doesn't cut it
		 */
		texp->marku[0] = r.u0 - texp->uoff;
		texp->markv[0] = r.v0 - texp->voff;
		texp->marku[1] = r.u0 - texp->uoff;
		texp->markv[1] = r.v0 - texp->voff;

	}

	if(ptinrect(pt(texp->marku[0]+texp->uoff, texp->markv[0]+texp->voff), &dstr)){
		blendcircle(
			texp->dst, 
			dstr,
			texp->selcolor,
			BlendUnder,
			pt((texp->marku[0]+texp->uoff)<<4, ((texp->markv[0]+texp->voff)-linespace()/2)<<4),
			(linespace()/2)<<4,
			4
		);
	}

	if(ptinrect(pt(texp->marku[1]+texp->uoff, texp->markv[1]+texp->voff), &dstr)){
		blendcircle(
			texp->dst, 
			dstr,
			texp->selcolor,
			BlendUnder,
			pt((texp->marku[1]+texp->uoff)<<4, ((texp->markv[1]+texp->voff)+linespace()+linespace()/2-1)<<4),
				(linespace()/2)<<4,
				4
		);
	}
}