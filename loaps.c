// Includes

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef DEBUG
	#define DEBUG 0
#else
	#define DEBUG 1
#endif

// Constants

const int sevenPointSquareCoord = 3;
const int threePointSquareCoordOne = 1;
const int threePointSquareCoordTwo = 5;

#define WIDTH 7
#define HEIGHT 7

#define EMPTY 0
#define PLAYER_ONE 1
#define PLAYER_TWO 2
#define BOTH 3

#define BACKSLASH_DIRECTION 1
#define FORWARDSLASH_DIRECTION 2

#define N 0
#define NE 1
#define E 2
#define SE 3
#define S 4
#define SW 5
#define W 6
#define NW 7

typedef struct gameMoveStruct {
	int fromX;
	int fromY;
	int toX;
	int toY;
	int score;
	int capture;
} gameMove;

// Global variables

int me = 0;		// Which player we are, one or two
int him = 0;	// Which player they are, one or two

int nextMoveNum = 0;	// The number of the next move

int playerOneScore = 0, playerTwoScore = 0;					// Scores for the two players
double playerOneTimeLeft = 60.0, playerTwoTimeLeft = 60.0;	// Time left for the two players
int playerOneCount = 0, playerTwoCount = 0;

int *ourScore, *hisScore;						// Pointers to the scores for quick reference
double *ourTime, *hisTime;						// Pointers to time left for quick reference
int *ourCount, *hisCount;

gameMove finalMove;

int gameBoard[WIDTH][HEIGHT];
int rowCounts[HEIGHT] = {-1, -1, -1, -1, -1, -1, -1};
int columnCounts[WIDTH] = {-1, -1, -1, -1, -1, -1, -1};

