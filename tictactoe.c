/*****************************************************************************
 *	Title		: tictactoe.c
 *	Author		: Wai Khoo
 *	Created on	: December 9, 2011
 *	Description	: A simple 2D Tic-Tac-Toe game with AI
 *	Purpose		: Demonstrate how to interact with system and controlling terminal input/output by making a game
 *	Usage		: tictactoe
 *	Remark		: Arrow keys to navigate; Enter key to make a selection; F1 key to quit.
 *			  The AI behind this game is based on Negamax, which is a brute force search on the game N-ary tree. This tree has zero-sum property of a two-player game. 
 *	Build with	: gcc -o tictactoe tictactoe.o -lncurses -lpthread
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* Various macro for the screen location */
#define GRIDX (COLS/3)
#define GRIDY (LINES/4)
#define CTRX  (COLS/6)
#define CTRY  (LINES/8)
#define TRUE  1
#define FALSE 0

/* Global variables */
enum Player {Human, Computer}; 

int currPlayer;		/* Current player in the game */
int negamaxCurrPlayer;	/* Virtual current player when computer is making a move */

int player1, player2;	/* Players */
int computerEnabled;	/* Whether you're playing with a computer or not */
int currGridX, currGridY;	/* Current grid selection on board */
int unoccupiedSq;	/* How many square that is still empty or unmarked */
int stackTop=0;		/* Index to keep track of the top element in the stack */

/* 
 * board representation - 8 possible winning moves
 * 	0 1 2
 * 	3 4 5
 * 	6 7 8
*/
const int scoring[8][3] = {
	{0,1,2},
	{3,4,5},
	{6,7,8},
	{0,3,6},
	{1,4,7},
	{2,5,8},
	{0,4,8},
	{2,4,6}};

int ttt_board[9];	/* Game board */
int board_stack[100];	/* Stack for the AI to keep track of "moves" he made */

/* Function to draw cross on the screen */
void drawCross(int *ctrY, int *ctrX);
/* Function to draw circle on the screen */
void drawCircle(int *ctrY, int *ctrX);
/* Function to the game board on the screen */
void drawboard();
/* Set which player is human or computer based on user's selection on menu */
void setPlayer(enum Player one, enum Player two);
/* Print the game menu on the screen */
void printmenu();
/* Start the game based on user's selection */
void startGame();
/* Check to see if there's a winning move on the board */
int checkVictory();
/* Display the victory result */
void winOrDraw(int result);
/* Computer is making a move */
void computerMove();
/* Human is making a move */
void humanMove();
/* A separate thread to handle the AI processing */
void *runComputer();
/* AI - Negamax */
int applyNegamax();
/* AI making a (virtual) move */
void negamaxMakeMove(int pos, int side);
/* AI reversing a virtual move*/
void negamaxUndoMove();
/* Function to give the illusion of sleep in seconds */
void wait(int seconds);

/************************  MAIN  *************************************/
int main()
{
	initscr();  		/* Initialize screen */
	clear();		/* Clear the screen */
	noecho();		/* turn off character echo */
	cbreak();		/* disable line buffering */
	keypad(stdscr, TRUE);	/* turn on function keys */

	startGame();		/* start the game */
	
	endwin();		/* close the screen */
	return 0;
}

/******************* Function to draw cross on the screen ************/
void drawCross(int *ctrY, int *ctrX)
{
	/* Save original location */
	int orig_x = *ctrX;
	int orig_y = *ctrY;
	
	/* Offset the specified location */
	*ctrY = (*ctrY)-4;
	*ctrX = (*ctrX)-4;
	move(*ctrY, *ctrX); /* Move */
	
	/* Draw the cross with fixed width and height */
	addstr("\\       /"); move(++(*ctrY),*ctrX);
	addstr(" \\     / "); move(++(*ctrY),*ctrX);
	addstr("  \\   /  "); move(++(*ctrY),*ctrX);
	addstr("   \\ /   "); move(++(*ctrY),*ctrX);
	addstr("    X     "); move(++(*ctrY),*ctrX);
	addstr("   / \\   "); move(++(*ctrY),*ctrX);
	addstr("  /   \\  "); move(++(*ctrY),*ctrX);
	addstr(" /     \\ "); move(++(*ctrY),*ctrX);
	addstr("/       \\"); move(++(*ctrY),*ctrX);

	/* Restore the original location */
	*ctrY = orig_y;
	*ctrX = orig_x;
	move(*ctrY,*ctrX);
}

