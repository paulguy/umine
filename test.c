#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "libmine.h"

int main(int argc, char **argv) {
	gamestate *g;
	viewrect *v;
	int i, j, k;

	fprintf(stderr, "initGame: ");
	g = initGame(0, 0, 32, 32);
	if(g == NULL) {
		fprintf(stderr, "failed\n");
		exit(-1);
	} else {
		fprintf(stderr, "success\n");
	}

	for(k = -40; k < 40; k++) {
		fprintf(stderr, "makeViewRect %d: ", k);
		v = makeViewRect(g, 0, k);
		if(v == NULL) {
			fprintf(stderr, "failed\n");
			exit(-1);
		} else {
			fprintf(stderr, "success\n");
		}

		for(j = 0; j < v->h; j++) {
			for(i = 0; i < v->w; i++) {
				fprintf(stderr, "%i", v->value[i][j]);
			}
			fprintf(stderr, "\n");
		}

		fprintf(stderr, "freeViewRect: ");
		freeViewRect(v);
		fprintf(stderr, "success\n");
	}

	fprintf(stderr, "freeGame: ");
	freeGame(g);
	fprintf(stderr, "success\n");

	exit(0);
}
