#ifndef _HW3Q1_H_
#define _HW3Q1_H_

//Includes
#include <linux/sched.h>
//#include <string.h>

/*=========================================================================
Constants and definitions:
==========================================================================*/
#define N (4) /* the size of the board */
#define M (3)  /* the initial size of the snake */
#define K (5)  /* the number of turns a snake can survive without eating */

typedef char Player;
/* PAY ATTENTION! i will use the fact that white is positive one and black is negative
one to describe the segments of the snake. for example, if the white snake is 2 segments
long and the black snake is 3 segments long
white snake is  1   2
black snake is -1  -2  -3 */
#define WHITE ( 1) /* id to the white player */
#define BLACK (-1) /* id to the black player */
#define WHITEWINNER ( 2) /* winner return value for the white player */
#define BLACKWINNER ( 4) /* winner return value for the black player */
#define EMPTY ( 0) /* to describe an empty point */
#define KEEP_PLAYING ( 0) /* return value for Game_Update*/
#define ITS_A_TIE ( -1) /* return value for Game_Update*/


/* to describe a point with food. having the value this big guarantees that there will be no
overlapping between the snake segments' numbers and the food id */
#define FOOD  (N*N)

typedef char bool;
#define FALSE (0)
#define TRUE  (1)

typedef int Direction;
#define DOWN  (2)
#define LEFT  (4)
#define RIGHT (6)
#define UP    (8)

/* a point in 2d space */
typedef struct
{
	int x, y;
} Point;


//typedef int Matrix[N][N];

typedef int ErrorCode;
#define ERR_OK      			((ErrorCode) 0)
#define ERR_BOARD_FULL			((ErrorCode)-1)
#define ERR_SNAKE_IS_TOO_HUNGRY ((ErrorCode)-2)
typedef int Matrix[N][N];

bool Game_Init(Matrix* matrix); /* initialize the board. return false if the board is illegal (should not occur, affected by N, M parameters) */
int Game_Update(Matrix *matrix, Player player,int move, int* white_counter, int* black_counter);/* handle all updating to this player. returns whether to continue or not. */
void Game_Print(Matrix* matrix,char* buffer,int* board_size);/* prints the state of the board */
Point GetInputLoc(Matrix*, Player,int);/* calculates the location that the player wants to go to */
bool CheckTarget(Matrix*, Player, Point);/* checks if the player can move to the specified location */
Point GetSegment(Matrix*, int);/* gets the location of a segment which is numbered by the value */
bool IsAvailable(Matrix*, Point);/* returns if the point wanted is in bounds and not occupied by any snake */
ErrorCode CheckFoodAndMove(Matrix*, Player, Point, int*, int*);/* handle food and advance the snake accordingly */
ErrorCode RandFoodLocation(Matrix*);/* randomize a location for food. return ERR_BOARD_FULL if the board is full */
void AdvancePlayer(Matrix*, Player, Point);/* advance the snake */
void IncSizePlayer(Matrix*, Player, Point);/* advance the snake and increase it's size */
bool IsMatrixFull(Matrix*);/* check if the matrix is full */
int GetSize(Matrix*, Player);/* gets the size of the snake */

/*******************************************************************************
		-This is the contents of the hw3q1.c file, that didn't work
		 with the linker of the linux...
*******************************************************************************/

//This function returns the winner
static int Loser_To_Winner(Player player){
	if (player == WHITE){
		return BLACKWINNER;
	}
	return WHITEWINNER;
}

static int rand(){
	return jiffies % N;
}

bool Game_Init(Matrix* matrix)
{
	int i,j;

	for (i=0;i<N;i++){
		for (j=0;j<N;j++){				//Initializing the matrix to zeros
			(*matrix)[i][j] = 0;
		}
	}
	/* initialize the snakes location */
	for (i = 0; i < M; ++i)
	{
		(*matrix)[0][i] =   WHITE * (i + 1);
		(*matrix)[N - 1][i] = BLACK * (i + 1);
	}

	/* initialize the food location */
	if (RandFoodLocation(matrix) != ERR_OK)
		return FALSE;

	return TRUE;
}

