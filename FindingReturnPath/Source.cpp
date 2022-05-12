#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include <Windows.h>

typedef unsigned char uchar;
typedef unsigned int uint;

uint SCREEN_WIDTH;
uint SCREEN_HIGHT;

const int WIDTH = 15;
const int HEIGHT = 11;


struct Vec2i
{
	int X;
	int Y;
};

struct Vec2f
{
	float X;
	float Y;
};

struct Vec4c
{
	uchar R;
	uchar G;
	uchar B;
	uchar A;
};

struct Image
{
	void Init(const char* image_name, Vec2i texSize, SDL_Renderer* renderer);
	void Destroy();

	void Render(SDL_Renderer* renderer, Vec2f position);
	void CreateRGBRect(SDL_Renderer* renderer, Vec4c colors, Vec2i tex_size);

	SDL_Texture* texture;
	Vec2i size;
};

void Image::Init(const char* image_name, Vec2i texSize, SDL_Renderer* renderer)
{
	SDL_Surface* surface = IMG_Load(image_name);
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", image_name, IMG_GetError());
		return ;
	}


	texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return ;
	}

	size = texSize;
	
	SDL_FreeSurface(surface);
}

void Image::Destroy()
{
	SDL_DestroyTexture(texture);
}

void Image::Render(SDL_Renderer* renderer, Vec2f position)
{
	SDL_Rect rect;
	rect.x = (int)position.X * size.X;
	rect.y = (int)position.Y * size.Y;
	rect.w = size.X;
	rect.h = size.Y;

	SDL_RenderCopyEx(renderer,
		texture,
		nullptr,
		&rect,
		0,
		nullptr,
		SDL_FLIP_NONE);
}

void Image::CreateRGBRect(SDL_Renderer* renderer, Vec4c colors, Vec2i tex_size)
{
	SDL_Surface* surface;
	size = tex_size;
	SDL_Rect rect = { 0, 0, tex_size.X, tex_size.Y };
	surface = SDL_CreateRGBSurface(0, tex_size.X, tex_size.Y, 32, 0, 0, 0, 0);
	SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, colors.R, colors.G, colors.B));
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
}



struct Character
{
	void Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer);
	void Destroy();

	void Render(SDL_Renderer* renderer);

	Image texture;
	Vec2f position;
};

void Character::Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer)
{
	texture.Init(fileName, texSize, renderer);
	position = startPosition;
}

void Character::Render(SDL_Renderer* renderer)
{
	texture.Render(renderer, position);
}

void Character::Destroy()
{
	texture.Destroy();
}

struct ListNode
{
	ListNode* another_node;
	Vec2i position;
};

struct Stack
{
	ListNode* LastNode = nullptr;
	void AddElement(Vec2i nodePosition);
	void DeleteLastNode();
	void Clear();
};

void Stack::AddElement(Vec2i nodePosition)
{
	ListNode* NewNode = (ListNode*)malloc(sizeof(ListNode));
	NewNode->another_node = LastNode;
	NewNode->position = nodePosition;
	LastNode = NewNode;
}

void Stack::DeleteLastNode()
{
	if (LastNode)
	{
		ListNode* another_node = LastNode->another_node;
		free(LastNode);
		LastNode = another_node;
	}
}

void Stack::Clear()
{
	while (LastNode)
	{
		DeleteLastNode();
	}
}

struct Queue
{
	void AddNode(Vec2i nodePosition);
	bool IsEmpty();
	void DeleteFirstNode();
	void Clear();

	ListNode* FirstNode = nullptr;
};

void Queue::AddNode(Vec2i nodePosition)
{
	ListNode** last_node = &FirstNode;
	while (*last_node)
	{
		last_node = &(*last_node)->another_node;
	}
	*last_node = (ListNode*)malloc(sizeof(ListNode));
	(*last_node)->another_node = nullptr;
	(*last_node)->position = nodePosition;
}
void Queue::DeleteFirstNode()
{
	ListNode* next_node = FirstNode->another_node;
	free(FirstNode);
	FirstNode = next_node;

}
void Queue::Clear()
{
	while (FirstNode)
	{
		DeleteFirstNode();
	}
}


bool Queue::IsEmpty()
{
	return !FirstNode;
}

int SDL_StartInit(SDL_Renderer** renderer, SDL_Window** window)
{
	SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
	SCREEN_HIGHT = GetSystemMetrics(SM_CYSCREEN);
	SDL_SetMainReady();
	int result = 0;
	result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if (result)
	{
		printf("Can't initialize SDL. Error: %s", SDL_GetError());
		return -1;
	}

	result = IMG_Init(IMG_INIT_PNG);
	if (!(result & IMG_INIT_PNG))
	{
		printf("Can't initialize SDL image. Error: %s", SDL_GetError());
		return -1;
	}

	*window = SDL_CreateWindow("FirstSDL",
		0, 0,
		SCREEN_WIDTH, SCREEN_HIGHT,
		SDL_WINDOW_SHOWN);

	if (!*window)
		return -1;


	*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
	if (!*renderer)
		return -1;

	return 0;
}