int myPiecesX[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
int myPiecesY[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

// Function prototypes

int main(int argc, char** argv);
int sevenPointSquare(int x, int y);
int threePointSquare(int x, int y);
int piecesOnRow(int row, int kind);
int piecesOnColumn(int column, int kind);
int piecesOnDiagonal(int x, int y, int direction, int kind);
void selectMove();
void movesTo(int x, int y, gameMove possibleMoves[], int *count);
void movesFrom(int x, int y, gameMove possibleMoves[], int *count);
gameMove *allocateMove();
void scoreMove(int x, int y, gameMove possibleMoves[], int index);
void printBoard();
int evaluateMove(gameMove possibleMoves[8*8], int which);

// Function definitions

void printBoard() {
	int x, y;

	printf("\nBoard:\n\n");

	for (y = 0; y < HEIGHT; y++) {
		for (x = 0; x < WIDTH; x++) {
			if (gameBoard[x][y] == PLAYER_ONE)
				printf("1");
			else if (gameBoard[x][y] == PLAYER_TWO)
				printf("2");
			else
				printf(".");
		}
		printf("\n");
	}

	printf("\n");	

	printf("Forward:\n\n");

	for (y = 0; y < HEIGHT; y++) {
		for (x = 0; x < WIDTH; x++) {
			printf("%d", piecesOnDiagonal(x, y, FORWARDSLASH_DIRECTION, BOTH));
		}
		printf("\n");
	}

	printf("\nBackward:\n\n");

	for (y = 0; y < HEIGHT; y++) {
		for (x = 0; x < WIDTH; x++) {
			printf("%d", piecesOnDiagonal(x, y, BACKSLASH_DIRECTION, BOTH));
		}
		printf("\n");
	}

	printf("\n\n");
}

void scoreMoves(int x, int y, gameMove possibleMoves[], int index) {
	possibleMoves[index].score = 0;
	possibleMoves[index].capture = 0;

	if (gameBoard[x][y] == him) {
		if (DEBUG)
			printf("\t\t\t%d, %d is a capture.\n", x, y);
		possibleMoves[index].capture = 1;
		possibleMoves[index].score += 1;
	}
	if (sevenPointSquare(x, y)) {
		if (DEBUG)
			printf("\t\t\t%d, %d is a seven point.\n", x, y);
		possibleMoves[index].score += 7;
	} else if (threePointSquare(x, y)) {
		if (DEBUG)
			printf("\t\t\t%d, %d is a three point.\n", x, y);
		possibleMoves[index].score += 3;
	}

	if (DEBUG)
		if (possibleMoves[index].score == 0)
			printf("\t\t\t%d, %d scores a 0.\n", x, y);
}

int validMove(int fromX, int fromY, int toX, int toY, int direction) {
	int count = 0;
	int i, xDelta, yDelta;

	if (DEBUG)
		printf("\tAsked to validate %d, %d to %d, %d; direction: %d...\n", fromX, fromY, toX, toY, direction);

	// Are the ending coords valid?

	if ((toX < 0) || (toX >= WIDTH) || (toY < 0) || (toY >= HEIGHT)) {
		if (DEBUG)
			printf("\t\tEnding coords of %d, %d were invalid.\n", toX, toY);
		return 0;
	}

	// First, find out if we can land there

	if (gameBoard[toX][toY] == me) {
		// Our piece is there, so the move is illegal
		if (DEBUG)
			printf("\t\tWould have to land on our piece at %d, %d.\n", toX, toY);
		return 0;
	}

	switch(direction) {
		case N:
		case S:
			count = piecesOnColumn(fromX, BOTH);
			break;
		case E:
		case W:
			count = piecesOnRow(fromY, BOTH);
			break;
		case NE:
		case SW:
			count = piecesOnDiagonal(fromX, fromY, FORWARDSLASH_DIRECTION, BOTH);
			break;
		case NW:
		case SE:
			count = piecesOnDiagonal(fromX, fromY, BACKSLASH_DIRECTION, BOTH);
			break;
		default:
			printf("\nInvalid direction in validMove: %d.\n\n", direction);
			exit(1);
	}

	if (DEBUG)
		printf("\t\tCount for our direction was %d.\n", count);

	xDelta = abs(fromX - toX);
	yDelta = abs(fromY - toY);

	if (xDelta == yDelta) {
		// Pure diagonal, is it OK?
		if (xDelta == count) {
			if (DEBUG) // It was OK
				printf("\t\tOK: Delta of %d, %d on count %d was valid.\n", xDelta, yDelta, count);
		} else {
			if (DEBUG) // It was OK
				printf("\t\tBAD: Delta of %d, %d on count %d were invalid.\n", xDelta, yDelta, count);
			return 0;
		}
	} else {
		// OK, it must be a straight move, check that
		if (((xDelta == 0) && (yDelta == count)) ||
				((yDelta == 0) && (xDelta == count))) {
			if (DEBUG) // It was OK
				printf("\t\tOK: Delta of %d, %d on count %d was valid.\n", xDelta, yDelta, count);				
		} else {
			if (DEBUG) // It was OK
				printf("\t\tBAD: Delta of %d, %d on count %d were invalid.\n", xDelta, yDelta, count);
			return 0;
		}		
	}

	switch(direction) {
		case N:
			i = fromY;
			while (i >= 0) {
				if (i == toY) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[toX][i] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n", toX, i);
					return 0;	// Can't jump over the opponent
				}
				i--;
			}
			break;
		case S:
			i = fromY;
			while (i < WIDTH) {
				if (i == toY) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[toX][i] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n", toX, i);
					return 0;	// Can't jump over the opponent
				}
				i++;
			}
			break;
		case E:
			i = fromX;
			while (i < WIDTH) {
				if (i == toX) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[i][toY] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n", i, toY);
					return 0;	// Can't jump over opponent
				}
				i++;
			}
			break;
		case W:
			i = fromX;
			while (i >= 0) {
				if (i == toX) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[i][toY] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n", i, toY);
					return 0;	// Can't jump over opponent
				}
				i--;
			}
			break;
		case NE:
			i = 1;
			while ((fromX + i < HEIGHT) && (fromY - i >= 0)) {
				if ((fromX + i == toX) && (fromY - i == toY)) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[fromX + i][fromY - i] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n",
																			fromX + i, fromY - i);
					return 0;	// Can't jump over opponent
				}
				i++;
			}
			break;
		case SW:
			i = 1;
			while ((fromY + i < HEIGHT) && (fromX - i >= 0)) {
				if ((fromX - i == toX) && (fromY + i == toY)) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[fromX - i][fromY + i] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n",
																			fromX - i, fromY + i);
					return 0;	// Can't jump over opponent
				}
				i++;
			}
			break;
		case NW:
			i = 1;
			while ((fromX - i >= 0) && (fromY - i >= 0)) {
				if ((fromX - i == toX) && (fromY - i == toY)) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[fromX - i][fromY - i] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n",
																			fromX - i, fromY - i);
					return 0;	// Can't jump over opponent
				}
				i++;
			}
			break;
		case SE:
			i = 1;
			while ((fromX + i < WIDTH) && (fromY + i < HEIGHT)) {
				if ((fromX + i == toX) && (fromY + i == toY)) {
					if (DEBUG)
						printf("\t\tMove was OK.\n");
					return 1;	// Move was OK
				} else if (gameBoard[fromX + i][fromY + i] == him) {
					if (DEBUG)
						printf("\t\tMove would require jumping over opponent at %d, %d.\n",
																			fromX + i, fromY + i);
					return 0;	// Can't jump over opponent
				}
				i++;
			}
			break;
		default:
			printf("ERROR: Invalid direction number in validMove: %d\n", direction);
			exit(1);
	}

	// If we're here, it was invalid

	return 0;
}

