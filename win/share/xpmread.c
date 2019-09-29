/* NetHack may be freely redistributed.  See license for details. */

#include <X11/xpm.h>
#include "hack.h"
#include "tile.h"

extern void *alloc(size_t);

extern const char *tilename(int, int);

static int tiles_across, tiles_down, curr_tiles_across, curr_tiles_down;
static XpmImage image;
static XpmInfo info;

/* Maybe these should be in xpm.h, but there isn't one. */
bool fopen_xpm_file(char *, char *);
bool read_xpm_tile(pixel(*)[]);
int fclose_xpm_file(void);

bool fopen_xpm_file(char *filename, char *type) {
    int i,n;
    int errorcode;
    char fmt[20];
    unsigned long r, g, b;
    const pixel bg = DEFAULT_BACKGROUND;

    if (strcmp(type, RDBMODE)) {
	fprintf(stderr, "using reading routine for non-reading?\n");
	return false;
    }
    errorcode = XpmReadFileToXpmImage(filename, &image, &info);
    if (errorcode != XpmSuccess) {
	fprintf(stderr, "cannot open xpm file %s: %s\n", filename,
	  XpmGetErrorString(errorcode));
	return false;
    }

    if (image.width % tile_x) {
	fprintf(stderr, "error: width %d not divisible by %d\n",
	  image.width, tile_x);
	exit(EXIT_FAILURE);
    }
    tiles_across = image.width / tile_x;
    curr_tiles_across = 0;
    if (image.height % tile_y) {
	fprintf(stderr, "error: height %d not divisible by %d\n",
	  image.height, tile_y);
	/* exit(EXIT_FAILURE) */;
    }
    tiles_down = image.height / tile_y;
    curr_tiles_down = 0;

    for (i = 0; i < image.ncolors; ++i) {
	if (!strcmp(image.colorTable[i].c_color, "None"))
	{
	    ColorMap[CM_RED][i] = bg.r;
	    ColorMap[CM_GREEN][i] = bg.g;
	    ColorMap[CM_BLUE][i] = bg.b;
	}
	else if (image.colorTable[i].c_color[0] == '#')
	{
	    n = strlen(image.colorTable[i].c_color + 1);
	    if (n % 3 || n > 12 || n < 3)
	    {
		fprintf(stderr, "error: Unknown color defn for %s (%s)\n",
		  image.colorTable[i].string, image.colorTable[i].c_color);
		exit(EXIT_FAILURE);
	    }
	    n /= 3;
	    sprintf(fmt, "%%0%dlx%%0%dlx%%0%dlx", n, n, n);
	    if (sscanf(image.colorTable[i].c_color + 1, fmt, &r, &g, &b) != 3)
	    {
		fprintf(stderr, "error: Unknown color defn for %s (%s)\n",
		  image.colorTable[i].string, image.colorTable[i].c_color);
		exit(EXIT_FAILURE);
	    }
	    if (n>=2)
	    {
		ColorMap[CM_RED][i]=r>>(n*4-8);
		ColorMap[CM_GREEN][i]=g>>(n*4-8);
		ColorMap[CM_BLUE][i]=b>>(n*4-8);
	    }
	    else
	    {
		ColorMap[CM_RED][i]=r<<4;
		ColorMap[CM_GREEN][i]=g<<4;
		ColorMap[CM_BLUE][i]=b<<4;
	    }
	}
	else
	{
	    fprintf(stderr, "error: Unknown color defn for %s (%s)\n",
	      image.colorTable[i].string, image.colorTable[i].c_color);
	    exit(EXIT_FAILURE);
	}
    }
    colorsinmap = image.ncolors;

    return true;
}

