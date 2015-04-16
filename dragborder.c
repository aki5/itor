
#include "os.h"
#include "draw3.h"
#include "dragborder.h"

void
freeborder(Border *db)
{
	if(db->corn != NULL)
		freeimage(db->corn);
	if(db->closei != NULL)
		freeimage(db->closei);
}

void
dragbordrects(
	Rect dstr, int bord, int pad,
	Rect *closer,
	Rect *topr, Rect *leftr, Rect *bottr, Rect *rightr,
	Rect *topleft, Rect *bottleft, Rect *bottright, Rect *topright
) {
	closer->u0 = dstr.u0;
	closer->v0 = dstr.v0 - (bord+pad) - (pad/2);
	closer->uend = dstr.u0 + bord+pad;
	closer->vend = closer->v0 + bord+pad;

	*topr = rect(
		closer->uend,
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

int
dragborder(Border *db, Rect *dstr, Input *inp, int ninp)
{
	Rect closer;
	Rect topr, leftr, bottr, rightr;
	Rect topleft, bottleft, bottright, topright;
	int i, didadj;
	int op;
	op = BlendUnder;

	dragbordrects(*dstr, db->bord, db->pad, 
		&closer,
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

	blend2(&screen, closer, db->closei, pt(closer.u0,closer.v0), op);

	didadj = 0;
	for(i = 0; i < ninp; i++){
		Input *p;
		p = inp + i;
		if(mousebegin(p) == Mouse1){
			if(ptinrect(p->xy, &topr) || ptinrect(p->xy, &leftr) || ptinrect(p->xy, &bottr) || ptinrect(p->xy, &rightr))
				db->drag = 1;
			if(ptinrect(p->xy, &topleft))// todo: try with point really on border
				db->drag = 2;
			if(ptinrect(p->xy, &bottleft))// todo: try with point really on border
				db->drag = 3;
			if(ptinrect(p->xy, &bottright))// todo: try with point really on border
				db->drag = 4;
			if(ptinrect(p->xy, &topright))// todo: try with point really on border
				db->drag = 5;
			if(ptinrect(p->xy, &closer))// todo: try with point really in circle
				db->drag = 6;
			if(db->drag != 0){
				db->xy[0] = p->xy[0];
				db->xy[1] = p->xy[1];
				db->rstart = *dstr;
				p->begin &= ~Mouse1;
			}
		}
		if(db->drag != 0 && mousemove(p) == Mouse1){
			intcoord du, dv;
			du = p->xy[0] - db->xy[0];
			dv = p->xy[1] - db->xy[1];
			if(db->drag == 1){
				dstr->v0 = db->rstart.v0 + dv;
				dstr->u0 = db->rstart.u0 + du;
				dstr->vend = db->rstart.vend + dv;
				dstr->uend = db->rstart.uend + du;
			} else if(db->drag == 2){
				dstr->v0 = db->rstart.v0 + dv;
				dstr->u0 = db->rstart.u0 + du;
				if(dstr->v0 > dstr->vend)
					dstr->v0 = dstr->vend;
				if(dstr->u0 > dstr->uend)
					dstr->u0 = dstr->uend;
			} else if(db->drag == 3){
				dstr->vend = db->rstart.vend + dv;
				dstr->u0 = db->rstart.u0 + du;
				if(dstr->vend < dstr->v0)
					dstr->vend = dstr->v0;
				if(dstr->u0 > dstr->uend)
					dstr->u0 = dstr->uend;
			} else if(db->drag == 4){
				dstr->vend = db->rstart.vend + dv;
				dstr->uend = db->rstart.uend + du;
				if(dstr->vend < dstr->v0)
					dstr->vend = dstr->v0;
				if(dstr->uend < dstr->u0)
					dstr->uend = dstr->u0;
			} else if(db->drag == 5){
				dstr->v0 = db->rstart.v0 + dv;
				dstr->uend = db->rstart.uend + du;
				if(dstr->v0 > dstr->vend)
					dstr->v0 = dstr->vend;
				if(dstr->uend < dstr->u0)
					dstr->uend = dstr->u0;
			}
			if(db->drag <= 5)
				didadj |= 1;
		}
		if(db->drag != 0 && mouseend(p) == Mouse1){
			if(db->drag == 6 && ptinrect(p->xy, &closer))
				didadj |= 2;
			db->drag = 0;
		}
	}

	return didadj;
}

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

	Image *closei;
	Image *red;

	red = allocimage(rect(0,0,1,1), color(255,70,70,255));
	closei = allocimage(rect(0,0,bord+pad,bord+pad), color(0,0,0,0));

	blend2(closei,
		rect(
			closei->r.u0,
			closei->r.v0 + db->pad/2,
			closei->r.uend,
			closei->r.v0 + db->pad/2 + (db->bord)
		),
		db->color,
		pt(0,0),
		BlendOver
	);

	blendcircle(
		closei,
		closei->r,
		db->color,
		BlendOver,
		pt(
			(closei->r.u0+closei->r.uend)<<3,
			(closei->r.v0+closei->r.vend)<<3
		),
		(db->bord+db->pad)<<3,
		4
	);

	blendcircle(
		closei,
		closei->r,
		red,
		BlendOver,
		pt(
			(closei->r.u0+closei->r.uend)<<3,
			(closei->r.v0+closei->r.vend)<<3
		),
		(db->bord+db->pad/2)<<3,
		4
	);
	freeimage(red);
	db->closei = closei;
}

