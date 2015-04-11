
typedef struct Dragborder Dragborder;
struct Dragborder {
	int drag;
	intcoord xy[2];
};

Rect dragborder(Dragborder *db, Rect dstr, Image *color, int bord, int pad, Input *inp, Input *inep);