int Game_Update(Matrix *matrix, Player player,int move, int* white_counter, int* black_counter)
{
	ErrorCode e;
	Point p = GetInputLoc(matrix, player,move);

	if (!CheckTarget(matrix, player, p))
	{
		return Loser_To_Winner(player);
	}
	e = CheckFoodAndMove(matrix, player, p, white_counter, black_counter);
	if (e == ERR_BOARD_FULL)
	{
//		printf("the board is full, tie");
//		return FALSE;
		//TODO - what do I need to return here???
		return ITS_A_TIE;
	}
	if (e == ERR_SNAKE_IS_TOO_HUNGRY)
	{
		return Loser_To_Winner(player);
	}
	// only option is that e == ERR_OK
	if (IsMatrixFull(matrix))
	{
//		printf("the board is full, tie");
//		return FALSE;
		//TODO - what do I need to return here???
		return ITS_A_TIE;
	}

	return KEEP_PLAYING;
}

Point GetInputLoc(Matrix *matrix, Player player,int move)
{
	Point p;

	p = GetSegment(matrix, player);

	switch (move)
	{
	case UP:    --p.y; break;
	case DOWN:  ++p.y; break;
	case LEFT:  --p.x; break;
	case RIGHT: ++p.x; break;
	}
	return p;
}

Point GetSegment(Matrix *matrix, int segment)
{
	/* just run through all the matrix */
	Point p;
	for (p.x = 0; p.x < N; ++p.x)
	{
		for (p.y = 0; p.y < N; ++p.y)
		{
			if ((*matrix)[p.y][p.x] == segment)
				return p;
		}
	}
	p.x = p.y = -1;
	return p;
}

int GetSize(Matrix *matrix, Player player)
{
	/* check one by one the size */
	Point p, next_p;
	int segment = 0;
	while (TRUE)
	{
		next_p = GetSegment(matrix, segment += player);
		if (next_p.x == -1)
			break;

		p = next_p;
	}

	return (*matrix)[p.y][p.x] * player;
}

bool CheckTarget(Matrix *matrix, Player player, Point p)
{
	/* is empty or is the tail of the snake (so it will move the next
	to make place) */
	return (IsAvailable(matrix, p) || ((*matrix)[p.y][p.x] == player * GetSize(matrix, player)));
}

bool IsAvailable(Matrix *matrix, Point p)
{
	return
		/* is out of bounds */
		!(p.x < 0 || p.x >(N - 1) ||
		p.y < 0 || p.y >(N - 1) ||
		/* is empty */
		((*matrix)[p.y][p.x] != EMPTY && (*matrix)[p.y][p.x] != FOOD));
}

ErrorCode CheckFoodAndMove(Matrix *matrix, Player player, Point p, int* white_counter, int* black_counter)
{
	*white_counter = K;
	*black_counter = K;
	/* if the player did come to the place where there is food */
	if ((*matrix)[p.y][p.x] == FOOD)
	{
		if (player == BLACK) *black_counter = K;
		if (player == WHITE) *white_counter = K;

		IncSizePlayer(matrix, player, p);

		if (RandFoodLocation(matrix) != ERR_OK)
			return ERR_BOARD_FULL;
	}
	else /* check hunger */
	{
		if (player == BLACK && --(*black_counter) == 0)
			return ERR_SNAKE_IS_TOO_HUNGRY;
		if (player == WHITE && --(*white_counter) == 0)
			return ERR_SNAKE_IS_TOO_HUNGRY;

		AdvancePlayer(matrix, player, p);
	}
	return ERR_OK;
}

void AdvancePlayer(Matrix *matrix, Player player, Point p)
{
	/* go from last to first so the identifier is always unique */
	Point p_tmp, p_tail = GetSegment(matrix, GetSize(matrix, player) * player);
	int segment = GetSize(matrix, player) * player;
	while (TRUE)
	{
		p_tmp = GetSegment(matrix, segment);
		(*matrix)[p_tmp.y][p_tmp.x] += player;
		segment -= player;
		if (segment == 0)
			break;
	}
	(*matrix)[p_tail.y][p_tail.x] = EMPTY;
	(*matrix)[p.y][p.x] = player;
}

void IncSizePlayer(Matrix *matrix, Player player, Point p)
{
	/* go from last to first so the identifier is always unique */
	Point p_tmp;
	int segment = GetSize(matrix, player)*player;
	while (TRUE)
	{
		p_tmp = GetSegment(matrix, segment);
		(*matrix)[p_tmp.y][p_tmp.x] += player;
		segment -= player;
		if (segment == 0)
			break;
	}
	(*matrix)[p.y][p.x] = player;
}

ErrorCode RandFoodLocation(Matrix *matrix)
{
	Point p;
	do
	{
		p.x = rand();	//TEST - HW4 - Lotem
		p.y = rand();	//TEST - HW4 - Lotem
	} while (!(IsAvailable(matrix, p) || IsMatrixFull(matrix)));				//HW4 - Lotem - Added () here to prevent infinite loop

	if (IsMatrixFull(matrix))
		return ERR_BOARD_FULL;

	(*matrix)[p.y][p.x] = FOOD;
	return ERR_OK;
}

