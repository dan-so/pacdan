/* pacman game with xlib */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "types.h"
#include "movements.c"
#include "maze.c"
#include "dude.c"
#include "ghosties.c"
#include "threading.c"

void initialize_font_and_colours(Display * dpy, XFontStruct** font, GC* gc_fab, GC* gc_black) {
    Colormap colourmap;
    XColor black_colour, fab_colour;
    XGCValues gcv_black, gcv_fab;

    int screen = DefaultScreen(dpy);

    colourmap = DefaultColormap(dpy, screen);
    XParseColor(dpy, colourmap, "rgb:00/00/00", &black_colour);
    XAllocColor(dpy, colourmap, &black_colour);

    XParseColor(dpy, colourmap, "rgb:fa/aa/ab", &fab_colour);
    XAllocColor(dpy, colourmap, &fab_colour);

    gcv_fab.foreground = fab_colour.pixel;
    gcv_fab.background = black_colour.pixel;
    gcv_black.foreground = black_colour.pixel;
    gcv_black.background = black_colour.pixel;

//     GC gc_fab = XCreateGC(dpy, RootWindow(dpy, screen), GCForeground | GCBackground, &gcv_fab);
//     GC gc_black = XCreateGC(dpy, RootWindow(dpy, screen), GCForeground | GCBackground, &gcv_black);
    *gc_fab = XCreateGC(dpy, RootWindow(dpy, screen), GCForeground | GCBackground, &gcv_fab);
    *gc_black = XCreateGC(dpy, RootWindow(dpy, screen), GCForeground | GCBackground, &gcv_black);

    *font = XLoadQueryFont(dpy, "*-18-*");

    if ((*font) == NULL) {
        fputs("font no exist\n", stderr);
        exit(EXIT_FAILURE);
    }
}

void update_score(Display * dpy, Window centre_win, uint64_t foods_eaten) {
    static uint64_t previous_foods_eaten = 42; // anything that isn't 0
    static GC gc_fab;
    static GC gc_black;
    static XFontStruct* font;
    static bool initialized = false;

    if (! initialized) {
        initialize_font_and_colours(dpy, &font, &gc_fab, &gc_black);
        initialized = true;
    }

    if (foods_eaten == previous_foods_eaten) {
        return; // no need to redraw anything
    }

    XTextItem xti;
    xti.chars = "score:";
    xti.nchars = strlen(xti.chars);
    xti.delta = 0;
    xti.font = font->fid;
    XDrawText(dpy, centre_win, gc_fab,
           (195-XTextWidth(font, xti.chars, xti.nchars))/2,
           ((195-(font->ascent+font->descent))/2)+font->ascent-18,
            &xti, 1);

    char text[15];
    snprintf(text, 15, "%ld", previous_foods_eaten * 100); // in games, numbers are always multiplied by 100
    xti.chars = text;
    xti.nchars = strlen(xti.chars);
    XDrawText(dpy, centre_win, gc_black,
           (195-XTextWidth(font, xti.chars, xti.nchars))/2,
           ((195-(font->ascent+font->descent))/2)+font->ascent,
            &xti, 1);

    previous_foods_eaten = foods_eaten;

    snprintf(text, 15, "%ld", foods_eaten * 100);
    xti.chars = text;
    xti.nchars = strlen(xti.chars);
    XDrawText(dpy, centre_win, gc_fab,
           (195-XTextWidth(font, xti.chars, xti.nchars))/2,
           ((195-(font->ascent+font->descent))/2)+font->ascent,
            &xti, 1);
//     XUnloadFont(dpy, font->fid);
}

void handle_keypress(XEvent event, Xevent_thread_data* data) {
    KeySym keysym = XLookupKeysym(&event.xkey, 0);
    thread_lock();
    switch (keysym) {
        case XK_Escape:
        case XK_q:
            data->game_over = true;
            thread_unlock();
            pthread_exit(0); // the end of the game
        case XK_Right:
        case XK_d:
        case XK_l:
            if (! data->dirs->right) {
                data->dirs->right = true;
            }
            break;
        case XK_Up:
        case XK_w:
        case XK_k:
            if (! data->dirs->up) {
                data->dirs->up = true;
            }
            break;
        case XK_Left:
        case XK_a:
        case XK_h:
            if (! data->dirs->left) {
                data->dirs->left = true;
            }
            break;
        case XK_Down:
        case XK_s:
        case XK_j:
            if (! data->dirs->down) {
                data->dirs->down = true;
            }
            break;
        default:
            fputs("that key doesn't do anything.\n", stderr);
    }
    thread_unlock();
}

void handle_keyrelease(XEvent event, Directions* dirs) {
    KeySym keysym = XLookupKeysym(&event.xkey, 0);
    thread_lock();
    switch (keysym) {
        case XK_Right:
        case XK_d:
        case XK_l:
            dirs->right = false;
            break;
        case XK_Up:
        case XK_w:
        case XK_k:
            dirs->up = false;
            break;
        case XK_Left:
        case XK_a:
        case XK_h:
            dirs->left = false;
            break;
        case XK_Down:
        case XK_s:
        case XK_j:
            dirs->down = false;
            break;
        default:
            break;
    }
    thread_unlock();
}