/* Function to draw circle on the screen */
void drawCircle(int *ctrY, int *ctrX)
{
	/* Save original location */
	int orig_x = *ctrX;
	int orig_y = *ctrY;
	
	/* Offset the specified location */
	*ctrY = (*ctrY)-4;
	*ctrX = (*ctrX)-4;
	move(*ctrY, *ctrX);

	/* Draw the circle with fixed width and height */
	addstr("  ooooo  "); move(++(*ctrY),*ctrX);
	addstr(" o     o "); move(++(*ctrY),*ctrX);
	addstr("o       o"); move(++(*ctrY),*ctrX);
	addstr("o       o"); move(++(*ctrY),*ctrX);
	addstr("o       o"); move(++(*ctrY),*ctrX);
	addstr("o       o"); move(++(*ctrY),*ctrX);
	addstr("o       o"); move(++(*ctrY),*ctrX);
	addstr(" o     o "); move(++(*ctrY),*ctrX);
	addstr("  ooooo  "); move(++(*ctrY),*ctrX);
	
	/* Restore the original location */
	*ctrY = orig_y;
	*ctrX = orig_x;
	move(*ctrY,*ctrX);
}

/* Function to the game board on the screen */
void drawboard()
{
	int i;

	/* Draw the grid - horizontal */
	for(i = 1; i < 3*GRIDX; i++)
	{
		mvaddstr(1, i, "-");
		mvaddstr(GRIDY, i, "-");
		mvaddstr(2*GRIDY, i, "-");
		mvaddstr(3*GRIDY, i, "-");
	}

	/* Draw the grid - vertical */
	for(i = 1; i < 3*GRIDY; i++)
	{
		mvaddstr(i, 1, "|");
		mvaddstr(i, GRIDX, "|");
		mvaddstr(i, 2*GRIDX, "|");
		mvaddstr(i, 3*GRIDX, "|");
	}

	/* Initalize the game board */
	/* -1 means empty */
	for(i = 0; i < 9; i++)
		ttt_board[i] = -1;

	/* 9 empty square */
	unoccupiedSq = 9;
}

/*** Set which player is human or computer based on user's selection on menu **/
void setPlayer(enum Player one, enum Player two)
{
	player1 = one;
	player2 = two;
	currPlayer = 1; /* Player 1 always goes first */

	/* Checking to see if a computer is playing or not */
	if ((player1 == Computer) || (player2 == Computer))
		computerEnabled = TRUE;
	else
		computerEnabled = FALSE;
}

