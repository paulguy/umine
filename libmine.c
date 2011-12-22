#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "libmine.h"

gamestate *initGame(int seed, int probability, int viewWidth, int viewHeight) {
	gamestate *g;
	int i;

	g = malloc(sizeof(gamestate));
	if(g == NULL) {
		return(NULL);
	}

	g->emptyChunk = malloc(sizeof(uint8_t) * CHUNK_SIZE);
	if(g->emptyChunk == NULL) {
		free(g);
		return(NULL);
	}
	for(i = 0; i < CHUNK_SIZE; i++) {
		g->emptyChunk[i] = 0;
	}

	g->seed = seed;
	g->probability = probability;
	g->loadedChunks = 0;
	g->maxChunks = ((viewWidth / CHUNK_DIM) + (viewHeight / CHUNK_DIM) + 1) * MAX_CHUNKS_MUL;
	g->first = NULL;
	g->last = NULL;
	g->openFiles = 0;
	g->maxFiles = ((viewWidth / (CHUNK_DIM * REGION_DIM)) + (viewHeight / (CHUNK_DIM * REGION_DIM)) + 1) * MAX_FILES_MUL;
	g->xpos = 0;
	g->ypos = 0;
	g->viewWidth = viewWidth;
	g->viewHeight = viewHeight;

	return(g);
}

void freeGame(gamestate *g) {
	int i;
	chunk *cur = g->first;
	chunk *tmp;

	for(i = 0; i < g->loadedChunks; i++) { /* Unsafe, but if there's an accounting error, this will crash */
		tmp = cur->next;
		free(cur);
		cur = tmp;
	}

	free(g);
}

regionFile *initRegion(int x, int y) {
	regionFile *r;

	r = malloc(sizeof(regionFile));
	if(r == NULL) {
		return(NULL);
	}

	r->x = x;
	r->y = y;
	r->file = NULL;
	r->next = NULL;
	r->prev = NULL;

	return(r);
}

void freeRegion(regionFile *r) {
	if(r->file != NULL) {
		fclose(r->file);
	}

	free(r);
}

void freeLastRegion(gamestate *g) {
	g->lastFile->prev->next = NULL;
	freeRegion(g->lastFile);
}

void addRegion(gamestate *g, regionFile *r) {
	if(g->openFiles >= g->maxFiles) {
		freeLastRegion(g);
		if(g->openFiles >= g->maxFiles) {  /* Gradual closure for window resize */
			freeLastRegion(g);
		}
	}

	r->next = g->firstFile;
	r->prev = NULL;
	g->firstFile->prev = r;
	g->firstFile = r;

	g->openFiles++;
}

void bringRegionToFront(gamestate *g, regionFile *r) {
	if(r == g->firstFile) {
		return;
	}

	if(r->next != NULL) {
		r->next->prev = r->prev;
	}
	r->prev->next = r->next;
	g->firstFile->prev = r;
	r->next = g->firstFile;
	r->prev = NULL;
	g->firstFile = r;
}

regionFile *getRegionFromMemory(gamestate *g, int x, int y) {
	regionFile *r;

	r = g->firstFile;
	while(r != NULL) {
		if(r->x == x && r->y == y) {
			bringRegionToFront(g, r);

			return(r);
		}
	}

	return(NULL);
}

regionFile *getRegionFromFile(int x, int y) {
	regionFile *r;
	char filename[24];

	r = initRegion(x, y);
	if(r == NULL) {
		return(NULL);
	}

	if(snprintf(filename, 24, "%i.%i", x, y) >= 24) {
		free(r);
		return(NULL);
	}
	r->file = fopen(filename, "r+");
	if(r->file == NULL) {
		free(r);
		return(NULL);
	}

	return(r);
}

