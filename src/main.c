/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <SDL2/SDL.h>
#include <stdio.h>

int screen_width = 640;
int screen_height = 480;

SDL_Window *g_window = NULL;
SDL_Renderer *g_renderer = NULL;
SDL_Texture *g_wintex = NULL;

int key_cooldown = 0;
const int KEY_COOLDOWN = 3;

int mouse_x;
int mouse_y;

int g_board_width = 200;
int g_board_height = 200;

typedef char cell;

struct board
{
	int width;
	int height;
	cell *cells;
};

struct board make_board(int width, int height)
{
	cell *cells = malloc(width * height * sizeof(cell));
	memset(cells, 0, width * height * sizeof(cell));
	struct board b = {width, height, cells};
	return b;
}

struct board destroy_board(struct board b)
{
	free(b.cells);
	b.width = 0;
	b.height = 0;
	return b;
}

double view_x = 0;
double view_y = 0;

int draw_grid = 1;

double zoom = 10;
const double ZOOM_MAX = 50;
const double ZOOM_MIN = 2;

cell draw_color = 1;

struct board g_board;

void make_bounds(struct board b, int *x, int *y)
{
	*x = abs((*x % b.width + b.height) % b.width);
	*y = abs((*y % b.height + b.height) % b.height);
}

cell get_cell(struct board b, int x, int y)
{
	make_bounds(b, &x, &y);
	return b.cells[y*b.width + x];
}

void set_cell(struct board b, int x, int y, cell c)
{
	make_bounds(b, &x, &y);
	b.cells[y*b.width + x] = c;
}

void draw_view(struct board b, double x, double y, double zoom)
{
	int max_x = x + (screen_width/zoom);
	int max_y = y + (screen_height/zoom);

	for (int dx = x - 1; dx <= max_x; dx++)
	{
		for (int dy = y - 1; dy <= max_y; dy++)
		{
			SDL_Rect cell_rect = 
				{(dx - x) * zoom, (dy - y) * zoom, zoom, zoom};
			
			if (get_cell(b,dx,dy) == 0)
				SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
			else
				SDL_SetRenderDrawColor(g_renderer, 0xFF, 0x00, 0x00, 0xFF);
			
			SDL_RenderFillRect(g_renderer, &cell_rect);
		}
	}
	if (draw_grid)
	{
		SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0xFF);
		for (int dx = x - 1; dx <= max_x; dx++)
			SDL_RenderDrawLine(g_renderer, (dx - x) * zoom, 0, (dx - x) * zoom, screen_height);
		for (int dy = y - 1; dy <= max_y; dy++)
			SDL_RenderDrawLine(g_renderer, 0, (dy - y) * zoom, screen_width, (dy - y) * zoom);
	}
}

void add_zoom(double level)
{
	zoom += level;
	if (zoom > ZOOM_MAX) zoom = ZOOM_MAX;
	else if (zoom < ZOOM_MIN) zoom = ZOOM_MIN;
}

void plot(int sx, int sy)
{
	double x = (double)sx / zoom + view_x - 0.5;
	double y = (double)sy / zoom + view_y - 0.5;

	set_cell(g_board, round(x), round(y), draw_color);
}

void set_view(double x, double y)
{
	view_x = x;
	view_y = y;
	view_x = fmod(fmod(view_x, g_board.width)+g_board.width, g_board.width);
	view_y = fmod(fmod(view_y, g_board.height)+g_board.height, g_board.height);
	if (view_x < 0) view_x += g_board.width;
	if (view_y < 0) view_y += g_board.height;
}

int tickrate = 30;
int current_tick = 0;
int ticking = 0;

const int TICKRATE_MAX = 60;
const int TICKRATE_MIN = 0;
const int TICKRATE_STEP = 5;

void tick()
{
	struct board new_board = make_board(g_board.width, g_board.height);
	// Get the new state for each cell on the board
	for (int y = 0; y < g_board.height; y++)
	{
		for (int x = 0; x < g_board.width; x++)
		{
			int neighbors = 0;
			// Count neighbors by checking the 8 cells around the cell
			for (int nx = -1; nx <= 1; nx++)
			{
				for (int ny = -1; ny <= 1; ny++)
				{
					if (nx == 0 && ny == 0) continue;
					if (get_cell(g_board, x+nx, y+ny) != 0) neighbors++;
				}
			}
			
			set_cell(new_board, x, y, 0);
			if (get_cell(g_board, x, y) != 0)
			{
				if (neighbors >= 2 && neighbors <= 3) set_cell(new_board, x, y, 1);
			}
			else
			{
				if (neighbors == 3) set_cell(new_board, x, y, 1);
			}
		}
	}
	g_board = destroy_board(g_board);
	g_board = new_board;
}

void init(const char *title);
void end();
void mainloop();
void make_wintex();

int main(int argc, char* argv[])
{
	init(argc > 0 ? argv[0] : "automata");
	mainloop();
	end();
	return 0;
}