/*********************** Print the game menu on the screen ********************/
void printmenu()
{
	int c, valid=FALSE;

	/* Clear the screen and print out menu in the center */
	clear();
	mvaddstr(LINES/2, COLS/2-13, "1) Human plays first");
	mvaddstr(LINES/2+1, COLS/2-13, "2) Computer plays first");
	mvaddstr(LINES/2+2, COLS/2-13, "3) Human vs Human");
	mvaddstr(LINES/2+3, COLS/2-13, "4) Computer vs Computer");
	mvaddstr(LINES/2+4, COLS/2-13, "F1 to Quit.");
	mvaddstr(LINES/2+6, COLS/2-13, "Player 1 is O. Player 2 is X.");
	mvaddstr(LINES/2+8, COLS/2-13, "Arrow keys to navigate.");
	mvaddstr(LINES/2+9, COLS/2-13, "Enter key to make selection");

	/* Prompt the user for input on bottom of screen */
	mvaddstr(LINES-1, 1, "Please pick a choice:"); 

	/* While the user hasn't enter a valid choice, keep prompting for input */
	/* Set the players based on menu selected */
	while(!valid)
	{
		c = getch(); /* Get input */
		switch(c)
		{
			case '1': /* Human plays first */
				mvaddstr(LINES-1, 23, "1");
				setPlayer(Human, Computer);
				valid = TRUE;
				break;
			case '2': /* Computer plays first */
				mvaddstr(LINES-1, 23, "2");
				setPlayer(Computer, Human);
				valid = TRUE;
				break;
			case '3': /* Human vs. human */
				mvaddstr(LINES-1, 23, "3");
				setPlayer(Human, Human);
				valid = TRUE;
				break;
			case '4': /* Computer vs. computer */
				mvaddstr(LINES-1, 23, "4");
				setPlayer(Computer, Computer);
				valid = TRUE;
				break;
			case KEY_F(1): /* Quit */
				endwin();
				exit(1);
		}
	}
	refresh(); /* refresh the screen */
	wait(1); /* Wait 1 second for visualization */
}

/******************* Start the game based on user's selection *****************/
void startGame()
{
	/* Print the menu and get user input, then set the game and draw board */
	printmenu();
	clear();
	drawboard();
	
	/* Default location is the middle square */
	currGridX = 2;
	currGridY = 2;
	
	/* Move to center */
	/* Need to convert grid coordinate to screen coordinate */
	move((currGridY*2-1)*CTRY, (currGridX*2-1)*CTRX);
	
	/* Figure what type of player moves first */
	if (player1 == Human)
		humanMove();
	else
		computerMove();
}

/************* Check to see if there's a winning move on the board ************/
int checkVictory()
{
	/* 0 = ongoing; 1 = o win; 2 = x win; 3 = draw */
	int i;
	/* For each 8 possible winning move, check the board to see if there's one*/
	for(i = 0; i < 8; i++)
	{
		if ((ttt_board[scoring[i][0]] != -1) && (ttt_board[scoring[i][0]] == ttt_board[scoring[i][1]]) && (ttt_board[scoring[i][1]] == ttt_board[scoring[i][2]]))
			return ttt_board[scoring[i][0]]; /* There's a winning move, return the player that wins */
	}

	/* All the squares have been filled and yet no winner, so it's a draw */
	if (unoccupiedSq == 0)
		return 3;
	
	/* At this point, no winner or draw, so game is ongoing */
	return 0;
}

/************************* Display the victory result ************************/
void winOrDraw(int result)
{
	/* Refresh the screen, showing the final move */
	refresh();

	/* Wait 1 second so user can see the final move */
	wait(1);

	/* Clear the screen and display result */
	clear();

	/* Display winning/losing message based on result */
	switch(result)
	{
		case 1: /* Player 1 win */
			if ((player1 == Human) && (player1 != player2))
				mvaddstr(LINES/2, COLS/2-20, "You beat the computer! Victory for you, Player 1.");
			else if ((player1 == Human) && (player1 == player2))
				mvaddstr(LINES/2, COLS/2-20, "Player 1 win! Player 2 suck.");
			else if ((player1 == Computer) && (player1 != player2))
				mvaddstr(LINES/2, COLS/2-20, "You got beaten by a computer. You lose!");
			else if ((player1 == Computer) && (player1 == player2))
				mvaddstr(LINES/2, COLS/2-20, "Computer 1 beat computer 2!!!");
			break;
		case 2: /* Player 2 win */
			if ((player2 == Human) && (player1 != player2))
				mvaddstr(LINES/2, COLS/2-20, "You beat the computer! Victory for you, Player 2.");
			else if ((player2 == Human) && (player1 == player2))
				mvaddstr(LINES/2, COLS/2-20, "Player 2 win! Player 1 suck.");
			else if ((player2 == Computer) && (player1 != player2))
				mvaddstr(LINES/2, COLS/2-20, "You got beaten by a computer. You lose!");
			else if ((player2 == Computer) && (player1 == player2))
				mvaddstr(LINES/2, COLS/2-20, "Computer 2 beat computer 1!!!");
			break;
		case 3: /* Draw */
			mvaddstr(LINES/2, COLS/2-13, "Draw! Try again.");
			break;
	}

	/* Let the user see the result and possibly start a new game */
	mvaddstr(LINES-1, 1, "Press any key to continue");
	getch();
	startGame();
}

