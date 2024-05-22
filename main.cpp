#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <filesystem>
#include <windows.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

//rozmiary okna
#define SCREEN_WIDTH	670
#define SCREEN_HEIGHT	780

typedef struct Sdl_
{
	SDL_Surface* screen = NULL;
	SDL_Surface* charset = NULL;
	SDL_Texture* scrtex = NULL;
	SDL_Window* window = NULL;
	SDL_Renderer* renderer = NULL;

	void clear()
	{
		SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
	};

	bool initialize()
	{
		// okno konsoli nie jest widoczne, jezeli chcemy zobaczyc
		// komunikaty wypisywane printf-em trzeba w opcjach:
		// project -> szablon2 properties -> Linker -> System -> Subsystem
		// zmienic na "Console"
		/*printf("wyjscie printfa trafia do tego okienka\n");
		printf("printf output goes here\n");*/

		if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			printf("SDL_Init error: %s\n", SDL_GetError());
			return false;
		}

		// tryb pelnoekranowy           
		int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
		if (rc != 0) {
			SDL_Quit();
			printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
			return false;
		};

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

		SDL_SetWindowTitle(window, "Spy Hunter");

		screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

		scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

		// wylaczenie widocznosci kursora myszy
		SDL_ShowCursor(SDL_DISABLE);

		return true;
	};

	bool load_charset()
	{
		// wczytanie obrazka cs8x8.bmp
		charset = SDL_LoadBMP("./cs8x8.bmp");
		if (charset == NULL) {
			printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
			SDL_FreeSurface(screen);
			SDL_DestroyTexture(scrtex);
			SDL_DestroyWindow(window);
			SDL_DestroyRenderer(renderer);
			SDL_Quit();
			return false;
		};
		SDL_SetColorKey(charset, true, 0x000000);

		return true;
	};

	bool load_sdl()
	{
		if(!initialize())
		{
			return false;
		}
		if(!load_charset())
		{
			return false;
		}

		return true;
	};

} Sdl;

// narysowanie napisu txt na powierzchni screen, zaczynajac od punktu (x, y)
// charset to bitmapa 128x128 zawierajaca znaki
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt srodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// rysowanie pojedynczego piksela
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};


// rysowanie linii o dlugosci l w pionie (gdy dx = 0, dy = 1) 
// badz poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};


// rysowanie prostokata o dlugosci bokow l i k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

int check_if_save_file(char *string)
{
  char * file_ext = strrchr(string, '.');

  if(file_ext != NULL)
    return(strcmp(file_ext, ".save"));

  return(1);
};

typedef struct _Saves
{
	char save_files[6][1024] = { 0 };
	int size = 0;
	int use = -1;

	void list_save_files()
	{
		int idx = 0;

		clear();
		WIN32_FIND_DATA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		TCHAR sDir[16] = TEXT(".\\*.save");

 		hFind = FindFirstFile(sDir, &ffd);
		if(hFind == INVALID_HANDLE_VALUE)
		{
			return;
		}

		do
		{
			char fName[32] = { 0 };

			wcstombs(fName, ffd.cFileName, 63);
			strcpy(save_files[idx++], fName);
  		} while(FindNextFile(hFind, &ffd) != 0 && (idx < 6));

		size = idx;
		if(size > 0)
		{
			use = 0;
		}
		else
		{
			use = -1;
		}
	};

	void clear()
	{
		for(int i=0; i< size; i++)
		{
			memset(save_files[i], 0, sizeof(save_files[i]));
		}
		use = -1;
	};

	void change()
	{
		if(size > 0)
		{
			use++;
			use %= size;
		}
	};

	char* get_file_name()
	{
		if(use > -1)
		{
			return save_files[use];
		}
		return NULL;
	};

} Saves;

typedef struct _Keys
{
	bool key_up = false, key_down = false, key_left = false, key_right = false;
} Keys;

