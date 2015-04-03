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
#include "textedit.h"
#include "dragborder.h"

enum {
	Bord = 10,
	Pad = 10
};

static Textedit mainview;
static Dragborder dragbord;

char *
tryfont(char *fontname)
{
	int fd;
	if((fd = open(fontname, O_RDONLY)) == -1)
		fontname = NULL;
	else
		close(fd);
	return fontname;
}

int
main(int argc, char *argv[])
{
	Input *inp, *inep;
	char *fontname = NULL;
	int fontsize;
	int opt, fgci;
	int fd;

	Image *selcolor;
	Image *fgcolor;
	Image *bordcolor;
	uchar selcval[4];
	uchar bordcval[4];
	
	if(fontname == NULL)
		fontname = tryfont("/System/Library/Fonts/Monaco.dfont");
	if(fontname == NULL)
		fontname = tryfont("/home/aki/Monaco.dfont");
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

	//if(drawinit(60*fontem(),800) == -1){
	if(drawinit(1920, 1080) == -1){
		fprintf(stderr, "drawinit failed\n");
		exit(1);	
	}

#if 0
	fgcolor = allocimage(rect(0,0,1,1), color(0, 0, 0, 255));
	selcval[0] = 80;
	selcval[1] = 90;
	selcval[2] = 70;
	selcval[3] = 127;
	selcolor = allocimage(rect(0,0,1,1), selcval);
#else
	uchar cval[4];
	if(fgci == 0){ // my new favorite green
		cval[0] = 80; cval[1] = 200; cval[2] = 150; cval[3] = 255;
	} else {
		idx2color(fgci, cval);
	}

	bordcval[0] = 80; bordcval[1] = 200; bordcval[2] = 150; bordcval[3] = 255;
	bordcolor = allocimage(rect(0,0,1,1), bordcval);

	fgcolor = allocimage(rect(0,0,1,1), cval);
	selcval[0] = cval[0]/2;
	selcval[1] = cval[1]/2;
	selcval[2] = cval[2]/2;
	selcval[3] = cval[3]/2;
	selcolor = allocimage(rect(0,0,1,1), selcval);
#endif

	int ntext = 0, atext = 0;
	char *text = NULL;
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


	inittextedit(
		&mainview,
		&screen,
		rect(200,200,1000,800),
		Pad,
		text,
		ntext
	);
	mainview.fgcolor = fgcolor;
	mainview.selcolor = selcolor;

	for(;;){
		Rect tmpr;
		inp = drawevents(&inep);

		drawrect(&screen, screen.r, color(0, 0, 0, 0));

		mainview.dstr = dragborder(&dragbord, mainview.dstr, bordcolor, Bord, Pad, inp, inep);

		textedit(&mainview, inp, inep);
	}

	return 0;
}