/************** Function to give the illusion of sleep in seconds ************/
void wait(int seconds)
{
	/* Simulate a wait based on number of seconds specified */
	clock_t endwait;
	endwait = clock() + seconds * CLOCKS_PER_SEC;
	while(clock() < endwait) {}
}

/************************** Computer is making a move ************************/
void computerMove()
{
	int r, c, vic, compBestMove;
	void *status;
	pthread_t pth;

	/* Current virtual player */
	negamaxCurrPlayer = currPlayer;

	/* Create worker thread */
	pthread_create(&pth, NULL, runComputer, NULL);

	/* wait for the thread to finish before continuing */
	pthread_join(pth, (void**) (&status));
	compBestMove = (int) status; /* Computer made a move */	
	
	wait(1); /* wait 1 second */

	/* When the computer figure out a move, it will be stored in compBestMove*/
	/* Convert the move (in linear index) to 2D grid coordinate */
	currGridY = (compBestMove / 3) + 1;
	currGridX = (compBestMove % 3) + 1;
	
	/* Convert 2D grid coordinate to screen coordinate */
	r = (currGridY*2-1)*CTRY;
	c = (currGridX*2-1)*CTRX;
	
	/* Move to that grid */
	move(r, c);

	/* Mark the square based on whether the computer is first or second player */
	if (currPlayer == 1)
		drawCircle(&r, &c);
	else
		drawCross(&r, &c);

	/* Mark the board as well */
	ttt_board[compBestMove] = currPlayer;
	unoccupiedSq--;

	/* Check to see if this new move yield a victory or not */
	if ((vic = checkVictory()) != 0)
		winOrDraw(vic);

	/* Refresh the screen */
	refresh();

	/* Switch player and make move by calling the appropriate function for human or computer */
	currPlayer ^= 0x3; 		/* Exclusive-or will flip 1 to 2 and vice versa */
	if ((currPlayer == 1) && (player1 == Computer))
		computerMove();
	else if ((currPlayer == 2) && (player2 == Computer))
		computerMove();
	else
		humanMove();
}

/***************************** AI making a (virtual) move *********************/
void negamaxMakeMove(int pos, int side)
{
	/* Mark the board with the side the current player is on, either 1 or 2 */
	ttt_board[pos] = side;
	unoccupiedSq--; 		/* Decrement count for empty square */
	board_stack[stackTop++] = pos; 	/* Keep track of the virtual move made in a stack */
	negamaxCurrPlayer ^= 0x3; 	/* Switch player, virtually */
}

/************************** AI reversing a virtual move ***********************/
void negamaxUndoMove()
{
	/* Pop the stack */
	int pos = board_stack[--stackTop];

	/* Undo the virtual move by emptying it out */
	ttt_board[pos] = -1;
	unoccupiedSq++;			/* Increment count for empty square */
	negamaxCurrPlayer ^= 0x3;	/* Switch player, virtually */
}

/******************* A separate thread to handle the AI processing ************/
void *runComputer()
{
	/* Simple heuristic values
	 * 	-2 = no play; -1 = loss; 0 = draw; 1 = win; otherwise = score of move i
  	*/

	int scores[9];
	int i, maxScore = -3;

	/* For each empty square, make a move and compute its score */
	for(i = 0; i < 9; i++)
	{
		if (ttt_board[i] == -1)
		{
			/* Make a virtual move and use negamax to compute score */
			negamaxMakeMove(i, negamaxCurrPlayer);
			scores[i] = -applyNegamax();
			negamaxUndoMove();	/* Undo virtual move */
		}
		else
			scores[i] = -2;		/* Can't make a move on this square */
	}

	/* See which move (i.e. which square) gives the highest score */
	static int bestMove = 0;
	for(i = 0; i < 9; i++)
	{
		if (scores[i] > maxScore)
		{
			/* Highest score constitutes the best move, save the index */
			maxScore = scores[i];
			bestMove = i;
		}
	}

	pthread_exit((void*) bestMove);
}

