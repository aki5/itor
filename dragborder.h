
typedef struct Dragborder Dragborder;
struct Dragborder {
	int hasfocus;
	int needfocus;
	int drag;
	short xy[2];
};

Rect dragborder(Dragborder *db, Rect dstr, Image *color, int bord, int pad, Input *inp, Input *inep);