FILE *openRegion(gamestate *g, int cx, int cy) {
	regionFile *r;
	int rx, ry;
	char filename[24];
	int i;

	rx = DIV2(cx, REGION_DIM);
	ry = DIV2(cy, REGION_DIM);

	r = getRegionFromMemory(g, rx, ry);
	if(r == NULL) {
		r = getRegionFromFile(rx, ry);
		if(r == NULL) {
			r = initRegion(rx, ry);
			if(r == NULL) {
				return(NULL);
			}

			if(snprintf(filename, 24, "%i.%i", cx, cy) >= 24) {
				freeRegion(r);
				return(NULL);
			}
			r->file = fopen(filename, "r+");
			if(r->file == NULL) {
				freeRegion(r);
				return(NULL);
			}
			for(i = 0; i < REGIONS_PER_FILE; i++) {
				if(fwrite(g->emptyChunk, CHUNK_SIZE, 1, r->file) < CHUNK_SIZE) {
					freeRegion(r);
					return(NULL);
				}
			}
		}

		addRegion(g, r);
	}

	return(r->file);
}

long getOffset(int cx, int cy) {
	return(MOD2(cy, REGION_DIM) * (CHUNK_SIZE * REGION_DIM) + (MOD2(cx, REGION_DIM) * CHUNK_SIZE));
}

chunk *initChunk(int cx, int cy) {
	chunk *c;
	int i;

	c = malloc(sizeof(chunk));
	if(c == NULL) {
		return(NULL);
	}

	c->x = cx;
	c->y = cy;
	c->prev = NULL;
	c->next = NULL;

	c->value = malloc(sizeof(uint8_t *) * CHUNK_DIM);
	if(c->value == NULL) {
		return(NULL);
	}

	for(i = 0; i < CHUNK_DIM; i++) {
		c->value[i] = malloc(sizeof(uint8_t) * CHUNK_DIM);
		if(c->value[i] == NULL) {
			return(NULL);
		}
	}

	return(c);
}

void randChunk(chunk *c, int seed) {
	int i, j;

	for(j = 0; j < CHUNK_DIM; j++) {
		for(i = 0; i < CHUNK_DIM; i++) {
			c->value[i][j] = (BOX_STATE_MINE & (MOD2(c->x + i, 3) == 0 || MOD2(c->y + j, 5) == 0)) | BOX_STATE_NORM;
			/* Randomization and shit will go here! */
		}
	}
}

void freeChunk(chunk *c) {
	int i;

	for(i = 0; i < CHUNK_DIM; i++) {
		free(c->value[i]);
	}

	free(c->value);
	free(c);
}

int freeLastChunk(gamestate *g) {
	chunk *t;

	t = g->last->prev;
	t->next = NULL;

	if(chunkToFile(g, g->last) != 0) {
		return(-1);
	}

	freeChunk(g->last);

	g->last = t;

	return(0);
}

int addChunk(gamestate *g, chunk *c) {
	if(g->loadedChunks >= g->maxChunks) {
		if(freeLastChunk(g) != 0) {
			return(-1);
		}
		if(g->loadedChunks >= g->maxChunks) {  /* Gradual closure for window resize */
			if(freeLastChunk(g) != 0) {
				return(-1);
			}
		}
	}

	if(g->first == NULL) {
		g->first = c;
		g->last = c;
	} else {
		c->next = g->first;
		c->prev = NULL;
		g->first->prev = c;
		g->first = c;
	}

	g->loadedChunks++;
	return(0);
}

void bringChunkToFront(gamestate *g, chunk *c) {
	if(c == g->first) {
		return;
	}

	if(c->next != NULL) {
		c->next->prev = c->prev;
	}
	c->prev->next = c->next;
	g->first->prev = c;
	c->next = g->first;
	c->prev = NULL;
	g->first = c;
}

int chunkToFile(gamestate *g, chunk *c) {
	FILE *f;
	long offset;
	int i;

	offset = getOffset(c->x, c->y);
	if(offset == -1) {
		return(-1);
	}

	f = openRegion(g, c->x, c->y);
	if(f == NULL) {
		return(-1);
	}
	fseek(f, offset, SEEK_SET);

	for(i = 0; i < CHUNK_DIM; i++) {
		fwrite(c->value[i], sizeof(uint8_t), CHUNK_DIM, f);
	}

	return(0);
}