/*********************************** AI - Negamax *****************************/
int applyNegamax()
{	/*
		Return values:
		-1 = loss for current player
		0 = draw for both players
		1 = win for current player.
		score = for that move
	*/
	/* See if there's already a victory or draw */
	int state = checkVictory();

	if ((state == 1) || (state == 2))
	{
		if (negamaxCurrPlayer == state)
			return 1;
		else
			return -1;
	}
	else if (state == 3)
		return 0;

	/* Otherwise, recursively call applyNegamax to figure out which move will gives the computer highest score and thus the lowest score for the other player */
	int scores[9];
	int i, maxScore = -3;

	/* For each empty square, make a move and compute its score */
	for(i = 0; i < 9; i++)
	{
		if (ttt_board[i] == -1)
		{
			/* Make a virtual move and use negamax to compute score */
			negamaxMakeMove(i, negamaxCurrPlayer);
			scores[i] = -applyNegamax();
			negamaxUndoMove();	/* Undo virtual move */
		}
		else
			scores[i] = -2;		/* Can't make a move on this square */
	}

	/* See which move (i.e. which square) gives the highest score */
	for(i = 0; i < 9; i++)
	{
		/* Highest score constitutes the best move */
		if (scores[i] > maxScore)
			maxScore = scores[i];
	}

	/* Return the highest score */
	return maxScore;
}

/**************************** Human is making a move **************************/
void humanMove()
{
	int ch, r, c, ind, vic;

	/* Prompt the user for navigational input or quit */
	/* Pressing the enter means marking the square (making your move) */
	while((ch = getch()) != '\n')
	{
		switch(ch)
		{
			/* The arrow keys will wrap around the grid */
			case KEY_LEFT:
				currGridX = (currGridX == 1) ? 3:currGridX-1;
				break;
			case KEY_RIGHT:
				currGridX = (currGridX == 3) ? 1:currGridX+1;
				break;
			case KEY_UP:
				currGridY = (currGridY == 1) ? 3:currGridY-1;
				break;
			case KEY_DOWN:
				currGridY = (currGridY == 3) ? 1:currGridY+1;
				break;
			case KEY_F(1):
				endwin();
				exit(1);
				break;
		}
		/* Move the cursor to the center of the grid */
		move((currGridY*2-1)*CTRY, (currGridX*2-1)*CTRX);			}

	/* Enter has been pressed, get the linear index for that location */
	ind = (currGridY-1)*3 + (currGridX-1);

	/* If the location is not empty, try again */
	if (ttt_board[ind] != -1)
		humanMove();
		
	/* Otherwise, get the screen coordinate */
	r = (currGridY*2-1)*CTRY;
	c = (currGridX*2-1)*CTRX;

	/* Draw circle or cross depending on which side the current player is on */
	if (currPlayer == 1)
		drawCircle(&r, &c);
	else 
		drawCross(&r, &c);

	/* Mark the board as well */
	ttt_board[ind] = currPlayer;
	unoccupiedSq--;

	/* Check to see if this new move yield a victory or not */
	if((vic = checkVictory()) != 0)
		winOrDraw(vic);

	/* Refresh the screen */
	refresh(); 

	/* Switch player and make move by calling the appropriate function for human or computer */
	currPlayer ^= 0x3; 		/* Exclusive-or will flip 1 to 2 and vice versa */
	if ((currPlayer == 1) && (player1 == Computer))
		computerMove();
	else if ((currPlayer == 2) && (player2 == Computer))
		computerMove();
	else
		humanMove();
}
