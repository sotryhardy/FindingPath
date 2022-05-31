#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include <Windows.h>
#include <string.h>
#include <time.h>
#include <SDL_ttf.h>

typedef unsigned char uchar;
typedef unsigned int uint;
typedef uchar mapType;			//if i will want to change the type for the map i will change it here

uint SCREEN_WIDTH;
uint SCREEN_HIGHT;

const int WIDTH = 15;
const int HEIGHT = 11;

enum WorldIndexes
{
	field = 1,
	//road...
	//swamp...
	//hm... i cant use float here... TODO
	obstacle = 255,
};

enum Commands
{
	Neutral,
	Red,
	Blue
};

struct Vec2i
{
	int X;
	int Y;
	bool operator == (Vec2i other)
	{
		return	other.X == X && other.Y == Y;
	}
	
};

struct Vec2f
{
	float X;
	float Y;
	bool operator == (Vec2f other)
	{
		return other.X == X && other.Y == Y;
	}
	explicit operator Vec2i()
	{
		return { (int)X, (int)Y };
	}
};

struct Vec4c
{
	uchar R;
	uchar G;
	uchar B;
	uchar A;
};

char* toArray(int number)
{
	int n = log10(number) + 1;
	if (number == 0)
	{
		n = 1;
	}
	int i;
	char* numberArray = (char*)calloc(n + 1, sizeof(char));
	numberArray[n] = '\0';
	for (i = n - 1; i >= 0; --i, number /= 10)
	{
		numberArray[i] = (number % 10) + '0';
	}

	return numberArray;
}

struct ListNode
{
	ListNode* another_node;
	Vec2i position;
};

struct Queue
{
	void AddNode(Vec2i nodePosition);
	bool IsEmpty();
	void DeleteFirstNode();
	void DeleteLastNode();
	void Clear();

	ListNode* FirstNode = nullptr;
};

struct Image
{
	void Init(const char* image_name, Vec2i texSize, SDL_Renderer* renderer);
	void Destroy();

	void Render(SDL_Renderer* renderer, Vec2f position);
	void AmountRender(SDL_Renderer* renderer, Vec2f position, int amount);
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
		return;
	}


	texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return;
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
	rect.x = position.X * size.X;
	rect.y = position.Y * size.Y;
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

struct Text
{
	Text(const char* fontName, Vec2i texSize, SDL_Renderer* renderer, SDL_Color colorFont);
	Text();
	~Text();
	void AmountRender(SDL_Renderer* renderer, Vec2f position, int amount);
private:
	SDL_Texture* texture;
	TTF_Font* font;
	SDL_Color color;
	Vec2i size;
	int past_amount = 0;
};

Text::Text() {}

Text::Text(const char* fontName, Vec2i texSize, SDL_Renderer* renderer, SDL_Color colorFont = { 0, 0, 0 })
{
	font = TTF_OpenFont(fontName, 24);
	size = texSize;
	color = colorFont;
	SDL_Surface* surface = SDL_CreateRGBSurface(0, size.X, size.Y, 32, 0, 0, 0, 0);
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
}

Text::~Text()
{
	if (texture)
		SDL_DestroyTexture(texture);
}

void Text::AmountRender(SDL_Renderer* renderer, Vec2f position, int amount)
{
	if (past_amount != amount)
	{
		past_amount = amount;
		char* amount_to_str = toArray(amount);
		
		if(texture)
			SDL_DestroyTexture(texture);

		SDL_Surface* s = TTF_RenderText_Solid(font, amount_to_str, color);
		if (!s)
		{
			printf("Failed to render text");
			return;
		}
		texture = SDL_CreateTextureFromSurface(renderer, s);
		SDL_FreeSurface(s);
		free(amount_to_str);
	}

	SDL_Rect rect;
	rect.x = position.X * size.X + size.X / 2;
	rect.y = position.Y * size.Y + size.Y / 2;
	rect.w = size.X / 2;
	rect.h = size.Y / 2;

	SDL_RenderCopyEx(renderer,
		texture,
		nullptr,
		&rect,
		0,
		nullptr,
		SDL_FLIP_NONE);


}