void movesTo(int x, int y, gameMove *possibleMoves, int *count) {
	int countR, countC, countDF, countDB;

	countR = piecesOnRow(y, BOTH);
	countC = piecesOnColumn(x, BOTH);
	countDF = piecesOnDiagonal(x, y, FORWARDSLASH_DIRECTION, BOTH);
	countDB = piecesOnDiagonal(x, y, BACKSLASH_DIRECTION, BOTH);

	// Now we try each direction to see moves

	if (y + countC < HEIGHT)								// Northward move
		if (!gameBoard[x][y + countC] == me) {
			if (validMove(x, y + countC, x, y, N)) {
				possibleMoves[*count].fromX = x;
				possibleMoves[*count].fromY = y + countC;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
	if ((x - countDB >= 0) && (y + countDB < HEIGHT))		// Northeast move
		if (!gameBoard[x - countDB][y + countDB] == me) {
			if (validMove(x - countDB, y + countDB, x, y, NE)) {
				possibleMoves[*count].fromX = x - countDB;
				possibleMoves[*count].fromY = y + countDB;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
	if (x - countR >= 0)								// Eastward move
		if (!gameBoard[x - countR][y] == me) {
			if (validMove(x - countR, y, x, y, E)) {
				possibleMoves[*count].fromX = x - countR;
				possibleMoves[*count].fromY = y;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
	if ((x - countDB >= 0) && (y - countDB >= 0))		// Southeast move
		if (!gameBoard[x - countDB][y - countDB] == me) {
			if (validMove(x - countDB, y - countDB, x, y, NE)) {
				possibleMoves[*count].fromX = x - countDB;
				possibleMoves[*count].fromY = y - countDB;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
	if (y - countC >= 0)								// Southward move
		if (!gameBoard[x][y - countC] == me) {
			if (validMove(x, y - countC, x, y, S)) {
				possibleMoves[*count].fromX = x;
				possibleMoves[*count].fromY = y - countC;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
	if ((x + countDB < WIDTH) && (y - countDB >= 0))		// Southwest move
		if (!gameBoard[x + countDB][y - countDB] == me) {
			if (validMove(x + countDB, y - countDB, x, y, NE)) {
				possibleMoves[*count].fromX = x + countDB;
				possibleMoves[*count].fromY = y - countDB;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
	if (x + countR < WIDTH) 								// Westward move
		if (!gameBoard[x + countR][y] == me) {
			if (validMove(x + countR, y, x, y, W)) {
				possibleMoves[*count].fromX = x + countR;
				possibleMoves[*count].fromY = y;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
	if ((x + countDB >= 0) && (y + countDB <= HEIGHT))		// Northeast move
		if (!gameBoard[x + countDB][y + countDB] == me) {
			if (validMove(x + countDB, y + countDB, x, y, NE)) {
				possibleMoves[*count].fromX = x + countDB;
				possibleMoves[*count].fromY = y + countDB;
				possibleMoves[*count].toX = x;
				possibleMoves[*count].toY = y;
				scoreMoves(x, y, possibleMoves, *count);
				(*count)++;
			}
		}
}

void movesFrom(int x, int y, gameMove *possibleMoves, int *count) {
	int countR, countC, countDF, countDB;

	countR = piecesOnRow(y, BOTH);
	countC = piecesOnColumn(x, BOTH);
	countDF = piecesOnDiagonal(x, y, FORWARDSLASH_DIRECTION, BOTH);
	countDB = piecesOnDiagonal(x, y, BACKSLASH_DIRECTION, BOTH);

	if (DEBUG) {
		printf("\nAsked to find moves from %d, %d.\n", x, y);
		printf("\tcR: %d, cC: %d, cDF: %d, cDB: %d\n\n", countR, countC, countDF, countDB);
	}

	if (validMove(x, y, x, y - countC, N)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x;
		possibleMoves[*count].toY = y - countC;
		scoreMoves(x, y - countC, possibleMoves, *count);
		(*count)++;
	}

	if (validMove(x, y, x + countDF, y - countDF, NE)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x + countDF;
		possibleMoves[*count].toY = y - countDF;
		scoreMoves(x + countDF, y - countDF, possibleMoves, *count);
		(*count)++;
	}

	if (validMove(x, y, x + countR, y, E)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x + countR;
		possibleMoves[*count].toY = y;
		scoreMoves(x + countR, y, possibleMoves, *count);
		(*count)++;
	}

	if (validMove(x, y, x + countDB, y + countDB, SE)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x + countDB;
		possibleMoves[*count].toY = y + countDB;
		scoreMoves(x + countDB, y + countDB, possibleMoves, *count);
		(*count)++;
	}

	if (validMove(x, y, x, y + countC, S)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x;
		possibleMoves[*count].toY = y + countC;
		scoreMoves(x, y + countC, possibleMoves, *count);
		(*count)++;
	}

	if (validMove(x, y, x - countDF, y + countDF, SW)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x - countDF;
		possibleMoves[*count].toY = y + countDF;
		scoreMoves(x - countDF, y + countDF, possibleMoves, *count);
		(*count)++;
	}

	if (validMove(x, y, x - countR, y, W)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x - countR;
		possibleMoves[*count].toY = y;
		scoreMoves(x - countR, y, possibleMoves, *count);
		(*count)++;
	}

	if (validMove(x, y, x - countDB, y - countDB, NW)) {
		possibleMoves[*count].fromX = x;
		possibleMoves[*count].fromY = y;
		possibleMoves[*count].toX = x - countDB;
		possibleMoves[*count].toY = y - countDB;
		scoreMoves(x - countDB, y - countDB, possibleMoves, *count);
		(*count)++;
	}

//	printf("count: %d\n", *count);
}

int evaluateMove(gameMove possibleMoves[8*8], int which) {
	int score = 0;

	score = possibleMoves[which].score;

	if ((nextMoveNum >= 30) && (nextMoveNum <=40)) {
		if (*hisCount > 6)
			if (possibleMoves[which].capture)
				score += 2;
	} else if (nextMoveNum >= 40) {
		if (*hisCount <= 3) {
			if (nextMoveNum >= 45)
				if (possibleMoves[which].capture)
					score += 10;
		} else {
			if (possibleMoves[which].capture)
				score += 10;
		}
	}

	return score;
}

void selectMove() {
	// Come up with a move for us, and put it into the finalMove global variable

	gameMove possibleMoves[8*8];

	int nextPossible = 0;

	int topScore = -1;
	int topIndex = 0;
	int newScore;

	// Generate the moves list

	int i;

	for (i = 0; i < *ourCount; i++)
		movesFrom(myPiecesX[i], myPiecesY[i], possibleMoves, &nextPossible);

	// Now select the best move

	if (DEBUG)
		printf("\nFound %d possible moves:\n\n", nextPossible);

	for (i = 0; i < nextPossible; i++) {
		newScore = evaluateMove(possibleMoves, i);
		if (newScore > topScore) {
			topScore = newScore;
			topIndex = i;
//			printf("Top: %d,%d to %d, %d - %d points\n", possibleMoves[i].fromX, possibleMoves[i].fromY,
//												 possibleMoves[i].toX, possibleMoves[i].toY,
//												 possibleMoves[i].score);
		} else if (newScore == topScore) {
			if ((float) rand() / RAND_MAX > (2.0 / 3.0)) {	// 2/3ds chance of replace
				topIndex = i;
			}
		}

		if (DEBUG) {
			printf("\t%d,%d to %d, %d - %d points - %d rank", possibleMoves[i].fromX, possibleMoves[i].fromY,
													 possibleMoves[i].toX, possibleMoves[i].toY,
													 possibleMoves[i].score, newScore);
			if (possibleMoves[i].capture)
				printf(", capture\n");
			else
				printf("\n");
		}
	}

	if (DEBUG)
		printf("\n");

	// Chose the identified move

	finalMove.fromX = possibleMoves[topIndex].fromX;
	finalMove.fromY = possibleMoves[topIndex].fromY;
	finalMove.toX = possibleMoves[topIndex].toX;
	finalMove.toY = possibleMoves[topIndex].toY;
}

int piecesOnDiagonal(int x, int y, int direction, int kind) {
	int count = 0;
	int i;

	int startX = 0;
	int startY = 0;

	int pieceThere;

	if (direction == BACKSLASH_DIRECTION) {
		// Backward slash
		if (x == y) {
			startX = 0;
			startY = 0;
		} else {
			if (x - y > 0) {	// Above the X=Y line
				startX = x - y;
				startY = 0;
			} else {			// Under the X=Y line
				startX = 0;
				startY = y - x;
			}
		}

		for (i = 0; i < WIDTH; i++) {
			if ((startX + i >= WIDTH) || (startY + i >= HEIGHT))
				break;

			pieceThere = gameBoard[startX + i][startY + i];

			if (pieceThere != EMPTY) {
				if (kind == BOTH) {
					count++;
				} else {
					if (kind == pieceThere) {
						count++;
					}
				}
			}
		}

	} else {
		// Forward slash
		if (x == 6 - y) {
			startX = 0;
			startY = 6;
		} else {
			if (x + y <= 5) {	// Before the main diagonal line
				startY = x + y;
				startX = 0;
			} else {			// After the main diagonal line
				startY = 6;
				startX = x + y - 6;
			}
		}

		for (i = 0; i < WIDTH; i++) {
			if ((startX + i >= WIDTH) || (startY - i < 0))
				break;

			pieceThere = gameBoard[startX + i][startY - i];

			if (pieceThere != EMPTY) {
				if (kind == BOTH) {
					count++;
				} else {
					if (kind == pieceThere) {
						count++;
					}
				}
			}
		}
	}

	return count;
}

int piecesOnRow(int row, int kind) {

	int count = 0;
	int i;

	if ((rowCounts[row] != -1) && (kind == BOTH))
		return rowCounts[row];

	for (i = 0; i < WIDTH; i++)
		if (gameBoard[i][row] != EMPTY)
			if (kind == BOTH)
				count++;
			else
				if (kind == gameBoard[i][row])
					count++;

	if (kind == BOTH)
		rowCounts[row] = count;

	return count;
}

int piecesOnColumn(int column, int kind) {

	int count = 0;
	int i;

	if ((columnCounts[column] != -1) && (kind == BOTH))
		return columnCounts[column];

	for (i = 0; i < HEIGHT; i++)
		if (gameBoard[column][i] != EMPTY)
			if (kind == BOTH)
				count++;
			else
				if (kind == gameBoard[column][i])
					count++;

	if (kind == BOTH)
		columnCounts[column] = count;

	return count;
}

int sevenPointSquare(int x, int y) {
	if ((x == sevenPointSquareCoord) && (y == sevenPointSquareCoord))
		return 1;
	else
		return 0;
}

int threePointSquare(int x, int y) {
	if (x == threePointSquareCoordOne) {
		if (y == threePointSquareCoordOne)
			return 1;
		else if (y == threePointSquareCoordTwo)
			return 1;
		else
			return 0;
	} else if (x == threePointSquareCoordTwo) {
		if (y == threePointSquareCoordOne)
			return 1;
		else if (y == threePointSquareCoordTwo)
			return 1;
		else
			return 0;		
	} else {
		return 0;
	}
}

int main(int argc, char** argv) {

	FILE *inputFile = NULL;

	char buffer[80];

	char fromXChar, toXChar;
	char fromYChar, toYChar;

	int got;

	int x, y;

	// Make sure we have arguments

	if (argc != 2) {
		printf("Error: bad command line arguments. Please call as:\n");

		if (argc >= 1) {
			printf("\t%s /path/to/input\n", argv[0]);
		} else {
			printf("\t/path/to/program /path/to/input\n");
		}

		exit(1);
	}

	// Step two, open the input file

	inputFile = fopen(argv[1], "r");

	if (inputFile == NULL) {
		printf("Unable to open input file: '%s'", argv[1]);
		printf("Error number %d.\n", ferror(inputFile));
		exit(1);
	}

	finalMove.fromX = 0;
	finalMove.toX = 0;
	finalMove.fromY = 0;
	finalMove.toY = 0;

	fgets(buffer, 80, inputFile);

	got = sscanf(buffer, "%d %d", &me, &nextMoveNum);	// Read in the first line

	if (DEBUG)
		printf("I'm player %d, turn is %d.\n", me, nextMoveNum);

	if (got != 2) {
		printf("Error reading in first line of input.\n");
		exit(1);
	}

	fgets(buffer, 80, inputFile);

	got = sscanf(buffer, "1 %d %lf", &playerOneScore, &playerOneTimeLeft);	// Read in player info

	if (DEBUG)
		printf("Player 1 has %d points and %lf seconds.\n", playerOneScore, playerOneTimeLeft);

	if (got != 2) {
		printf("Error reading in second line of input.\n");
		exit(1);
	}

	fgets(buffer, 80, inputFile);

	got = sscanf(buffer, "2 %d %lf", &playerTwoScore, &playerTwoTimeLeft);

	if (DEBUG)
		printf("Player 2 has %d points and %lf seconds.\n", playerTwoScore, playerTwoTimeLeft);

	if (got != 2) {
		printf("Error reading in third line of input.\n");
		exit(1);
	}

	buffer[7] = '\0';

	for (y = 0; y < HEIGHT; y++) {
		if (!fgets(buffer, 80, inputFile)) {	// Get the next line
			printf("Error reading board line %d from the file: '%s'.\n", y + 1, buffer);
			printf("Error number %d.\n", ferror(inputFile));
			exit(1);
		}

		for (x = 0; x < WIDTH; x++) {
			switch(buffer[x]) {
				case '.':
					gameBoard[x][y] = EMPTY;
					break;
				case '1':
					gameBoard[x][y] = PLAYER_ONE;
					if (me == PLAYER_ONE) {
						if (DEBUG)
							printf("Found my piece at '1', %d, %d.\n", x, y);
						myPiecesX[playerOneCount] = x;
						myPiecesY[playerOneCount] = y;
					}
					playerOneCount++;
					break;
				case '2':
					gameBoard[x][y] = PLAYER_TWO;
					if (me == PLAYER_TWO) {
						if (DEBUG)
							printf("Found my piece '2' at %d, %d.\n", x, y);
						myPiecesX[playerTwoCount] = x;
						myPiecesY[playerTwoCount] = y;
					}
					playerTwoCount++;
					break;				
				default:
					printf("Error understanding board line %d: '%s'.\n", y + 1, buffer);
					printf("Character %d is '%c'.\n", x + 1, buffer[x]);
					exit(1);
			}
		}
	}

	if (DEBUG) {
//		printBoard();
	}

	// That takes care of all input, so close the file.

	fclose(inputFile);

	// Set some quick stuff up

	if (me == 1) {
		him = 2;
		ourScore = &playerOneScore;
		ourTime = &playerOneTimeLeft;
		ourCount = &playerOneCount;
		hisScore = &playerTwoScore;
		hisTime = &playerTwoTimeLeft;
		hisCount = &playerTwoCount;
	} else {
		him = 1;
		hisScore = &playerOneScore;
		hisTime = &playerOneTimeLeft;
		hisCount = &playerOneCount;
		ourScore = &playerTwoScore;
		ourTime = &playerTwoTimeLeft;	
		ourCount = &playerTwoCount;
	}

	if (DEBUG) {
//		printBoard();
	}

	// Seed the RNG

	srand((unsigned) time(NULL));

	// Time to start processing.

	selectMove();	// Figure out our move

	// Print out the move

	fromXChar = 'a' + (char) finalMove.fromX;
	toXChar = 'a' + (char) finalMove.toX;

	fromYChar = '7' - finalMove.fromY;
	toYChar = '7' - finalMove.toY;

	printf("%c%c %c%c\n", fromXChar, fromYChar, toXChar, toYChar);

	if (DEBUG) {
//		printBoard();
	}

	return 0;

}