bool IsMatrixFull(Matrix *matrix)
{
	Point p;
	for (p.x = 0; p.x < N; ++p.x)
	{
		for (p.y = 0; p.y < N; ++p.y)
		{
			if ((*matrix)[p.y][p.x] == EMPTY || (*matrix)[p.y][p.x] == FOOD)
				return FALSE;
		}
	}
	return TRUE;
}

void Game_Print(Matrix* matrix,char* buffer,int* board_size){/* prints the state of the board */
	printk("\t[DEBUG]\tIn Game_Print method\n");
	//Making the buffer an empty string	- Lotem
	buffer[0] = '\0';
	int i;
	Point p;
	char temp_buffer[4];


	for (i = 0; i < N + 1; ++i)
		strcat(buffer,"---");
	strcat(buffer,"\n");

	for (p.y = 0; p.y < N; ++p.y)
	{
		strcat(buffer,"|");
		for (p.x = 0; p.x < N; ++p.x)
		{
			int matrixVal = (int)((*matrix)[p.y][p.x]) ;
			switch ((*matrix)[p.y][p.x])
			{
			case FOOD:
				strcat(buffer,"  *");
				break;
			case EMPTY:
				strcat(buffer,"  .");
				break;
			default:
				temp_buffer[0] = ' ';
				if (matrixVal < 0){
					temp_buffer[1] = '-';
					temp_buffer[2] = (-matrixVal)  + '0';
				} else {
					temp_buffer[1] = ' ';
					temp_buffer[2] = matrixVal  + '0';
				}
				temp_buffer[3] = '\0';

				strcat(buffer,temp_buffer);
			}
		}
		strcat(buffer," |\n");
	}

	for (i = 0; i < N + 1; ++i)
		strcat(buffer,"---");
	strcat(buffer,"\n");

	//Calculating the length of the buffer
	*board_size = strlen(buffer);
	printk("\t[DEBUG]:\tThis is the kernel print\n%s\n",buffer);

	/*
	 * Alternatice impl
	 */
/*
	Point p;
		char tmp_buff[3*N*N + 10*N + 10]; //3*N*N + 10*N + 8 = (3N+4)(N+2)= (#cols)*(#rows)
		int tmp_buff_size = 3*N*N + 10*N + 10;
		int i = 0, tmp_buff_curr = 0;

		for(i = 0; i < tmp_buff_size; i++)
			tmp_buff[i] = 0;
		for(i = 0; i < count; i++)
			buff[i] = 0;

		for (i = 0; i < N + 1; ++i){
			strncpy(tmp_buff+tmp_buff_curr,"---",3);
			tmp_buff_curr+=3;
		}
		strncpy(tmp_buff+tmp_buff_curr,"\n",1);
		tmp_buff_curr+=1;

		for (p.y = 0; p.y < N; ++p.y)
		{
			strncpy(tmp_buff+tmp_buff_curr,"|",1);
			tmp_buff_curr+=1;
			for (p.x = 0; p.x < N; ++p.x)
			{
				switch ((*matrix)[p.y][p.x])
				{
				case FOOD:
					strncpy(tmp_buff+tmp_buff_curr,"  *",3);
					tmp_buff_curr+=3;
					break;
				case EMPTY:
					strncpy(tmp_buff+tmp_buff_curr,"  .",3);
					tmp_buff_curr+=3;
					break;
				default: //printf("% 3d", (*matrix)[p.y][p.x]);
					tmp_buff[tmp_buff_curr++]=' ';
					int num = (*matrix)[p.y][p.x];
					if(num < 0){
						tmp_buff[tmp_buff_curr++] = '-';
						num *= -1;
					}
					else tmp_buff[tmp_buff_curr++] = ' ';
					tmp_buff[tmp_buff_curr++] = num + '0';
				}
			}
			strncpy(tmp_buff+tmp_buff_curr," |\n",3);
			tmp_buff_curr+=3;
		}

		for (i = 0; i < N + 1; ++i){
			strncpy(tmp_buff+tmp_buff_curr,"---",3);
			tmp_buff_curr+=3;
		}
		strncpy(tmp_buff+tmp_buff_curr,"\n",1);
		tmp_buff_curr+=1;

		strncpy(buff, tmp_buff, count);
*/
}

#endif /* _HW3Q1_H_ */