// game
typedef struct _Game
{
	double delta = 0;
	double points = 0;
	double multiplier = 1;
	double point_cooldown = 3;
	double worldTime = 0;
	double enemy_timer = 4;
	int lives = 1;
	int quit = 0;
	bool pause = false;
	bool load = false;
	Saves saves;

	void reset()
	{
		worldTime = 0;
		delta = 0;
		points = 0;
		load = false;
		saves.clear();
	};

	void setDelta(double d)
	{
		delta = d;
		worldTime += delta;
	};

	//score
	int calc_score(int map_pace)
	{
		if ((int)(points / 10) * 10 < 0) points = 0;
		if (point_cooldown >= 3) points += ((double)map_pace / 6 * multiplier);
		return (int)(points/10)*10;
	};

} Game;

SDL_Surface* get_sprite(char* filename, Game* game)
{
	char path[30] = "./";
	SDL_Surface* sprite = SDL_LoadBMP(strcat(path, filename));

	if (sprite == NULL) {
		printf("SDL_LoadBMP(%s) error: %s\n",filename, SDL_GetError());
		game->quit = 1;
	}
	else
	{
		SDL_SetColorKey(sprite, SDL_TRUE, SDL_MapRGB(sprite->format, 255, 174, 200));
		return sprite;
	}
	return NULL;
};

typedef struct _Map
{
	int backgroundOffset = 0;
	int pos_x = SCREEN_WIDTH / 2;
	int pos_y = SCREEN_HEIGHT / 2;
	int pace = 13;
	int maxOffset = 0;
	SDL_Surface* map_sprite = NULL;

	void load_map_sprite(Game* game)
	{
		map_sprite = get_sprite("Mapa.bmp", game);
		if(map_sprite)
		{
			maxOffset = map_sprite->h;
		}
	};

	void drawMap(SDL_Surface* screen)
	{

		if(backgroundOffset > maxOffset)
		{
			backgroundOffset = 0;
		}
		DrawSurface(screen, map_sprite, pos_x, backgroundOffset);
		DrawSurface(screen, map_sprite, pos_x, backgroundOffset - maxOffset);
	};

	void reset()
	{
		backgroundOffset = 0;
	};

	void clear()
	{
		SDL_FreeSurface(map_sprite);
	};

	void update_offset(double delta)
	{
		backgroundOffset += (delta * 100 * pace);
	};

	void map_move(double delta)
	{
		if (pos_y >= SCREEN_HEIGHT * 1.5)
		{
			pos_y = (SCREEN_HEIGHT / -2);
		}
		else
		{
			pos_y += (pace * delta);
		}
	};
} Map;

bool isGrassColor(SDL_Color* color)
{
	return (color->r == 32 && color->g == 64 && color->b == 0);
}

enum CarType {PLAYER, ENEMY, MISSILE, NEUTRAL, NONE};