void * handle_xevents(void * arg) {
    Xevent_thread_data* data = (Xevent_thread_data*) arg;
    assert (data != NULL);
    assert (data->dpy_p != NULL);

    XEvent event;
    while (*data->dpy_p != NULL && (! data->game_over)) {
        XNextEvent(*data->dpy_p, &event);
        assert(event.type == Expose ||
               event.type == KeyPress ||
               event.type == KeyRelease ||
               event.type == ButtonPress ||
               event.type == ButtonRelease ||
               event.type == MappingNotify);
        switch (event.type) {
          case Expose:
            thread_signal(); // only necessary the first time, does nothing afterward
            break;
          case KeyPress:
            handle_keypress(event, data);
            break;
          case KeyRelease: // FIXME : prevent the player from holding multiple keys
            handle_keyrelease(event, data->dirs);
            break;
          case ButtonPress:
            puts("button pressed, does nothing");
            break;
          case ButtonRelease:
            puts("button released, does nothing");
            break;
          case MappingNotify:
            XRefreshKeyboardMapping(&event.xmapping);
            break;
          default:
            fprintf(stderr, "received unusual XEvent of type %d\n", event.type);
        }
    }
    pthread_exit(0);
}

void draw_game(Display* dpy, Window win, Maze* maze, Dude* dude, Ghostie ghosties[], uint8_t num_ghosties,
                Window centre_win) {
//     XLockDisplay(dpy);
    draw_maze(dpy, win, maze);
    draw_dude(dpy, win, dude);
    for (uint8_t i = 0; i < num_ghosties; i++) {
        draw_ghostie(dpy, win, &ghosties[i]);
    }
    update_score(dpy, centre_win, dude->foods_eaten);
//     XUnlockDisplay(dpy);
}

int main(void) {
//     mutex = (void *) 0;
    pthread_mutex_init(&mutex, NULL);

    if (0 == XInitThreads()) {
        fputs("not able to initialize multithreading.", stderr);
    }

    Directions dirs = { false, false, false, false };

    Maze maze;
    build_maze(&maze);

    Dude dude = initialize_dude(1, 1, right, &maze);

    uint32_t num_ghosties = 7;
    Ghostie ghosties[num_ghosties];
    ghosties[0] = initialize_ghostie(1, 27, right, &maze);
    ghosties[1] = initialize_ghostie(27, 27, up, &maze);
    ghosties[2] = initialize_ghostie(27, 1, down, &maze);
    ghosties[3] = initialize_ghostie(19, 9, down, &maze);
    ghosties[4] = initialize_ghostie(9, 19, down, &maze);
    ghosties[5] = initialize_ghostie(9, 9, down, &maze);
    ghosties[6] = initialize_ghostie(19, 19, down, &maze);

    Display * display = XOpenDisplay(NULL);
    if (display == NULL) {
        fputs("no display.\n", stderr);
        exit(EXIT_FAILURE);
    }

    int screen = DefaultScreen(display);

    XWindowAttributes root_attrs;
    if (0 == XGetWindowAttributes(display, RootWindow(display, screen), &root_attrs)) {
        fputs("error getting root window attributes.\n", stderr);
        exit(EXIT_FAILURE);
    }

    if (root_attrs.width < WINDOW_HEIGHT || root_attrs.height < WINDOW_HEIGHT) {
        fputs("display isn't big enough.\n", stderr);
        exit(EXIT_FAILURE);
    }

    XSetWindowAttributes attrs;
    attrs.border_pixel = WhitePixel(display, screen);
    attrs.background_pixel = BlackPixel(display, screen);
    attrs.override_redirect = true;
    attrs.colormap = CopyFromParent;
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask;

    Window window = XCreateWindow(display, RootWindow(display, screen),
                root_attrs.width/2-WINDOW_HEIGHT/2, root_attrs.height/2-WINDOW_HEIGHT/2,
                WINDOW_HEIGHT, WINDOW_HEIGHT,
                0, DefaultDepth(display, screen), InputOutput, CopyFromParent,
                CWBackPixel | CWColormap | CWBorderPixel | CWEventMask, &attrs);

    XStoreName(display, window, "pacdan");

    XMapWindow(display, window);

    Window centre_win = XCreateWindow(display, window, 252, 252, 195, 195,
                0, CopyFromParent, CopyFromParent, CopyFromParent,
                CWBackPixel | CWColormap | CWBorderPixel, &attrs);
    XMapWindow(display, centre_win);

    Xevent_thread_data data = {
        .dpy_p = &display,
        .win_p = &window,
        .dirs = &dirs,
        .game_over = false
    };

    draw_game(display, window, &maze, &dude, ghosties, num_ghosties, centre_win);

    pthread_t thread;
    if (0 != pthread_create(&thread, NULL, handle_xevents, &data)) {
        fputs("could not create thread.\n", stderr);
        exit(EXIT_FAILURE);
    }

    thread_wait(); // wait until expose event is received

    const struct timespec tim = {.tv_sec = 0, .tv_nsec = 50000000L};
    while (! data.game_over) {
        assert (maze.food_count + dude.foods_eaten == 388 - num_ghosties);
        draw_game(display, window, &maze, &dude, ghosties, num_ghosties, centre_win);
        XFlush(display);
        nanosleep(&tim, NULL);

        if (0 == maze.food_count) {
            puts("okay, you win.");
            break;
        }

        thread_lock();
        if (dirs.right) {
            move_dude(&dude, right, &maze, display, window);
        } else if (dirs.up) {
            move_dude(&dude, up, &maze, display, window);
        } else if (dirs.left) {
            move_dude(&dude, left, &maze, display, window);
        } else if (dirs.down) {
            move_dude(&dude, down, &maze, display, window);
        }
        for (uint8_t i = 0; i < num_ghosties; i++) {
            move_ghostie(&ghosties[i], &maze, display, window);
        }
        thread_unlock();
    }
    printf("final score is: %lu.\n", dude.foods_eaten*100); // make the score bigger, that's what makes games fun
    pthread_join(thread, NULL);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return 0;
}