struct Character
{
	void Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer,float attackPower, float ch_health, float ch_speed);
	void Destroy();

	void Render(SDL_Renderer* renderer);
	void MoveInit(Queue _path);
	void Move(float Deltatime);
	bool IsMove();

public:
	Image texture;
	Vec2f position;
	float attack_power;
	float health;
	float speed;

private:
	Queue path;
	bool do_get_position;
	Vec2i goal, direction;
};

#pragma region CharacterFunctions

void Character::Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer, float attackPower, float ch_health, float ch_speed)
{
	texture.Init(fileName, texSize, renderer);
	position = startPosition;
	attack_power = attackPower;
	speed = ch_speed;
	health = ch_health;
	do_get_position = true;
}

void Character::Destroy()
{
	texture.Destroy();
}

void Character::Render(SDL_Renderer* renderer)
{
	texture.Render(renderer, position);
}

void Character::MoveInit(Queue _path)
{
	path.Clear();
	path = _path;
}

void Character::Move(float Deltatime)
{
	if (!path.IsEmpty())
	{
		if (do_get_position)
		{
			goal = path.FirstNode->position;
			direction = { goal.X - (int)position.X, goal.Y - (int)position.Y };
			do_get_position = false;
		}
		position.X += direction.X * speed * Deltatime;
		position.Y += direction.Y * speed * Deltatime;
		if (fabs(position.X - goal.X) < speed * Deltatime && fabs(position.Y - goal.Y) < speed * Deltatime)
		{
			position = { (float)goal.X, (float)goal.Y };
			do_get_position = true;
			path.DeleteFirstNode();
		}
	}
}

bool Character::IsMove()
{
	return !path.IsEmpty();
}

Queue FindPath(Vec2i start_position, Vec2i end_position, mapType** battlefield, Character* characters = nullptr, int amount = 0, bool attack = false);

#pragma endregion 

struct Team;

struct Squad
{
	Squad();
	Squad(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer, float ch_attackPower, float ch_health, float ch_speed, int squad_amount);
	~Squad();

	void Render(SDL_Renderer* renderer);
	void MoveInit(Queue init_path);
	void Move(float Deltatime);
	void AttackSquadInit(Squad* squadToAttack, mapType** battlefield, Character* characters, int amount_all, Team* teamToAttack, Team* thisTeam);
	void Attack();
	void CounterAttack(Squad* squad_to_attack);
	void MakeDamage(Squad* squad_to_attack);
	bool IsMove();
	bool IsEmpty();

public:
	Character character;
	float general_attack_power;
	float general_health;
	int amount;
private:
	Squad* squad_to_attack = nullptr;
	Team* team_to_attack;
	Text text;
};

Squad::Squad() {};

Squad::Squad(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer, float ch_attackPower, float ch_health, float ch_speed, int squadAmount)
	: text("Oswald-Regular.ttf", texSize, renderer)
{	
	character.Init(fileName, texSize, startPosition, renderer,ch_attackPower, ch_health, ch_speed);
	amount = squadAmount;
	general_attack_power = amount * ch_attackPower;
	general_health = ch_health * amount;
}

Squad::~Squad() 
{
	text.~Text();
}

void Squad::Render(SDL_Renderer* renderer)
{
	character.Render(renderer);
	text.AmountRender(renderer, character.position, amount);
}

void Squad::MoveInit(Queue init_path)
{
	character.MoveInit(init_path);
}

void Squad::Move(float Deltatime)
{
	character.Move(Deltatime);
	if (squad_to_attack && !IsMove())
	{
		Attack();
		squad_to_attack = nullptr;
	}
}

void Squad::AttackSquadInit(Squad* squadToAttack, mapType** battlefield, Character* characters, int amount_all, Team* teamToAttack, Team* thisTeam)
{
	team_to_attack = teamToAttack;
	Vec2i squad_to_attack_pos = (Vec2i)squadToAttack->character.position;

	Queue Path = FindPath((Vec2i)character.position, (Vec2i)squad_to_attack_pos, battlefield, characters, amount_all, true);
	Path.DeleteLastNode();
	MoveInit(Path);
	if (IsMove())
	{
		squad_to_attack = squadToAttack;
		squadToAttack->team_to_attack = thisTeam;

	}

}