void DrawMap(uchar** grid)
{
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			printf("%i \t", grid[i][j]);
		}
		printf("\n\n\n");
	}
	printf("\n\n\n");
}

Queue FindPath(Vec2i start_position, Vec2i end_position, uchar** battlefield)
{
	Queue search_queue ,path;
	search_queue.AddNode(end_position);
	if (start_position.X == end_position.X && start_position.Y == end_position.Y || battlefield[end_position.Y][end_position.X] == 255)
	{
		return path;
	}
	uchar** grid = (uchar**)malloc(sizeof(uchar*) * HEIGHT);
	for (int i = 0; i < HEIGHT; i++)
	{
		grid[i] = (uchar*)malloc(sizeof(uchar) * WIDTH);
		for (int j = 0; j < WIDTH; j++)
		{
			grid[i][j] = battlefield[i][j];
		}
	}
	while (!search_queue.IsEmpty() && path.IsEmpty())
	{
		const Vec2i current_pos = search_queue.FirstNode->position;
		int index = grid[current_pos.Y][current_pos.X] + 1;
		search_queue.DeleteFirstNode();
		for (int i = -1; i <= 1; i++)
			for (int j = -1; j <= 1; j++)
				if (i * j == 0 && i != j)
				{
					Vec2i neighbour = { current_pos.X + i,  current_pos.Y + j};
					if (neighbour.X == start_position.X && neighbour.Y == start_position.Y)
					{
						path.AddNode(neighbour);
						grid[neighbour.Y][neighbour.X] = index;
					}
					else if (neighbour.X >= 0 && neighbour.Y >= 0 && neighbour.X < WIDTH && neighbour.Y < HEIGHT && (neighbour.X != end_position.X || neighbour.Y != end_position.Y))
					{
						if (grid[neighbour.Y][neighbour.X] > 0 && grid[neighbour.Y][neighbour.X] < 255 && grid[neighbour.Y][neighbour.X] == battlefield[neighbour.Y][neighbour.X])
						{
							search_queue.AddNode(neighbour);
							grid[neighbour.Y][neighbour.X] = index;
						}
					}
				}
	}

	search_queue.Clear();
	DrawMap(grid);
	if (path.FirstNode)
	{
		Vec2i current_pos = path.FirstNode->position;
		int index = grid[current_pos.Y][current_pos.X] - 1;
		while (index > 1)
		{
			for (int i = -1; i <= 1; i++)
				for (int j = -1; j <= 1; j++)
					if (i * j == 0 && i != j)
					{
						Vec2i neighbour{ current_pos.X + i, current_pos.Y + j};
						if (grid[neighbour.Y][neighbour.X] == index)
						{
							path.AddNode(neighbour);
							current_pos = neighbour;
							index--;
						}
					}
		}
		path.AddNode(end_position);
	}
	for (int i = 0; i < HEIGHT; i++)
	{
		free(grid[i]);
	}
	free(grid);
	return path;
}