typedef struct _Car
{
	CarType allocation = NONE;
	int pos_x = SCREEN_WIDTH/2;
	int pos_y = SCREEN_HEIGHT/2;
	double pace_multiplier_1 = 1;
	double pace_multiplier_2 = 1;
	double pace_multiplier_3 = 1;
	int usage = 0;
	SDL_Surface* sprite[2] = {0};
	double bulletTimer = 1;

	//obliczanie predkosci pojazdu
	double pace() 
	{
		return (6 * pace_multiplier_1 * pace_multiplier_2 * pace_multiplier_3);
	};

	void load_sprite(int number, Game *game)
	{
		char filename[30] = {0};

		if (allocation == NEUTRAL)
		{
			char file[] = "AutkoYellow";
			char ext[] = ".bmp";

			sprintf(filename, "%s%d%s", file, number, ext);

			sprite[number-1] = get_sprite(filename, game);
		}
		if (allocation == PLAYER)
		{
			char file[] = "AutkoRed";
			char ext[] = ".bmp";

			sprintf(filename, "%s%d%s", file, number, ext);

			sprite[number-1] = get_sprite(filename, game);
		}
		if (allocation == MISSILE)
		{
			sprite[number-1] = get_sprite("missileStraight.bmp", game);
		}
		if (allocation == ENEMY)
		{
			char file[] = "AutkoBlue";
			char ext[] = ".bmp";

			sprintf(filename, "%s%d%s", file, number, ext);

			sprite[number - 1] = get_sprite(filename, game);
		}

	};

	//wspolrzedne do obliczania kolizji
	int coll_x_topleft(Game *game)
	{
		return pos_x - sprite[0]->w/2;
	};

	int coll_y_topleft(Game* game)
	{
		return pos_y - sprite[0]->h / 2;
	};

	int coll_x_botright(Game* game)
	{
		return pos_x + sprite[0]->w / 2;
	};

	int coll_y_botright(Game* game)
	{
		return pos_y + sprite[0]->h / 2;
	};

	void clear() {
		for(int i=0; i<2; i++)
		{
			if (sprite[i] != NULL)
			{
				SDL_FreeSurface(sprite[i]);
			}
		}
	};

	void bullet_delay(double delta)
	{
		bulletTimer += delta*2;
	};

	//usuniecie przeciwnika i ustawienie jego pozycji w losowym miejscu poza mapa
	void kill_enemy()
	{
		pos_x = 200 + rand() % 314;
		if (allocation == NEUTRAL)
		{
			pos_y = (150 + rand() % 900) * -1;
			pace_multiplier_1 = 1 + ((rand()%4)*0.1);
		}
		if (allocation == ENEMY)
		{
			pos_y = (SCREEN_HEIGHT + 300 + rand() % 1000);
			pace_multiplier_1 = 0.3;
		}
	};

	void enemy_move(Game* game, double delta)
	{
		if (game->worldTime > 3)
		{
			if (allocation == NEUTRAL)
			{
				if (pos_y >= SCREEN_HEIGHT + 50 || pos_y <= -1000)
				{
					kill_enemy();
				}
				else
				{
					pos_y += (delta * pace());
				}
			}
			else if (allocation == ENEMY)
			{
				if (pos_y <= -200)
				{
					kill_enemy();
				}
				else
				{
					pos_y -= (delta * pace());
				}
			}
		}
	};

	//
	SDL_Color check_surface(const SDL_Surface* screen)
	{
		// Bytes per pixel
		const Uint8 bpp = screen->format->BytesPerPixel;
		
		Uint8* pPixel = (Uint8*)screen->pixels + pos_y * screen->pitch + pos_x * bpp;

		Uint32 pixelData = *(Uint32*)pPixel;

		SDL_Color color = { 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE };

		//Zwrocenie RGB danego piksela
		SDL_GetRGB(pixelData, screen->format, &color.r, &color.g, &color.b);
		return color;
	};

	//sprawdzanie powierzchni pod przeciwnikiem
	void check_enemy_surface(const SDL_Surface* screen, Game *game)
	{
		if(allocation != NEUTRAL && allocation != ENEMY)
			return;

		if (pos_x > 0 && pos_x <= SCREEN_WIDTH
			&& pos_y > 0 && pos_y <= SCREEN_HEIGHT)
		{
			SDL_Color color = check_surface(screen);
			if (isGrassColor(&color))
			{
				if (allocation == NEUTRAL)
				{
					pace_multiplier_2 = 2;
					game->point_cooldown = 1;
				}
				else if (allocation == ENEMY)
				{
					pace_multiplier_2 = 0.05;
					if (game->enemy_timer > 8)
					{
						game->points += 2000;
						game->enemy_timer = 0;
					}
				}
			}
			else
			{
				pace_multiplier_2 = 1;
			}
		}
		else
		{
			pace_multiplier_2 = 1;
		}
	};
} Car;

void crush(Car* neutral, Car* player, Game* game)
{
	if (player->coll_x_topleft(game) < neutral->coll_x_botright(game)
		&& player->coll_x_botright(game) > neutral->coll_x_topleft(game)
		&& player->coll_y_topleft(game) < neutral->coll_y_botright(game)
		&& player->coll_y_botright(game) > neutral->coll_y_topleft(game))
	{
		if (player->allocation == PLAYER && neutral->allocation == NEUTRAL) game->point_cooldown = 2;
		if (player->allocation != MISSILE && neutral->allocation != MISSILE)
		{
			if (player->pos_x < neutral->pos_x)
			{
				neutral->pos_x += 6;
			}
			if (player->pos_x > neutral->pos_x)
			{
				neutral->pos_x -= 6;
			}
			if (player->pos_y < neutral->pos_y)
			{
				neutral->pos_y += 6;
			}
			if (player->pos_y > neutral->pos_y)
			{
				neutral->pos_y -= 6;
			}
		}
		if (player->allocation == MISSILE)
		{
			player->usage = 0;
			neutral->kill_enemy();
			if (neutral->allocation == NEUTRAL) game->point_cooldown = 0;
			if (neutral->allocation == ENEMY) game->points += 1000;
		}

	}
	else if (player->allocation == PLAYER) player->pace_multiplier_2 = 1;
};