bool Squad::IsEmpty()
{
	return !amount;
}



bool Squad::IsMove()
{
	return character.IsMove();
}


const int max_squads = 15;

struct Team
{
	Team(Commands _side);
	void AddSquad(Squad* squad_to_add);
	void MakeAction(Vec2i goal, Team* team_to_attack, uchar** battlefield);
	Squad* GetSquadByIndex(int index);
	Squad* GetCurrentTourSquad();
	int GetRealIndex(int index);
	void MakeActionAI(Team* team_to_attack, mapType** battlefield);
	Character* GatherEveryCharacters(Team* EnemyTeam, int* size);
	void DeleteKilledSquad();


public:
	Squad* squads[max_squads];
	Commands side;
	int amount = 0;
	int squad_index = 0;
private:

};

Team::Team(Commands _side)
{
	side = _side;
	for (int i = 0; i < max_squads; i++)
	{
		squads[i] = nullptr;
	}
}

void Team::AddSquad(Squad* squad_to_add)
{
	amount++;
	int i;
	for (i = 0; squads[i]; i++);
	squads[i] = squad_to_add;
}

Squad* Team::GetSquadByIndex(int index)
{
	int real_index = - 1;
	for (int  i = 0; real_index < amount; i++)
	{
		if (squads[i]->amount)
		{
			real_index++;
		}
		if (real_index == index)
		{
			return squads[i];
		}
	}
}



int Team::GetRealIndex(int index)
{
	int array_index = -1;
	for (int i = 0; array_index < amount; i++)
	{
		if (squads[i]->amount)
		{
			array_index++;
		}
		if (array_index == index)
		{
			return i;
		}
	}
}

Squad* Team::GetCurrentTourSquad()
{
	return squads[GetRealIndex(squad_index)];
}

void Squad::Attack()
{

	MakeDamage(squad_to_attack);
	if (!squad_to_attack->IsEmpty())
	{
		squad_to_attack->CounterAttack(this);
	}
}

void Squad::CounterAttack(Squad* squadToAttack)
{

	MakeDamage(squadToAttack);

}

void Squad::MakeDamage(Squad* squad_to_attack)
{
	squad_to_attack->general_health -= general_attack_power;
	squad_to_attack->amount = ceil((float)squad_to_attack->general_health / squad_to_attack->character.health);
	squad_to_attack->general_attack_power = squad_to_attack->amount * squad_to_attack->character.attack_power;

	if (squad_to_attack->amount <= 0)
	{
		squad_to_attack->amount = 0;
		team_to_attack->DeleteKilledSquad();
	}
}

void Team::MakeAction(Vec2i goal, Team* team_to_attack, mapType** battlefield)
{
	if (!amount)
		return;
	if (squad_index >= amount || squad_index < 0)
		squad_index = 0;

	Squad* current_tour_squad = GetCurrentTourSquad();
	bool attack = false;

	int size;
	Character* EveryCharacters = GatherEveryCharacters(team_to_attack, &size);

	for (int i = 0; i < team_to_attack->amount; i++)
	{
		Squad* squad_for_check = team_to_attack->GetSquadByIndex(i);
		if (goal == (Vec2i)squad_for_check->character.position)
		{
			current_tour_squad->AttackSquadInit(squad_for_check, battlefield, EveryCharacters, size, team_to_attack, this);
			attack = true;
			break;
		}
	}
	
	if(!attack)
		current_tour_squad->MoveInit(FindPath((Vec2i)current_tour_squad->character.position, goal, battlefield, EveryCharacters, size));

	squad_index++;

	delete[] EveryCharacters;
}

