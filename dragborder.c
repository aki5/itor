
#include "os.h"
#include "draw3.h"
#include "dragborder.h"

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
dragborder(Dragborder *db, Rect dstr, Image *color, int bord, int pad, Input *inp, Input *inep, int *hitp)
{
	Rect topr, leftr, bottr, rightr;
	Rect topleft, bottleft, bottright, topright;
	int didadj;

	dragbordrects(dstr, bord, pad, 
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
drawborder(Dragborder *db, Rect dstr, Image *color, int bord, int pad)
{
	Rect topr, leftr, bottr, rightr;
	Rect topleft, bottleft, bottright, topright;

	dragbordrects(dstr, bord, pad, 
		&topr, &leftr, &bottr, &rightr,
		&topleft, &bottleft, &bottright, &topright
	);

	blend2(&screen, topr, color, pt(0,0), BlendOver);
	blend2(&screen, leftr, color, pt(0,0), BlendOver);
	blend2(&screen, rightr, color, pt(0,0), BlendOver);
	blend2(&screen, bottr, color, pt(0,0), BlendOver);

	/* top left add */
	blendcircle(
		&screen,
		topleft,
		color,
		BlendOver,
		pt(
			(dstr.u0-1)<<4,
			(dstr.v0-1)<<4
		),
		(bord+pad)<<4,
		4
	);
	/* top left sub */
	blendcircle(
		&screen,
		topleft,
		color,
		BlendSub,
		pt(
			(dstr.u0-1)<<4,
			(dstr.v0-1)<<4
		),
		(pad)<<4,
		4
	);

	/* bottom left add */
	blendcircle(
		&screen,
		bottleft,
		color,
		BlendOver,
		pt(
			(dstr.u0-1)<<4,
			dstr.vend<<4
		),
		(bord+pad)<<4,
		4
	);

	/* bottom left sub */
	blendcircle(
		&screen,
		bottleft,
		color,
		BlendSub,
		pt(
			(dstr.u0-1)<<4,
			dstr.vend<<4
		),
		(pad)<<4,
		4
	);

	/* bottom right add */
	blendcircle(
		&screen,
		bottright,
		color,
		BlendOver,
		pt(
			dstr.uend<<4,
			dstr.vend<<4
		),
		(bord+pad)<<4,
		4
	);

	/* bottom right sub */
	blendcircle(
		&screen,
		bottright,
		color,
		BlendSub,
		pt(
			dstr.uend<<4,
			dstr.vend<<4
		),
		(pad)<<4,
		4
	);

	/* top right add */
	blendcircle(
		&screen,
		topright,
		color,
		BlendOver,
		pt(
			dstr.uend<<4,
			(dstr.v0-1)<<4
		),
		(bord+pad)<<4,
		4
	);

	/* top right sub */
	blendcircle(
		&screen,
		topright,
		color,
		BlendSub,
		pt(
			dstr.uend<<4,
			(dstr.v0-1)<<4
		),
		(pad)<<4,
		4
	);

	/* close button */
	Rect closeb = rect(topr.u0, topr.v0, topr.u0+bord+pad, topr.vend);

	blendcircle(
		&screen,
		screen.r,
		color,
		BlendOver,
		pt(
			(closeb.u0+closeb.uend)<<3,
			(closeb.v0+closeb.vend)<<3
		),
		(bord+pad)<<3,
		4
	);

	blendcircle(
		&screen,
		screen.r,
		color,
		BlendSub,
		pt(
			(closeb.u0+closeb.uend)<<3,
			(closeb.v0+closeb.vend)<<3
		),
		(bord+pad-4)<<3,
		4
	);
}