//sprawdzenie czy przeciwnicy nie nakladaja sie na siebie
void check_enemies(Car* car1, Car* car2, Game *game)
{
	if (car1->coll_x_topleft(game) < car2->coll_x_botright(game)
		&& car1->coll_x_botright(game) > car2->coll_x_topleft(game)
		&& (car1->pos_y < 0 || car1->pos_y > SCREEN_HEIGHT) && car2->pos_y < 0)
	{
		if (car1->pos_y > car2->pos_y) car2->kill_enemy();
		else car1->kill_enemy();
	}
};

typedef struct _Bullets
{
	static const size_t size = 4;
	Car projectiles[size];

	void missile_move()
	{
		for (int i = 0; i < size; i++)
		{
			if (projectiles[i].usage == 1)
			{
				projectiles[i].pos_y -= projectiles[i].pace() * 1.2;
				if (projectiles[i].pos_y < -20)
				{
					projectiles[i].usage = 0;
				}
			}
		}
	};

	void set_pace(double pace)
	{
		for (int i = 0; i < size; i++)
		{
			projectiles[i].pace_multiplier_1 = pace;
		}
	};

	void draw(SDL_Surface* screen)
	{
		for (int i = 0; i < size; i++)
		{
			if (projectiles[i].usage == 1)
			{
				DrawSurface(screen, projectiles[i].sprite[0], projectiles[i].pos_x, projectiles[i].pos_y);
			}
		}
	};

	void clear()
	{
		for (int i = 0; i < size; i++)
		{
			projectiles[i].clear();
		}
	};

	void create_missile(int x, int y)
	{
		for (int i = 0; i < size; i++)
		{
			Car* projectile = &(projectiles[i]);
			if (projectile->usage == 0)
			{
				projectile->usage = 1;
				projectile->pos_x = x;
				projectile->pos_y = y;
				return;
			}
		}
	};

	void shoot(Car* player)
	{
		if (player->bulletTimer >= 1)
		{
			create_missile(player->pos_x, player->pos_y);
			player->bulletTimer  = 0;
		}
	};

} Bullets;

//sprawdzenie czy pojazd znajduje sie na ekranie
bool on_screen(Car* car)
{
	if (car->pos_x > 0 && car->pos_x <= SCREEN_WIDTH && car->pos_y > 0 && car->pos_y <= SCREEN_HEIGHT - 0) return true;
	return false;
}