void Team::MakeActionAI(Team* teamToAttack, mapType** battlefield)
{
	if (squad_index >= amount || squad_index < 0)
		squad_index = 0;
	Squad* current_tour_squad = GetCurrentTourSquad();

	int index_squad_to_attack = rand() % teamToAttack->amount;
	int size;
	
	Squad* squad_to_attack = teamToAttack->GetSquadByIndex(index_squad_to_attack);

	Character* EveryCharacters = GatherEveryCharacters(teamToAttack,&size);

	current_tour_squad->AttackSquadInit(squad_to_attack, battlefield, EveryCharacters, size, teamToAttack, this);

	if (!current_tour_squad->IsMove())
	{
		Vec2i goal = { rand() % WIDTH, rand() % HEIGHT };
		current_tour_squad->MoveInit(FindPath((Vec2i)current_tour_squad->character.position, goal, battlefield, EveryCharacters, size));
	}
	delete[] EveryCharacters;
	squad_index++;
}

Character* Team::GatherEveryCharacters(Team* EnemyTeam, int* size)
{
	*size = amount + EnemyTeam->amount;
	Character* EveryCharacters = new Character[*size];

	for (size_t i = 0; i < amount; i++)
	{
		EveryCharacters[i] = GetSquadByIndex(i)->character;
	}

	for (int i = amount; i < *size; i++)
	{
		EveryCharacters[i] = EnemyTeam->GetSquadByIndex(i - amount)->character;
	}

	return EveryCharacters;
}

void Team::DeleteKilledSquad()
{
	bool edit_index = true;
	for (int i = 0; i < amount; i++)
	{
		if (!squads[i]->amount)
		{
			if (edit_index)
			{
				edit_index = false;
				if (i < squad_index)
					squad_index--;
			}
			Squad* buffer = squads[i];
			squads[i] = squads[i + 1];
			squads[i + 1] = buffer;
		}
	}
	amount--;
}

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

void Queue::DeleteLastNode()
{
	ListNode* node_to_check = FirstNode;

	if (!FirstNode)
		return;
	if (!node_to_check->another_node)
	{
		free(node_to_check);
		FirstNode = nullptr;
	}
	while (node_to_check->another_node->another_node)
	{
		node_to_check = node_to_check->another_node;
	}

	free(node_to_check->another_node);
	node_to_check->another_node = nullptr;
}

bool Queue::IsEmpty()
{
	return !FirstNode;
}


int SDL_StartInit(SDL_Renderer** renderer, SDL_Window** window)
{
	SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
	SCREEN_HIGHT = GetSystemMetrics(SM_CYSCREEN);
	srand(time(NULL));
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

	result = TTF_Init();

	if (result)
	{
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

void SumString(char* string, const char* value)
{
	strcpy(string + strlen(string), value);
}

void DrawMap(mapType** grid)
{
	char map[120 * 90] = "";
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			char buf[10] = "";
			sprintf(buf, "%i\t", grid[i][j]);
			SumString(map, buf);
		}
		SumString(map, "\n\n\n");
	}
	SumString(map, "\n\n\n");
	printf("%s", map);
}

