// #include "types.h"

#define gcc_pure __attribute__((pure))

static gcc_pure bool isBorder(Dude* dude) {
    // window borders are essentially walls, but whatever
    assert (dude->direction == right ||
            dude->direction == up    ||
            dude->direction == left  ||
            dude->direction == down);
    switch (dude->direction) {
        case right:
            return (dude->x+1 > WINDOW_HEIGHT - (dude->size/2+1));
        case up:
            return (dude->y-1 < dude->size / 2+1);
        case left:
            return (dude->x-1 < dude->size / 2+1);
        case down:
            return (dude->y+1 > WINDOW_HEIGHT - (dude->size/2+1));
    }
    return false; // should never happen
}

static gcc_pure bool isOffTrack(Dude* dude) {
    // returns true if dude can proceed in this direction
    switch (dude->direction) {
        case up:
        case down:
            return dude->x % CORRIDOR_SIZE != 0;
        case left:
        case right:
            return dude->y % CORRIDOR_SIZE != 0;
    }
    abort();
    return true; // should never happen
}

static gcc_pure bool isWall(Dude* dude, Maze* maze) {
    // returns true if dude is wak-blocked
    assert (dude->x > 0);
    assert (dude->y > 0);
    assert (dude->x < WINDOW_HEIGHT);
    assert (dude->y < WINDOW_HEIGHT);
    assert (dude->x % CORRIDOR_SIZE == 0 || dude->y % CORRIDOR_SIZE == 0);
    if (dude->x % CORRIDOR_SIZE != 0 || dude->y % CORRIDOR_SIZE != 0) {
        return false; // walls are only relevant at turning points
    }
    assert (dude->x % CORRIDOR_SIZE == 0);
    assert (dude->y % CORRIDOR_SIZE == 0);

    uint32_t x = dude->x;
    uint32_t y = dude->y;
    switch (dude->direction) {
        case right:
            x += CORRIDOR_SIZE;
            break;
        case up:
            y -= CORRIDOR_SIZE;
            break;
        case down:
            y += CORRIDOR_SIZE;
            break;
        case left:
            x -= CORRIDOR_SIZE;
            break;
    }

    if (maze->tiles_blocked[x/CORRIDOR_SIZE][y/CORRIDOR_SIZE]) {
        return true;
    }
    return false;
}

bool can_proceed(Dude* dude, Maze* maze) {
    if (isBorder(dude)) {
        fprintf(stderr, "can't move through border at %u %u\n", dude->x, dude->y);
        return false;
    }

    if (isOffTrack(dude)) {
        fprintf(stderr, "not on track at %u %u\n", dude->x, dude->y);
        return false;
    }

    if (isWall(dude, maze)) {
        fprintf(stderr, "running into a wall at %u %u\n", dude->x, dude->y);
        return false;
    }
    return true;
}