typedef struct _Cars
{
	Car player;
	Car neutral1;
	Car neutral2;
	Car neutral3;
	Car enemy;

	void init_cars(Bullets* projectiles, Game* game)
	{
		for (int i = 0; i < projectiles->size; i++)
		{
			Car* bullet = &(projectiles->projectiles[i]);
			bullet->allocation = MISSILE;
			bullet->load_sprite(1, game);
		}

		player.allocation = PLAYER;
		neutral1.allocation = neutral2.allocation = neutral3.allocation = NEUTRAL;
		enemy.allocation = ENEMY;
		for(int i = 1; i < 3; i++)
		{
			player.load_sprite(i, game);
			neutral1.load_sprite(i, game);
			neutral2.load_sprite(i, game);
			neutral3.load_sprite(i, game);
			enemy.load_sprite(i, game);
		}
	};

	//naprzemienne sprite pojazdów 
	void animate_cars(SDL_Surface* screen, double worldTime)
	{
		int sprite_index = 1;
		if ((int)(worldTime * 15) % 2)
		{
			sprite_index = 0;
		}

		DrawSurface(screen, player.sprite[sprite_index], player.pos_x, player.pos_y);
		DrawSurface(screen, enemy.sprite[sprite_index], enemy.pos_x, enemy.pos_y);

		DrawSurface(screen, neutral1.sprite[sprite_index], neutral1.pos_x, neutral1.pos_y);
		DrawSurface(screen, neutral1.sprite[sprite_index], neutral2.pos_x, neutral2.pos_y);
		DrawSurface(screen, neutral1.sprite[sprite_index], neutral3.pos_x, neutral3.pos_y);
	};

	void kill_enemies()
	{
		neutral1.kill_enemy();
		neutral2.kill_enemy();
		neutral3.kill_enemy();
		enemy.kill_enemy();
	};

	void clear()
	{
		player.clear();
		neutral1.clear();
		neutral2.clear();
		neutral3.clear();
		enemy.clear();
	};

	void enemy_moves(Game* game, double delta)
	{
		neutral1.enemy_move(game, delta * 100);
		neutral2.enemy_move(game, delta * 100);
		neutral3.enemy_move(game, delta * 100);
		enemy.enemy_move(game, delta * 100);
	};

	void crush_enemies(Car* car, Game* game)
	{
		crush(&neutral1, car, game);
		crush(&neutral2, car, game);
		crush(&neutral3, car, game);
		crush(&enemy, car, game);
	};

	void enemy_overlap(Game *game)
	{
		check_enemies(&neutral1, &neutral2, game);
		check_enemies(&neutral2, &neutral3, game);
		check_enemies(&neutral1, &neutral3, game);
		check_enemies(&enemy, &neutral1, game);
		check_enemies(&enemy, &neutral2, game);
		check_enemies(&enemy, &neutral3, game);
	};

	void check_enemies_surface(SDL_Surface* screen, Game *game)
	{
		neutral1.check_enemy_surface(screen, game);
		neutral2.check_enemy_surface(screen, game);
		neutral3.check_enemy_surface(screen, game);
		enemy.check_enemy_surface(screen, game);
	}

	void reset_player()
	{
		player.pos_x = SCREEN_WIDTH / 2;
		player.pos_y = SCREEN_HEIGHT / 2;
	}

	//zachowanie przeciwnika
	void enemy_attack(Car* player, Car* enemy, Game* game, const SDL_Surface* screen)
	{
		if (on_screen(enemy))
		{
			SDL_Color color1 = enemy->check_surface(screen);
			SDL_Color color2 = player->check_surface(screen);
			if (enemy->pos_y >= player->pos_y - 20)
			{
				if (player->pos_x <= enemy->pos_x) enemy->pos_x --;
				if (player->pos_x > enemy->pos_x) enemy->pos_x ++;
			}
		}

		crush(player, enemy, game);
	}

} Cars;


void pace_on_grass(Car* car, const SDL_Surface* screen)
{
	SDL_Color color = car->check_surface(screen);
	if (isGrassColor(&color))
	{
		if (car->allocation == NEUTRAL) car->pace_multiplier_3 = 0.5;
		else if (car->allocation == ENEMY) car->pace_multiplier_3 = -2;
	}
	else
	{
		if (car->allocation == NEUTRAL) car->pace_multiplier_3 = -0.5;
		else if (car->allocation == ENEMY) car->pace_multiplier_3 = 2;
	}
}

//zmiana predkosci zaleznie od polozenia pojazdow
void modify_pace(const SDL_Surface* screen, Map* map, Game* game, Cars* cars, Bullets* projectiles)
{
	Car* player = &(cars->player);
	Car* neutral1 = &(cars->neutral1);
	Car* neutral2 = &(cars->neutral2);
	Car* neutral3 = &(cars->neutral3);
	Car* enemy = &(cars->enemy);

	SDL_Color color = player->check_surface(screen);
	if (isGrassColor(&color))
	{
		game->multiplier = 0;
		player->pace_multiplier_1 = 0.5;
		map->pace = 6;
		projectiles->set_pace(2);
		if (on_screen(neutral1)) pace_on_grass(neutral1, screen);

		if (on_screen(neutral2)) pace_on_grass(neutral2, screen);

		if (on_screen(neutral3)) pace_on_grass(neutral3, screen);

		if (on_screen(enemy)) pace_on_grass(enemy, screen);
	}
	else
	{
		game->multiplier = player->pace_multiplier_1 = neutral1->pace_multiplier_3 = neutral2->pace_multiplier_3 = neutral3->pace_multiplier_3 = enemy->pace_multiplier_3 = 1;
		map->pace = 13;
		projectiles->set_pace(1);
	}
};

