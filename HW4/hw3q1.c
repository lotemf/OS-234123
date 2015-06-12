/******************************************************************************/
		//This is the .c file
/******************************************************************************/

//Includes
//#include <linux/sched.h>
//#include "hw3q1.h"

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


//int main()
//{
//	Player player = WHITE;
//	Matrix matrix = { { EMPTY } };
//
//	if (!Init(&matrix))
//	{
//		printf("Illegal M, N parameters.");
//		return -1;
//	}
//	while (Update(&matrix, player))
//	{
//		Print(&matrix);
//		/* switch turns */
//		player = -player;
//	}
//
//	return 0;
//}

bool Game_Init(Matrix* matrix)
{
	int i;
	/* initialize the snakes location */
	for (i = 0; i < M; ++i)
	{
		(*matrix)[0][i] =   WHITE * (i + 1);
		(*matrix)[N - 1][i] = BLACK * (i + 1);
	}
	/* initialize the food location */
//	srand(time(0));						//TODO - Change the use of random
	if (RandFoodLocation(matrix) != ERR_OK)
		return FALSE;
//	printf("instructions: white player is represented by positive numbers, \nblack player is represented by negative numbers\n");
	char* buffer = NULL;
	int board_size = 0;

	Game_Print(matrix,buffer,&board_size);

	return TRUE;
}

int Game_Update(Matrix *matrix, Player player,int move)
{
	ErrorCode e;
	Point p = GetInputLoc(matrix, player,move);

	if (!CheckTarget(matrix, player, p))
	{
		return Loser_To_Winner(player);
	}
	e = CheckFoodAndMove(matrix, player, p);
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
//	Direction dir;
	Point p;

	//The input check isn't needed here...
//	printk("% d, please enter your move(DOWN2, LEFT4, RIGHT6, UP8):\n", player);
//	do
//	{
//		if (scanf("%d", &dir) < 0)
//		{
//			printf("an error occurred, the program will now exit.\n");
//			exit(1);
//		}
//		if (move != UP   && move != DOWN && move != LEFT && move != RIGHT)
//		{
//			printf("invalid input, please try again\n");
//		}
//		else
//		{
//			break;
//		}
//	} while (TRUE);

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
	return IsAvailable(matrix, p) || ((*matrix)[p.y][p.x] == player * GetSize(matrix, player));
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

ErrorCode CheckFoodAndMove(Matrix *matrix, Player player, Point p)
{
	static int white_counter = K;
	static int black_counter = K;
	/* if the player did come to the place where there is food */
	if ((*matrix)[p.y][p.x] == FOOD)
	{
		if (player == BLACK) black_counter = K;
		if (player == WHITE) white_counter = K;

		IncSizePlayer(matrix, player, p);

		if (RandFoodLocation(matrix) != ERR_OK)
			return ERR_BOARD_FULL;
	}
	else /* check hunger */
	{
		if (player == BLACK && --black_counter == 0)
			return ERR_SNAKE_IS_TOO_HUNGRY;
		if (player == WHITE && --white_counter == 0)
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
		p.x = rand() % N;		//TODO - change the rand
		p.y = rand() % N;
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

	//Making the buffer an empty string	- Lotem
	buffer[0] = '\0';
	int i;
	Point p;
	char temp_buffer[3];

	for (i = 0; i < N + 1; ++i)
		strcat(buffer,"---");
	strcat(buffer,"\n");
	for (p.y = 0; p.y < N; ++p.y)
	{
		strcat(buffer,"|");
		for (p.x = 0; p.x < N; ++p.x)
		{
			switch ((*matrix)[p.y][p.x])
			{
			case FOOD:  strcat(buffer,"  *");
			break;
			case EMPTY: strcat(buffer,"  .");
			break;
			default:
				snprintf(temp_buffer,3,"% 3d", (*matrix)[p.y][p.x]);
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
}