void print_status()
{
	int hover_x = round(mouse_x / zoom + view_x - 0.5);
	int hover_y = round(mouse_y / zoom + view_y - 0.5);
	// Escape code: Clear line; return to beginning of line
	printf("\33[2K\r%d, %d; %d, %d; %dx; tr:%d; c:%d; %s", 
			(int)view_x, (int)view_y, hover_x, hover_y, (int)zoom, tickrate, 
			(int)draw_color, ticking ? "ticking" : "paused");
	fflush(stdout); // The buffer flushes on newline, but there's no newline.
					// We want the text to display now.
}

void handle_events()
{
	SDL_Event e;
	while (SDL_PollEvent(&e)) 
	{
		switch (e.type)
		{
			case SDL_QUIT:
				end(); break;
			case SDL_WINDOWEVENT:
				if (e.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					screen_width = e.window.data1;
					screen_height = e.window.data2;
					SDL_SetWindowSize(g_window, screen_width, screen_height);

					SDL_DestroyTexture(g_wintex);
					make_wintex();
				}
				break;
			case SDL_MOUSEMOTION:
				if (e.motion.state & SDL_BUTTON_RMASK)
				{
					view_x -= e.motion.xrel / zoom;
					view_y -= e.motion.yrel / zoom;
				}
				if (e.motion.state & SDL_BUTTON_LMASK) plot(e.motion.x, e.motion.y);
				mouse_x = e.motion.x;
				mouse_y = e.motion.y;
				break;
			case SDL_MOUSEWHEEL:
				if (e.wheel.y == 0) break;
				add_zoom(e.wheel.y / abs(e.wheel.y));
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (e.button.button == SDL_BUTTON_LEFT) plot(e.button.x, e.button.y);
				break;
			case SDL_KEYDOWN:
				if (key_cooldown == 0)
				{
					switch (e.key.keysym.sym) 
					{
						case SDLK_SPACE:
							ticking = !ticking;
							break;
						case SDLK_n:
							ticking = 0;
							tick();
							break;
						case SDLK_d:
							draw_color = !draw_color;
							break;
						case SDLK_EQUALS:
							add_zoom(1);
							break;
						case SDLK_MINUS:
							add_zoom(-1);
							break;
						case SDLK_h:
							if (e.key.keysym.mod == KMOD_LCTRL) set_view(0,0);
							break;
						case SDLK_PERIOD:
							tickrate += TICKRATE_STEP;
							if (tickrate > TICKRATE_MAX) tickrate = TICKRATE_MAX;
							break;
						case SDLK_COMMA:
							tickrate -= TICKRATE_STEP;
							if (tickrate < TICKRATE_MIN) tickrate = TICKRATE_MIN;
							break;
						case SDLK_c:
							if (e.key.keysym.mod == KMOD_LCTRL)
							{
								g_board = destroy_board(g_board);
								g_board = make_board(g_board_width, g_board_height);
								printf("\nCleared board\n");
							}
							break;
						case SDLK_g:
							draw_grid = !draw_grid;
							printf(draw_grid ? "\nGrid enabled\n" : "\nGrid disabled\n");
							break;
					}
					
					key_cooldown = KEY_COOLDOWN;
				}
				break;
			default:
				break;
		}
		print_status();
	}
	if (key_cooldown != 0) key_cooldown--;
	
	if (ticking)
	{
		current_tick++;
		if (tickrate != 0) current_tick %= tickrate;
		else current_tick = 0;
		if (current_tick == 0) tick();
	}
}

void mainloop()
{
	g_board = make_board(g_board_width, g_board_height);
	
	while (1)
	{
		handle_events();
		
		// Render to texture for performance
		SDL_SetRenderTarget(g_renderer, g_wintex);
		SDL_RenderClear(g_renderer);
		draw_view(g_board, view_x, view_y, zoom);
		
		SDL_SetRenderTarget(g_renderer, NULL);
		SDL_RenderClear(g_renderer);
		SDL_RenderCopy(g_renderer, g_wintex, NULL, NULL);
		SDL_RenderPresent(g_renderer);
	}
}

void report_error()
{
	fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
	exit(-1);
}

void init(const char *title) 
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) report_error();

	g_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                                   screen_width, screen_height, SDL_WINDOW_SHOWN |
									   SDL_WINDOW_RESIZABLE);
	if (g_window == NULL) report_error();

	g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | 
	                                              SDL_RENDERER_PRESENTVSYNC |
	                                              SDL_RENDERER_TARGETTEXTURE);
	if (g_renderer == NULL) report_error();
	
	make_wintex(); 
	
	SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xFF, 0xFF, 0xFF);
}

void make_wintex()
{
	g_wintex = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
	                              SDL_TEXTUREACCESS_TARGET, screen_width, screen_height);
}

void end()
{
	printf("\n");
	SDL_DestroyWindow(g_window);
	SDL_Quit();
	exit(0);
}
