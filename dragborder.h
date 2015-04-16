
typedef struct Border Border;
struct Border {
	Image *color;
	Image *corn;
	Image *closei;
	int bord;
	int pad;
	int drag;
	intcoord xy[2];
	Rect rstart;
};

int dragborder(Border *db, Rect *dstr, Input *inp, int ninp);
void drawborder(Border *db, Rect dstr, int op);
void initborder(Border *db, Image *color, int bord, int pad);
void freeborder(Border *db);