void copy_car(Car* src, Car* dst)
{
	dst->pos_x = src->pos_x;
	dst->pos_y = src->pos_y;
	dst->pace_multiplier_1 = src->pace_multiplier_1;
	dst->pace_multiplier_2 = src->pace_multiplier_2;
	dst->pace_multiplier_3 = src->pace_multiplier_3;
	dst->bulletTimer = src->bulletTimer;
	dst->usage = src->usage;
};

void copy_game(Game* src, Game* dst)
{
	dst->delta = src->delta;
	dst->worldTime = src->worldTime;
	dst->lives = src->lives;
	dst->multiplier = src->multiplier;
	dst->pause = src->pause;
	dst->points = src->points;
	dst->quit = src->quit;
};

void copy_map(Map* src, Map* dst)
{
	dst->backgroundOffset = src->backgroundOffset;
	dst->pace = src->pace;
	dst->pos_x = src->pos_x;
	dst->pos_y = src->pos_y;
};

void copy_cars(Cars* src, Cars* dst)
{
	copy_car(&(src->player), &(dst->player));
	copy_car(&(src->neutral1), &(dst->neutral1));
	copy_car(&(src->neutral2), &(dst->neutral2));
	copy_car(&(src->neutral3), &(dst->neutral3));
	copy_car(&(src->enemy), &(dst->enemy));
};

void copy_bullets(Bullets* src, Bullets* dst)
{
	for(int i=0; i<dst->size; i++)
	{
		copy_car(&(src->projectiles[i]), &(dst->projectiles[i]));
	}
};

//wyrysowanie UI
typedef struct _Menu
{
	// header
	void drawHeader(SDL_Surface* screen, SDL_Surface* charset, Uint32 outlineColor, Uint32 fillColor, Game *game, Map* map)
	{
		char text[128] = { 0 };
		DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 30, outlineColor, fillColor);
		sprintf(text, "Czas = %.1lf s  Wynik = %d", game->worldTime, game->calc_score(map->pace));
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 12, text, charset);
		/*sprintf(text, "Esc - wyjscie, n - nowa gra, strzalki - ruch, spacja - strzal, p - pauza");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
		sprintf(text, "s - zapis, l - lista zapisow, tab - wybor, enter - potwierdz, l - anuluj");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 42, text, charset);*/
	};

	// footer
	void drawFooter(SDL_Surface* screen, SDL_Surface* charset, Game* game, Uint32 outlineColor, Uint32 fillColor)
	{
		char text[128] = { 0 };
		DrawRectangle(screen, 4, SCREEN_HEIGHT - 36, SCREEN_WIDTH - 8, 36, outlineColor, fillColor);
		sprintf(text, "Zaimplementowane podpunkty: a, b, c, d, e, f, g1, g2, i, j, k, l");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT - 26, text, charset);
	};

	// pause
	void drawPause(SDL_Surface* screen, SDL_Surface* charset, Uint32 outlineColor, Uint32 fillColor)
	{
		char text[128] = { 0 };
		DrawRectangle(screen, 44, SCREEN_HEIGHT/2, SCREEN_WIDTH - 88, 36, outlineColor, fillColor);
		sprintf(text, "P A U Z A");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT/2 + 14, text, charset);
	};

	//list of saves
	void drawSaveFiles(SDL_Surface* screen, SDL_Surface* charset, Saves* saves, Uint32 outlineColor, Uint32 fillColor)
	{
		char text[128] = { 0 };
		DrawRectangle(screen, 44, SCREEN_HEIGHT - 300 - 12, SCREEN_WIDTH - 88, saves->size * 12 + 50, outlineColor, fillColor);
		sprintf(text, "tab - wybieranie zapisow, enter - potwierdzenie, l - anulowanie");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT - 300, text, charset);
		sprintf(text, "Zapisane stany gry:");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT - 280, text, charset);
		for(int k=0; k < saves->size; k++)
		{
			sprintf(text, "%c (%d) %s", (saves->use == k) ? '*' : ' ', k, saves->save_files[k]);
			DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT - 280 + (k+1) * 12, text, charset);
		}
	};

} Menu;

