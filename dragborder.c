
#include "os.h"
#include "draw3.h"
#include "dragborder.h"

void
initborder(Border *db, Image *color, int bord, int pad)
{
	memset(db, 0, sizeof db[0]);

	db->color = color;
	db->bord = bord;
	db->pad = pad;

	db->corn = allocimage(rect(0,0,2*(db->bord+db->pad),2*(db->bord+db->pad)), color(0,0,0,0));
	blendcircle(
		db->corn,
		db->corn->r,
		db->color,
		BlendOver,
		pt((db->bord+db->pad)<<4,(db->bord+db->pad)<<4),
		(db->bord+db->pad)<<4,
		4
	);
	blendcircle(
		db->corn,
		db->corn->r,
		db->color,
		BlendSub,
		pt((db->bord+pad)<<4,(db->bord+pad)<<4),
		(db->pad)<<4,
		4
	);
}

void
freeborder(Border *db)
{
	if(db->corn != NULL)
		freeimage(db->corn);
}

void
dragbordrects(Rect dstr, int bord, int pad, Rect *topr, Rect *leftr, Rect *bottr, Rect *rightr, Rect *topleft, Rect *bottleft, Rect *bottright, Rect *topright)
{
	*topr = rect(
		dstr.u0,
		dstr.v0-(bord+pad),
		dstr.uend,
		dstr.v0-pad
	);

	*leftr = rect(
		dstr.u0-(bord+pad),
		dstr.v0,
		dstr.u0-(pad),
		dstr.vend
	);

	*rightr = rect(
		dstr.uend+pad,
		dstr.v0,
		dstr.uend+(bord+pad),
		dstr.vend
	);

	*bottr = rect(
		dstr.u0,
		dstr.vend+pad,
		dstr.uend,
		dstr.vend+(bord+pad)
	);

	*topleft = rect(
		dstr.u0-(bord+pad)-1,
		dstr.v0-(bord+pad)-1,
		dstr.u0,
		dstr.v0
	);

	*bottleft = rect(
		dstr.u0-(bord+pad)-1,
		dstr.vend,
		dstr.u0,
		dstr.vend+(bord+pad)
	);

	*topright = rect(
		dstr.uend,
		dstr.v0-(bord+pad)-1,
		dstr.uend+(bord+pad),
		dstr.v0
	);

	*bottright = rect(
		dstr.uend,
		dstr.vend,
		dstr.uend+(bord+pad),
		dstr.vend+(bord+pad)
	);
}

Rect
dragborder(Border *db, Rect dstr, Input *inp, Input *inep, int *hitp)
{
	Rect topr, leftr, bottr, rightr;
	Rect topleft, bottleft, bottright, topright;
	int didadj;

	dragbordrects(dstr, db->bord, db->pad, 
		&topr, &leftr, &bottr, &rightr,
		&topleft, &bottleft, &bottright, &topright
	);

	didadj = 0;
	for(;inp < inep; inp++){
		if(mousebegin(inp) == Mouse1){
			if(ptinrect(inp->xy, &topr) || ptinrect(inp->xy, &leftr) || ptinrect(inp->xy, &bottr) || ptinrect(inp->xy, &rightr))
				db->drag = 1;
			if(ptinrect(inp->xy, &topleft))// todo: try with point really on border
				db->drag = 2;
			if(ptinrect(inp->xy, &bottleft))// todo: try with point really on border
				db->drag = 3;
			if(ptinrect(inp->xy, &bottright))// todo: try with point really on border
				db->drag = 4;
			if(ptinrect(inp->xy, &topright))// todo: try with point really on border
				db->drag = 5;
			if(db->drag != 0){
				db->xy[0] = inp->xy[0];
				db->xy[1] = inp->xy[1];
				inp->begin &= ~Mouse1;
			}
		}
		if(db->drag != 0 && mousemove(inp) == Mouse1){
			intcoord du, dv;
			du = inp->xy[0] - db->xy[0];
			dv = inp->xy[1] - db->xy[1];
			db->xy[0] = inp->xy[0];
			db->xy[1] = inp->xy[1];
			if(db->drag == 1){
				dstr.v0 += dv;
				dstr.u0 += du;
				dstr.vend += dv;
				dstr.uend += du;
			} else if(db->drag == 2){
				dstr.v0 += dv;
				dstr.u0 += du;
			} else if(db->drag == 3){
				dstr.vend += dv;
				dstr.u0 += du;
			} else if(db->drag == 4){
				dstr.vend += dv;
				dstr.uend += du;
			} else if(db->drag == 5){
				dstr.v0 += dv;
				dstr.uend += du;
			}
			didadj = 1;
		}
		if(db->drag != 0 && mouseend(inp) == Mouse1)
			db->drag = 0;
	}

	if(hitp != NULL)
		*hitp = didadj;

	return dstr;
}

void
drawborder(Border *db, Rect dstr, int op)
{
	Rect topr, leftr, bottr, rightr;
	Rect topleft, bottleft, bottright, topright;

	dragbordrects(dstr, db->bord, db->pad, 
		&topr, &leftr, &bottr, &rightr,
		&topleft, &bottleft, &bottright, &topright
	);

	blend2(&screen, topr, db->color, pt(0,0), op);
	blend2(&screen, leftr, db->color, pt(0,0), op);
	blend2(&screen, rightr, db->color, pt(0,0), op);
	blend2(&screen, bottr, db->color, pt(0,0), op);

	blend2(&screen, topleft, db->corn, pt(topleft.u0,topleft.v0), op);
	blend2(&screen, bottleft, db->corn, pt(bottleft.u0,bottleft.v0+db->bord+db->pad), op);
	blend2(&screen, bottright, db->corn, pt(bottright.u0+db->bord+db->pad,bottright.v0+db->bord+db->pad), op);
	blend2(&screen, topright, db->corn, pt(topright.u0+db->bord+db->pad,topright.v0), op);

/* close button 
	Rect closeb = rect(topr.u0, topr.v0, topr.u0+db->bord+db->pad, topr.vend);

	blendcircle(
		&screen,
		screen.r,
		db->color,
		BlendOver,
		pt(
			(closeb.u0+closeb.uend)<<3,
			(closeb.v0+closeb.vend)<<3
		),
		(db->bord+db->pad)<<3,
		4
	);

	blendcircle(
		&screen,
		screen.r,
		db->color,
		BlendSub,
		pt(
			(closeb.u0+closeb.uend)<<3,
			(closeb.v0+closeb.vend)<<3
		),
		(db->bord+db->pad-4)<<3,
		4
	);
*/
}