/* Read a tile.  Returns false when there are no more tiles */
bool read_xpm_tile(pixel (*pixels)[MAX_TILE_X]) {
    int i, j;
    unsigned int *src;

    if (curr_tiles_down >= tiles_down) return false;
    if (curr_tiles_across == tiles_across) {
	curr_tiles_across = 0;
	curr_tiles_down++;
	if (curr_tiles_down >= tiles_down) return false;
    }
    src = image.data + curr_tiles_down * tile_y * image.width +
      curr_tiles_across * tile_x;
    for (j = 0; j < tile_y; j++) {
	for (i = 0; i < tile_x; i++) {
	    pixels[j][i].r = ColorMap[CM_RED][src[j * image.width + i]];
	    pixels[j][i].g = ColorMap[CM_GREEN][src[j * image.width + i]];
	    pixels[j][i].b = ColorMap[CM_BLUE][src[j * image.width + i]];
	}
    }
    curr_tiles_across++;

    /* check for "filler" tile */
    for (j = 0; j < tile_y; j++) {
	for (i = 0; i < tile_x && i < 4; i += 2) {
	    if (pixels[j][i].r != ColorMap[CM_RED][0] ||
		pixels[j][i].g != ColorMap[CM_GREEN][0] ||
		pixels[j][i].b != ColorMap[CM_BLUE][0] ||
		pixels[j][i+1].r != ColorMap[CM_RED][1] ||
		pixels[j][i+1].g != ColorMap[CM_GREEN][1] ||
		pixels[j][i+1].b != ColorMap[CM_BLUE][1])
		    return true;
	}
    }
    return false;
}

int
fclose_xpm_file()
{
    int i;

    XpmFreeXpmImage(&image);
    XpmFreeXpmInfo(&info);

    return false;
}

static char *std_args[] = { "tilemap",	/* dummy argv[0] */
			"monsters.xpm", "monsters.txt",
			"objects.xpm",  "objects.txt",
			"other.xpm",    "other.txt" };

int
main(argc, argv)
int argc;
char *argv[];
{
    int i, argn = 1;
    bool split_mode = false;
    int tile_set = 0;
    const char *name;
    pixel pixels[MAX_TILE_Y][MAX_TILE_X];

    if (argc == 1) {
	argc = SIZE(std_args);
	argv = std_args;
    }

    tile_x = 16;
    tile_y = 16;

    while (argn < argc) {
	if (argv[argn][0] == '-' && argv[argn][1] == 'w') {
	    if (argv[argn][2])
		tile_x = atoi(argv[argn] + 2);
	    else if (argn + 1 < argc)
		tile_x = atoi(argv[++argn]);
	    else {
		fprintf(stderr, "xpm2txt: -w option needs an argument\n");
		exit(EXIT_FAILURE);
	    }
	    argn ++;
	}
	if (argv[argn][0] == '-' && argv[argn][1] == 'h') {
	    if (argv[argn][2])
		tile_y = atoi(argv[argn] + 2);
	    else if (argn + 1 < argc)
		tile_y = atoi(argv[++argn]);
	    else {
		fprintf(stderr, "xpm2txt: -h option needs an argument\n");
		exit(EXIT_FAILURE);
	    }
	    argn ++;
	}
	if (!strcmp(argv[argn], "-s")) {
	    split_mode = true;
	    argn ++;
	}
	else
	    break;
    }

    if (argc - argn != (split_mode ? 4 : 2)) {
	fprintf(stderr, "usage: xpm2txt [-w#] [-h#] xpmfile txtfile\n");
	fprintf(stderr, "       xpm2txt -s xpmfile montxt objtxt othtxt\n");
	exit(EXIT_FAILURE);
    }

    if (!fopen_xpm_file(argv[argn++], RDBMODE))
	exit(EXIT_FAILURE);

    init_colormap();

    if (split_mode)
	tile_set = 1;

    while (argc > argn) {

	if (!fopen_text_file(argv[argn++], WRTMODE)) {
	    fclose_xpm_file();
	    exit(EXIT_FAILURE);
	}

	for(i = 0; ; i++)
	{
	    if (tile_set)
	    {
		name = tilename(tile_set, i);
		if (tile_set < 3 && !strncmp(name, "unknown ", 8))
		    break;
	    }
	    else
		name = "unknown";
	    if (!read_xpm_tile(pixels))
		break;
	    write_text_tile_info(pixels, "tile", i, name);
	}
	if (tile_set)
	    tile_set++;

	fclose_text_file();
    }
    fclose_xpm_file();
    exit(EXIT_SUCCESS);
    /*NOTREACHED*/
    return 0;
}