void move(Keys* keys, Cars* cars, Game* game, Bullets* projectiles, const SDL_Surface* screen)
{
	Car* player = &(cars->player);
	Car* enemy = &(cars->enemy);
	if (keys->key_right && player->pos_x <= SCREEN_WIDTH)
	{
		player->pos_x+=player->pace();
	}
	if (keys->key_left && player->pos_x >= 0)
	{
		player->pos_x-= player->pace();
	}
	if (keys->key_down && player->pos_y <= SCREEN_HEIGHT)
	{
		player->pos_y+= player->pace();
	}
	if (keys->key_up && player->pos_y >= 0)
	{
		player->pos_y-= player->pace()/2;
	}

	cars->crush_enemies(player, game);
	
	for (int i = 0; i < projectiles->size; i++)
	{
		Car* bullet = &(projectiles->projectiles[i]);
		if (bullet->usage == 1)
		{
			cars->crush_enemies(bullet, game);
		}
	}
	
	cars->enemy_attack(player, enemy, game, screen);
};

void save_game(Game* game, Map* map, Cars* cars, Bullets* bullets)
{
	time_t rawtime;
  	struct tm * timeinfo;
  	char file_name[80] = { 0 };

  	time(&rawtime);
  	timeinfo = localtime(&rawtime);
	strftime(file_name, sizeof(file_name), "%d-%m-%Y_%H_%M_%S\.save", timeinfo);

	FILE* fp = fopen(file_name, "w");
	if(fp)
	{
		fwrite(game, sizeof(Game), 1, fp);
		fwrite(map, sizeof(Map), 1, fp);
		fwrite(cars, sizeof(Cars), 1, fp);
		fwrite(bullets, sizeof(Bullets), 1, fp);

		fclose(fp);
	}
}

void load_game(char* file_name, Game* game, Map* map, Cars* cars, Bullets* bullets) {

	Game _game;
	Map _map;
	Cars _cars;
	Bullets _bullets;
	bool read_ok = false;

	if(file_name == NULL)
	{
		return;
	}

	FILE* fp = fopen(file_name, "r");
	if(fp)
	{
		
		fread(&_game, sizeof(Game), 1, fp);
		fread(&_map, sizeof(Map), 1, fp);
		fread(&_cars, sizeof(Cars), 1, fp);
		fread(&_bullets, sizeof(Bullets), 1, fp);

		fclose(fp);
		read_ok = true;
	}

	if(read_ok)
	{
		copy_game(&_game, game);
		copy_map(&_map, map);
		copy_cars(&_cars, cars);
		copy_bullets(&_bullets, bullets);
	}
}

//handling of events (if there were any)
void handle_event(SDL_Event* event, Keys* keys, Game* game, Map* map, Cars* cars, Bullets* projectiles)
{
	while (SDL_PollEvent(event)) {
		switch (event->type) {
			case SDL_KEYDOWN:
				if(!game->pause)
				{
					if (event->key.keysym.sym == SDLK_UP) keys->key_up = true;
					else if (event->key.keysym.sym == SDLK_DOWN) keys->key_down = true;
					else if (event->key.keysym.sym == SDLK_RIGHT) keys->key_right = true;
					else if (event->key.keysym.sym == SDLK_LEFT) keys->key_left = true;
					if (event->key.keysym.sym == SDLK_SPACE) projectiles->shoot(&(cars->player));
				}
				break;
			case SDL_KEYUP:
				if(!game->pause)
				{
					if (event->key.keysym.sym == SDLK_n)
					{
						game->reset();
						map->reset();
						cars->kill_enemies();
						cars->reset_player();
						for (int i = 0; i < projectiles->size; i++) projectiles->projectiles[i].usage = 0;
					}
					else if (event->key.keysym.sym == SDLK_s && game->load == false) save_game(game, map, cars, projectiles);
					else if (event->key.keysym.sym == SDLK_l)
					{
						if(game->load)
						{
							game->load = false;
						}
						else
						{
							game->saves.list_save_files();
							if(game->saves.size > 0)
							{
								game->load = true;
							}
						}
					}
					else if (event->key.keysym.sym == SDLK_TAB && game->load == true) game->saves.change();
					else if (event->key.keysym.sym == SDLK_RETURN && game->load == true)
					{
						game->load = false;
						char* save_file = game->saves.get_file_name();
						load_game(save_file, game, map, cars, projectiles);
					}
					else if (event->key.keysym.sym == SDLK_UP) keys->key_up = false;
					else if (event->key.keysym.sym == SDLK_DOWN) keys->key_down = false;
					else if (event->key.keysym.sym == SDLK_RIGHT) keys->key_right = false;
					else if (event->key.keysym.sym == SDLK_LEFT) keys->key_left = false;
				}
				if (event->key.keysym.sym == SDLK_ESCAPE) game->quit = 1;
				else if (event->key.keysym.sym == SDLK_p) game->pause = !game->pause;
				break;
			case SDL_QUIT:
				game->quit = 1;
				break;
		};
	};
};

