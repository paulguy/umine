#define BOX_INNER_MASK	(0x0F)
#define BOX_STATE_0	(0x00)
#define BOX_STATE_1	(0x01)
#define BOX_STATE_2	(0x02)
#define BOX_STATE_3	(0x03)
#define BOX_STATE_4	(0x04)
#define BOX_STATE_5	(0x05)
#define BOX_STATE_6	(0x06)
#define BOX_STATE_7	(0x07)
#define BOX_STATE_8	(0x08)
#define BOX_STATE_MINE	(0x09)

#define BOX_DISPLAY_MASK	(0x30)
#define BOX_STATE_NORM	(0x00)
#define BOX_STATE_FLAG	(0x10)
#define BOX_STATE_GUESS	(0x20)
#define BOX_STATE_OPEN	(0x30)

#define CHUNK_DIM	(32)
#define CHUNK_SIZE	(CHUNK_DIM * CHUNK_DIM)
#define REGION_DIM	(16)
#define REGION_PHYS_DIM	(CHUNK_DIM * REGION_DIM)
#define REGIONS_PER_FILE	(REGION_DIM * REGION_DIM)
#define MAX_CHUNKS_MUL	(1024)
#define MAX_FILES_MUL	(4)

/* Returns consistent results when a is negative */
#define DIV2(a, b)	(a >= 0? a / b : a / b - 1)
#define MOD2(a, b)	(a >= 0? a % b : b + a % b)

struct chunk;

typedef struct chunk {
	int x, y;
	uint8_t **value;

	struct chunk *prev;
	struct chunk *next;
} chunk;

struct regionFile;

typedef struct regionFile {
	int x, y;
	FILE *file;

	struct regionFile *prev;
	struct regionFile *next;
} regionFile;

typedef struct gamestate {
	/* Permanent */
	int seed;
	int probability;
	uint8_t *emptyChunk;  /* Used for initializing region files */

	/* Chunk stuff */
	int loadedChunks;
	int maxChunks;
	chunk *first;
	chunk *last;

	/* region stuff */
	int openFiles;
	int maxFiles;
	regionFile *firstFile;
	regionFile *lastFile;

	/* Transient */
	int xpos, ypos;
	int viewWidth, viewHeight;
} gamestate;

typedef struct viewrect {
	int w, h;
	uint8_t **value;
} viewrect;

/* Manage game state */
gamestate *initGame(int seed, int probability, int viewWidth, int viewHeight);
void freeGame(gamestate *g);

/* Manage region files/data */
regionFile *initRegion(int x, int y);
void freeRegion(regionFile *r);
void freeLastRegion(gamestate *g);
void addRegion(gamestate *g, regionFile *r);
void bringRegionToFront(gamestate *g, regionFile *r);
regionFile *getRegionFromMemory(gamestate *g, int x, int y);
regionFile *getRegionFromFile(int x, int y);
FILE *openRegion(gamestate *g, int x, int y);
long getOffset(int cx, int cy);

/* Manage chunks */
chunk *initChunk(int x, int y);
void randChunk(chunk *c, int seed);
void freeChunk(chunk *c);
int freeLastChunk(gamestate *g);
int addChunk(gamestate *g, chunk *c);
void bringChunkToFront(gamestate *g, chunk *c);
int chunkToFile(gamestate *g, chunk *c);
chunk *getChunkFromMemory(gamestate *g, int x, int y);
chunk *getChunkFromFile(gamestate *g, int x, int y);

/* Get data */
uint8_t *getPtr(gamestate *g, int x, int y);
uint8_t update(gamestate *g, int x, int y);
viewrect *makeViewRect(gamestate *g, int x, int y);
void freeViewRect(viewrect* v);