Queue FindPath(Vec2i start_position, Vec2i end_position, mapType** battlefield, Character* characters, int amount, bool attack)
{
	//https://nrsyed.com/2017/12/30/animating-the-grassfire-path-planning-algorithm/
	// Algorytn is based on finnding path starting from the end to start 
	Queue search_queue, path;
	search_queue.AddNode(end_position);

	if (!(end_position.X >= 0 && end_position.Y >= 0 && end_position.X < WIDTH && end_position.Y < HEIGHT))
		return path;

	if (start_position.X == end_position.X && start_position.Y == end_position.Y)
	{
		path.AddNode(start_position);
		return path;
	}
	
	mapType** grid = (mapType**)malloc(sizeof(mapType*) * HEIGHT);	// copy original battlefield 
	for (int i = 0; i < HEIGHT; i++)
	{
		grid[i] = (mapType*)malloc(sizeof(uchar) * WIDTH);
		for (int j = 0; j < WIDTH; j++)
		{
			grid[i][j] = battlefield[i][j];
		}
	}

	for (int i = 0; i < amount; i++)
	{
		Vec2f pos = characters[i].position;
		grid[(int)pos.Y][(int)pos.X] = obstacle;
	}

	if (attack)
		grid[end_position.Y][end_position.X] = field;

	if (grid[end_position.Y][end_position.X] == obstacle)
	{
		return path;
	}

	grid[end_position.Y][end_position.X] = field + 1;			//End point designation

	while (!search_queue.IsEmpty() && path.IsEmpty())
	{
		const Vec2i current_pos = search_queue.FirstNode->position;
		search_queue.DeleteFirstNode();

		uchar next_value = grid[current_pos.Y][current_pos.X] + 1;

		for (int i = -1; i <= 1; i++)		//cheking neighbour points of current point 
			for (int j = -1; j <= 1; j++)
				if (i * j == 0 && i != j)
				{
					Vec2i neighbour = { current_pos.X + i,  current_pos.Y + j };
					if (neighbour.X == start_position.X && neighbour.Y == start_position.Y)
					{
						path.AddNode(neighbour);
						grid[neighbour.Y][neighbour.X] = next_value;
					}
					else if (neighbour.X >= 0 && neighbour.Y >= 0 && neighbour.X < WIDTH && neighbour.Y < HEIGHT)
					{
						if (grid[neighbour.Y][neighbour.X] != obstacle && grid[neighbour.Y][neighbour.X] == battlefield[neighbour.Y][neighbour.X])	// if neighbour is not obstacle and was not edited by previouses iterations 
						{
							search_queue.AddNode(neighbour); // add point to check-list 
							grid[neighbour.Y][neighbour.X] = next_value;
						}
					}
				}
	}

	search_queue.Clear();
	DrawMap(grid);			//TODO: on assection-based for debug configuration
	if (path.FirstNode)
	{
		Vec2i current_pos = path.FirstNode->position;
		int next_value = grid[current_pos.Y][current_pos.X] - 1;
		while (next_value > grid[end_position.Y][end_position.X])
		{
			for (int i = -1; i <= 1; i++)
				for (int j = -1; j <= 1; j++)
					if (i * j == 0 && i != j)
					{
						Vec2i neighbour{ current_pos.X + i, current_pos.Y + j };
						if (neighbour.X >= 0 && neighbour.Y >= 0 && neighbour.X < WIDTH && neighbour.Y < HEIGHT)
						{
							if (grid[neighbour.Y][neighbour.X] == next_value && next_value != grid[end_position.Y][end_position.X])
							{
								path.AddNode(neighbour);
								current_pos = neighbour;
								next_value--;

							}
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


void Squad_Init(Squad* arr,int amount, Vec2i texSize, SDL_Renderer* renderer, float attackPower, float health, float ch_speed)
{
	int above_indent = 1;
	int size_indent = 1;
	for (int i = 0; i < amount; i++)
	{
		if (!(i % 2))
			arr[i] = Squad("image_red.png", texSize, { (float)size_indent, (float)above_indent + (i / 2) }, renderer, attackPower, health, ch_speed, 10);
		else
			arr[i] = Squad("image_blue.png", texSize, { (float)WIDTH - 1 - size_indent, (float)above_indent + (i / 2) }, renderer, attackPower, health, ch_speed, 10);
	}
}

void ObstacleGenerate(mapType** battlefield, int min_amount, int max_amount)
{
	int amount = rand() % (max_amount - min_amount) + min_amount;
	for (int i = 0; i < amount; i++)
	{
		int x = rand() % WIDTH;
		int y = rand() % HEIGHT;
		battlefield[y][x] = obstacle;
	}
}

bool SquadsAreMove(Squad* squads, int amount)
{
	for (int i = 0; i < amount; i++)
	{
		if (squads[i].IsMove())
			return true;
	}
	return false;
}

int main(int argc, char* argv[])
{
	SDL_Renderer* renderer;						//creating pointers in this locality zone
	SDL_Window* window;
	bool LBM = false;
	bool PlayerPart = true;
	bool EnemyPart = false;

	if (SDL_StartInit(&renderer, &window))		//Initialization of SDL
		return -1;								// If you need to include more you can do it in this function

	SDL_SetRenderDrawColor(renderer, 125, 125, 125, 255);		//fill empty with color


	int SquadsAmount = 16;
	int PlayerAmount = SquadsAmount / 2;
	int EnemyAmount = SquadsAmount / 2;

	Team RedTeam(Red);
	Team BlueTeam(Blue);

	Squad* Squads = new Squad[SquadsAmount];

	float character_speed = 10.0f;

	Vec2i CellSize{ SCREEN_WIDTH / WIDTH , SCREEN_HIGHT / HEIGHT }; //size of 1 part of screen in pixels

	Squad_Init(Squads, SquadsAmount, CellSize, renderer, 300, 200, character_speed);

	for (int i = 0; i < SquadsAmount; i++)
	{
		if (!(i % 2))
			RedTeam.AddSquad(Squads + i);
		else
			BlueTeam.AddSquad(Squads + i);
	}


	Image BlackRect;
	BlackRect.CreateRGBRect(renderer, { 0, 0, 0, 0 }, CellSize);	//I will use it for obstacle

	mapType** battlefield = (mapType**)malloc(sizeof(mapType*) * HEIGHT);
	for (int i = 0; i < HEIGHT; i++)
	{
		battlefield[i] = (uchar*)malloc(sizeof(uchar) * WIDTH);					//initialization of battlefield
		for (int j = 0; j < WIDTH; j++)
		{
			battlefield[i][j] = field;
		}
	}

	ObstacleGenerate(battlefield, 10, 20);

	bool done = false;
	SDL_Event sdl_event;
	float Deltatime = 0.016;
	bool CharacterIsMove = false;
	int PartIndex = 0;
	int Pindex = 0;
	int Eindex = 0;
	//Squad* CharacterToMove = PlayerCommand[0];
	while (!done)				//start main cycle  for game
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
					LBM = true;
				}
				default:
					break;
				}
			}
		}

		SDL_RenderClear(renderer);



		if (LBM && PlayerPart)
		{
			LBM = false;
			PlayerPart = false;
			CharacterIsMove = true;
			Vec2i screen_mouse_pos, world_mouse_pos;
			SDL_GetMouseState(&screen_mouse_pos.X, &screen_mouse_pos.Y);
			world_mouse_pos = { screen_mouse_pos.X / CellSize.X, screen_mouse_pos.Y / CellSize.Y };
			int last = SDL_GetTicks();
			RedTeam.MakeAction(world_mouse_pos, &BlueTeam, battlefield);	//get the path from function
			printf("\n%i", SDL_GetTicks() - last);
		}

		if (EnemyPart)
		{
			EnemyPart = false;
			CharacterIsMove = true;
			Vec2i goal = { rand() % WIDTH, rand() % HEIGHT };

			BlueTeam.MakeActionAI(&RedTeam, battlefield);

		}

		if (CharacterIsMove && !SquadsAreMove(Squads, SquadsAmount))
		{
			CharacterIsMove = false;
			PartIndex++;
			if (PartIndex % 2)
			{
				EnemyPart = true;
			}
			else
			{
				PlayerPart = true;
			}
		}

		if (!RedTeam.amount || !BlueTeam.amount)
		{
			done = true;
		}

		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				if (battlefield[i][j] == 255)
				{																			// } rendering obstacles 
					BlackRect.Render(renderer, { (float)j, (float)i });
				}
			}
		}

		for (int i = 0; i < SquadsAmount; i++)
		{
			if (Squads[i].amount)
			{
				Squads[i].Move(Deltatime);
				Squads[i].Render(renderer);
			}
		}
		SDL_RenderPresent(renderer);
		
		int TotalTime = SDL_GetTicks();
		int SleepTime = Deltatime * 1000 - (TotalTime - lasttime);
		if (SleepTime > 0)
		{
			Sleep(SleepTime);
		}

	}

	for (int i = 0; i < HEIGHT; i++)
	{
		free(battlefield[i]);
	}

	delete[] Squads;

	free(battlefield);
	TTF_Quit();
	BlackRect.Destroy();
	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}
