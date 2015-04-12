
typedef struct Border Border;
struct Border {
	Image *color;
	Image *corn;
	int bord;
	int pad;
	int drag;
	intcoord xy[2];
};

Rect dragborder(Border *db, Rect dstr, Input *inp, Input *inep, int *hitp);
void drawborder(Border *db, Rect dstr);
void initborder(Border *db, Image *color, int bord, int pad);
void freeborder(Border *db);