chunk *getChunkFromMemory(gamestate *g, int cx, int cy) {
	chunk *t;

	t = g->first;
	while(t != NULL) {
		if(t->x == cx && t->y == cy) {
			bringChunkToFront(g, t);

			return(t);
		}

		t = t->next;
	}

	return(NULL);
}

chunk *getChunkFromFile(gamestate *g, int cx, int cy) {
	chunk *c;
	FILE *f;
	int offset;
	int i;

	offset = getOffset(cx, cy);
	if(offset == -1) {
		return(NULL);
	}

	f = openRegion(g, cx, cy);
	if(f == NULL) {
		return(NULL);
	}

	c = initChunk(cx, cy);
	if(c == NULL) {
		return(NULL);
	}

	fseek(f, offset, SEEK_SET);
	for(i = 0; i < CHUNK_DIM; i++) {
		fread(c->value[i], sizeof(uint8_t), CHUNK_DIM, f);
	}

	return(c);
}

uint8_t *getPtr(gamestate *g, int x, int y) {
	chunk *c;
	int cx, cy;

	cx = DIV2(x, CHUNK_DIM);
	cy = DIV2(y, CHUNK_DIM);

	c = getChunkFromMemory(g, cx, cy);
	if(c == NULL) {
		c = getChunkFromFile(g, cx, cy);

		if(c == NULL) {
			c = initChunk(cx, cy);
			if(c == NULL) {
				return(NULL);
			}

			randChunk(c, g->seed);
		}

		if(addChunk(g, c) != 0) {
			return(NULL);
		}
	}

	return(&(c->value[MOD2(x, CHUNK_DIM)][MOD2(y, CHUNK_DIM)]));
}

uint8_t update(gamestate *g, int x, int y) {
	uint8_t *cur;

	cur = getPtr(g, x, y);
	if((*cur & BOX_STATE_OPEN) && ((*cur & BOX_INNER_MASK) == BOX_STATE_0)) {
		*(getPtr(g, x-1	, y-1	)) |= BOX_STATE_OPEN;
		*(getPtr(g, x	, y-1	)) |= BOX_STATE_OPEN;
		*(getPtr(g, x+1	, y-1	)) |= BOX_STATE_OPEN;

		*(getPtr(g, x-1	, y	)) |= BOX_STATE_OPEN;
		*(getPtr(g, x+1	, y	)) |= BOX_STATE_OPEN;

		*(getPtr(g, x-1	, y+1	)) |= BOX_STATE_OPEN;
		*(getPtr(g, x	, y+1	)) |= BOX_STATE_OPEN;
		*(getPtr(g, x+1	, y+1	)) |= BOX_STATE_OPEN;
	}

	return(*cur);
}

viewrect *makeViewRect(gamestate *g, int x, int y) {
	viewrect *v;
	int i, j;

	g->xpos = x;
	g->ypos = y;

	v = malloc(sizeof(viewrect));
	if(v == NULL) {
		return(NULL);
	}

	v->w = g->viewWidth;
	v->h = g->viewHeight;

	v->value = malloc(sizeof(uint8_t *) * v->h);
	if(v->value == NULL) {
		return(NULL);
	}
	for(i = 0; i < v->w; i++) {
		v->value[i] = malloc(sizeof(uint8_t) * v->w);
		if(v->value[i] == NULL) {
			return(NULL);
		}
		for(j = 0; j < v->h; j++) {
			v->value[i][j] = update(g, x + i, y + j);
		}
	}

	return(v);
}

void freeViewRect(viewrect* v) {
	int i;

	for(i = 0; i < v->h; i++) {
		free(v->value[i]);
	}

	free(v->value);
	free(v);
}