int main(int argc, char* argv[])
{
	SDL_Renderer* renderer;
	SDL_Window* window;

	if (SDL_StartInit(&renderer, &window))
		return -1;

	SDL_SetRenderDrawColor(renderer, 20, 13, 39, 255);


	Image BlackRect;
	Character Ship;

	int WidthPixels = SCREEN_WIDTH / WIDTH;
	int HeightPixels = SCREEN_HIGHT / HEIGHT;

	Ship.Init("image.png", { WidthPixels, HeightPixels }, { 7.0f, 10.0f }, renderer);
	BlackRect.CreateRGBRect(renderer, { 0, 0, 0, 0 }, { WidthPixels, HeightPixels });

	uchar** battlefield = (uchar**)malloc(sizeof(uchar*) * HEIGHT);
	for (int i = 0; i < HEIGHT; i++)
	{
		battlefield[i] = (uchar*)malloc(sizeof(uchar) * WIDTH);
		for (int j = 0; j < WIDTH; j++)
		{
			battlefield[i][j] = 1;
		}
	}

	battlefield[2][7] = 255;
	battlefield[3][6] = 255;
	battlefield[4][7] = 255;
	battlefield[3][8] = 255;
	for (int i = 1; i < WIDTH - 1; i++)
	{
		battlefield[HEIGHT / 2][i] = 255;
	}

	Queue Path;
	bool done = false;
	SDL_Event sdl_event;
	while (!done)
	{
		int lasttime = SDL_GetTicks();
		while (SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_QUIT)
			{
				done = true;
			}
			else if (sdl_event.type == SDL_KEYDOWN)
			{
				switch (sdl_event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					SDL_Event event;
					event.type = SDL_QUIT;
					event.quit.type = SDL_QUIT;
					event.quit.timestamp = SDL_GetTicks();
					SDL_PushEvent(&event);
					break;
				default:
					break;
				}
			}
			else if (sdl_event.type == SDL_MOUSEBUTTONDOWN)
			{
				switch (sdl_event.button.button)
				{
				case SDL_BUTTON_LEFT:
				{
					Vec2i screen_mouse_pos, world_mouse_pos;
					SDL_GetMouseState(&screen_mouse_pos.X, &screen_mouse_pos.Y);
					world_mouse_pos = { screen_mouse_pos.X / WidthPixels, screen_mouse_pos.Y / HeightPixels };
					Path.Clear();
					int last = SDL_GetTicks();
					Path = FindPath({ (int)Ship.position.X, (int)Ship.position.Y }, world_mouse_pos, battlefield);
					printf("\n%i", SDL_GetTicks() - last);
					break;
				}
				default:
					break;
				}
			}
		}

		// Clearing the screen
		SDL_RenderClear(renderer);

		int StartTimer = SDL_GetTicks();
		//while (finding)
		//{
		//	uchar x = CheckNow->LastNode->X;
		//	uchar y = CheckNow->LastNode->Y;
		//	if (x - 1 >= 0 && (battlefield[y][x - 1] == 1 || battlefield[y][x - 1] == 255))		// LEFT POINT
		//	{
		//		if (battlefield[y][x - 1] != 255)
		//		{
		//			CheckNext->AddElement(x - 1, y);
		//		}
		//		else
		//		{
		//			Path.AddElement(x - 1, y);
		//			PathGenerate = true;
		//		}
		//		battlefield[y][x - 1] = battlefield[y][x] + 1;
		//	}

		//	if (y - 1 >= 0 && (battlefield[y - 1][x] == 1 || battlefield[y - 1][x] == 255))		//UP POINT
		//	{
		//		if (battlefield[y - 1][x] != 255)
		//		{
		//			CheckNext->AddElement(x, y - 1);
		//		}
		//		else
		//		{
		//			Path.AddElement(x, y - 1);
		//			PathGenerate = true;
		//		}
		//		battlefield[y - 1][x] = battlefield[y][x] + 1;
		//	}

		//	if (x + 1 < WIDTH && (battlefield[y][x + 1] == 1 || battlefield[y][x + 1] == 255))			//RIGTH POINT
		//	{

		//		if (battlefield[y][x + 1] != 255)
		//		{
		//			CheckNext->AddElement(x + 1, y);
		//		}
		//		else
		//		{
		//			Path.AddElement(x + 1, y);
		//			PathGenerate = true;
		//		}
		//		battlefield[y][x + 1] = battlefield[y][x] + 1;
		//	}

		//	if (y + 1 < HEIGHT && (battlefield[y + 1][x] == 1 || battlefield[y + 1][x] == 255))		//DOWN POINT
		//	{
		//		if (battlefield[y + 1][x] != 255)
		//		{
		//			CheckNext->AddElement(x, y + 1);
		//		}
		//		else
		//		{
		//			Path.AddElement(x, y + 1);
		//			PathGenerate = true;
		//		}
		//		battlefield[y + 1][x] = battlefield[y][x] + 1;
		//	}
		//	CheckNow->DeleteLastNode();
		//	if (!CheckNow->LastNode)
		//	{
		//		if (CheckNext->LastNode)
		//		{
		//			Stack* temp = CheckNow;
		//			CheckNow = CheckNext;1
		//		{
		//			finding = false;
		//			break;
		//		}
		//	}


		//	while (PathGenerate)
		//	{

		//		uchar x = Path.LastNode->X;
		//		uchar y = Path.LastNode->Y;
		//		if (battlefield[y][x] <= 3)
		//		{
		//			finding = false;
		//			PathGenerate = false;
		//			break;
		//		}
		//		if (x - 1 >= 0 && battlefield[y][x - 1] == battlefield[y][x] - 1)
		//		{
		//			Path.AddElement(x - 1, y);
		//		}
		//		else if (x + 1 < WIDTH && battlefield[y][x + 1] == battlefield[y][x] - 1)
		//		{
		//			Path.AddElement(x + 1, y);
		//		}
		//		else if (y - 1 >= 0 && battlefield[y - 1][x] == battlefield[y][x] - 1)
		//		{
		//			Path.AddElement(x, y - 1);
		//		}
		//		else if (y + 1 < HEIGHT && battlefield[y + 1][x] == battlefield[y][x] - 1)
		//		{
		//			Path.AddElement(x, y + 1);
		//		}

		//	}
		//}
		//if (GetTimeOfAlgorytm)
		//{
		//	printf("time: %i\n", (SDL_GetTicks() - StartTimer));
		//	GetTimeOfAlgorytm = false;
		//}

		if (Path.FirstNode)
		{
			Ship.position = { (float)Path.FirstNode->position.X, (float)Path.FirstNode->position.Y };
			Path.DeleteFirstNode();
			Sleep(100);
		}

		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				if (battlefield[i][j] == 255)
				{
					BlackRect.Render(renderer, { (float)j, (float)i });
				}
			}
		}

		Ship.Render(renderer);

		SDL_RenderPresent(renderer);

	}

	for (int i = 0; i < HEIGHT; i++)
	{
		free(battlefield[i]);
	}

	free(battlefield);

	BlackRect.Destroy();
	Ship.Destroy();

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	// Done.
	return 0;
}
