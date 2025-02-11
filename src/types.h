#define WALL_LIMIT 66
#define WINDOW_HEIGHT 700 // also window width, because the game is a square
#define CORRIDOR_SIZE 25
#define TILES_HEIGHT WINDOW_HEIGHT/CORRIDOR_SIZE + 1

#define gcc_pure __attribute__((pure))

typedef enum { right , up , left , down } Direction;

typedef struct {
    bool right;
    bool up;
    bool left;
    bool down;
} Directions;

typedef struct {
    uint32_t x;
    uint32_t y;
} Point;

typedef struct {
    uint32_t x; // these are the centre of pacman
    uint32_t y;
    uint32_t size; // pacman is a square, this means width and height
    Direction direction;
    GC gc;
} Dude;

/* don't initialize a Wall except by calling build_wall */
typedef struct {
    Point start;
    Point end;
} Wall;

typedef enum { vacant , food , blocked , special } Tile;

typedef struct {
    uint8_t wall_count; // 255 walls ought to suffice
    uint16_t food_count;
    Wall walls[WALL_LIMIT];
    Tile tiles[TILES_HEIGHT][TILES_HEIGHT]; // FIXME use a bitfield instead.
} Maze;

typedef struct {
    Display* const dpy;
    const Window win;
    bool game_over;
    bool paused;
    Directions* dirs;
} Controls_thread_data;