void drawGame(Sdl* sdl, Game* game, Map* map, Cars* cars, Bullets* projectiles, Menu* menu)
{
	SDL_Surface* screen = sdl->screen;

	static const int czarny = SDL_MapRGB(screen->format, 0x1b, 0x1b, 0x1b);
	static const int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	static const int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

	if(!game->pause)
	{
		SDL_FillRect(screen, NULL, czarny);
		map->drawMap(screen);

		modify_pace(screen, map, game, cars, projectiles);
		
		cars->check_enemies_surface(screen, game);
		cars->animate_cars(screen, game->worldTime);

		menu->drawHeader(screen, sdl->charset, czerwony, niebieski, game, map);

		menu->drawFooter(screen, sdl->charset, game, czerwony, niebieski);

		projectiles->draw(screen);
	}
	else
	{
		menu->drawPause(sdl->screen, sdl->charset, czerwony, niebieski);
	}

	if(game->load)
	{
		menu->drawSaveFiles(sdl->screen, sdl->charset, &(game->saves), czerwony, niebieski);
	}


	SDL_UpdateTexture(sdl->scrtex, NULL, screen->pixels, screen->pitch);
	SDL_RenderCopy(sdl->renderer, sdl->scrtex, NULL, NULL);
	SDL_RenderPresent(sdl->renderer);
};

void computeFrame(SDL_Surface* screen, Keys* keys, Game* game, Map* map, Cars* cars, Bullets* projectiles)
{
	cars->player.bullet_delay(game->delta * 2);
	game->point_cooldown += game->delta;
	game->enemy_timer += game->delta;
	map->update_offset(game->delta);

	cars->enemy_moves(game, game->delta);
	cars->enemy_overlap(game);

	projectiles->missile_move();

	move(keys, cars, game, projectiles, screen);
};

void vsync(Game* game)
{
	static const int max_fps = 50;
	static const double frameDelay = 1./max_fps; // max fps na jedna seconde
	if(frameDelay > game->delta)
	{
		SDL_Delay((frameDelay - game->delta) * 1000); // sekundy na milisekundy
	}
};

//freeing all surfaces
void clear_all(Sdl* sdl, Game* game, Bullets* projectiles, Cars* cars, Map* map)
{
	projectiles->clear();
	cars->clear();
	map->clear();
	sdl->clear();

	SDL_Quit();
};

// main
#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv)
{
	Sdl sdl;
	if(!sdl.load_sdl()) return 1;

	srand(time(NULL));
	int t1, t2;
	SDL_Event event;
	Menu menu;
	Keys keys;
	Game game;
	Map map;
	Bullets projectiles;
	Cars cars;

	map.load_map_sprite(&game);
	cars.init_cars(&projectiles, &game);
	cars.kill_enemies();

	t1 = SDL_GetTicks();

	while (!game.quit) {
		t2 = SDL_GetTicks();

		// w tym momencie t2-t1 to czas w milisekundach,
		// jaki uplynal od ostatniego narysowania ekranu
		// delta to ten sam czas w sekundach
		game.setDelta((t2 - t1) * 0.001);
		t1 = t2;

		// delay frames greater than max_fps
		vsync(&game);

		handle_event(&event, &keys, &game, &map, &cars, &projectiles);

		if(!game.pause)
		{
			computeFrame(sdl.screen, &keys, &game, &map, &cars, &projectiles);
		}
		drawGame(&sdl, &game, &map, &cars, &projectiles, &menu);
	};

	clear_all(&sdl, &game, &projectiles, &cars, &map);
	return 0;
};