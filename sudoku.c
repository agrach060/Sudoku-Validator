// Sudoku puzzle verifier and solver
// compile: gcc -Wall -Wextra -pthread -o sudoku sudoku.c -lm
// run (verify): ./sudoku puzzle2-valid.txt
// run (hard): ./sudoku puzzle9-difficult.txt
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* structure for passing data to threads */
typedef struct {
  int row;
  int column;
  int result; 
  int sudokuSize;
  int **grid;
  bool isValid;
  bool isComplete;
} parameters;

void *isValidRow(void *args) {
  parameters *params = (parameters *)args;
  int sudokuSize = params->sudokuSize;
  int **grid = params->grid;

  // rows
  for (int row = 1; row <= sudokuSize; row++) {
    bool seen[sudokuSize];
    for (int j = 1; j <= sudokuSize; j++) {
      seen[j] = false;
    }

    for (int col = 1; col <= sudokuSize; col++) {
      int num = grid[row][col];
      if (num < 1 || num > sudokuSize || seen[num]) {
        params->isValid = false;
        pthread_exit(NULL);
      }
      seen[num] = true;
    }
  }
  params->isValid = true;
  pthread_exit(NULL);
}

void *isValidColumn(void *args) {
  parameters *params = (parameters *)args;
  int sudokuSize = params->sudokuSize;
  int **grid = params->grid;

  // columns
  for (int col = 1; col <= sudokuSize; col++) {
    bool seen[sudokuSize];
    for (int j = 1; j <= sudokuSize; j++) {
      seen[j] = false;
    }

    for (int row = 1; row <= sudokuSize; row++) {
      int num = grid[row][col];
      if (num < 1 || num > sudokuSize || seen[num]) {
        params->isValid = false;
        pthread_exit(NULL);
      }
      seen[num] = true;
    }
  }
  params->isValid = true;
  pthread_exit(NULL);
}

void *isValidSubGrid(void *args) {
  parameters *params = (parameters *)args;
  int sudokuSize = params->sudokuSize;
  int **grid = params->grid;
  // size of each subgrid
  int subgridSize = sqrt(sudokuSize);

  // iterate over each subgrid
  for (int startRow = 1; startRow <= sudokuSize; startRow += subgridSize) {
    for (int startCol = 1; startCol <= sudokuSize; startCol += subgridSize) {
      // initialize an array to keep track of seen numbers
      bool seen[sudokuSize];
      // initialize all elements of seen to false
      for (int j = 1; j <= sudokuSize; j++) {
        seen[j] = false;
      }

      // iterate over each cell in the subgrid
      for (int row = startRow; row <= startRow + subgridSize - 1; row++) {
        for (int col = startCol; col <= startCol + subgridSize - 1; col++) {
          int num = grid[row][col];
          if (num < 1 || num > sudokuSize || seen[num]) {
            params->isValid = false;
            pthread_exit(NULL);
          }
          seen[num] = true;
        }
      }
    }
  }
  params->isValid = true;
  pthread_exit(NULL);
}

void *isComplete(void *args) {
  parameters *params = (parameters *)args;
  int sudokuSize = params->sudokuSize;
  int **grid = params->grid;

  for (int row = 1; row <= sudokuSize; row++) {
    for (int col = 1; col <= sudokuSize; col++) {
      int num = grid[row][col];
      if (num == 0) {
        params->isComplete = false;
        pthread_exit(NULL);
      }
    }
  }
  params->isComplete = true;
  pthread_exit(NULL);
}

void *completion(void *args) {
  parameters *params = (parameters *)args;
  int sudokuSize = params->sudokuSize;
  int **grid = params->grid;

  bool hasZero = true;

  while (hasZero) {
    hasZero = false;
    for (int row = 1; row <= sudokuSize; row++) {
      for (int col = 1; col <= sudokuSize; col++) {
        if (grid[row][col] == 0) {
          int possibleNumbers[sudokuSize + 1];
          for (int i = 1; i <= sudokuSize; i++) {
            possibleNumbers[i] = 0;
          }

          // checking row
          for (int j = 1; j <= sudokuSize; j++) {
            possibleNumbers[grid[row][j]] = 1;
          }

          // checking column
          for (int z = 1; z <= sudokuSize; z++) {
            possibleNumbers[grid[z][col]] = 1;
          }

          // checking subgrid
          int subgridSize = sqrt(sudokuSize);
          int startRow = (row - 1) / subgridSize * subgridSize + 1;
          int startCol = (col - 1) / subgridSize * subgridSize + 1;
          for (int r = startRow; r < startRow + subgridSize; r++) {
            for (int c = startCol; c < startCol + subgridSize; c++) {
              possibleNumbers[grid[r][c]] = 1;
            }
          }

          int count = 0;
          int lastPossibility = 0;
          for (int i = 1; i <= sudokuSize; i++) {
            if (!possibleNumbers[i]) {
              count++;
              lastPossibility = i;
            }
          }

          if (count == 1) {
            grid[row][col] = lastPossibility;
          } else {
            hasZero = true;
          }
        }
      }
    }
  }

  params->isComplete = true;
  pthread_exit(NULL);
}

// takes puzzle size and grid[][] representing sudoku puzzle
// and two booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9]9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize For incomplete puzzles, we cannot say anything about validity
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid) {
  // YOUR CODE GOES HERE and in HELPER FUNCTIONS
  parameters params;
  params.sudokuSize = psize;
  params.grid = grid;

  // thread ID
  pthread_t thread_complete, thread_rows, thread_columns, thread_subGrids;
  // attributes
  pthread_attr_t attr;
  // set default attributes
  pthread_attr_init(&attr);
  pthread_create(&thread_complete, &attr, isComplete, (void *)&params);

  // thread to validate rows
  pthread_create(&thread_rows, &attr, isValidRow, (void *)&params);

  // thread to validate columns
  pthread_create(&thread_columns, &attr, isValidColumn, (void *)&params);

  // threads to validate subgrids
  for (int i = 0; i < psize; i++) {
    pthread_create(&thread_subGrids, &attr, isValidSubGrid, (void *)&params);
  }

  // join threads
  pthread_join(thread_complete, NULL);
  pthread_join(thread_rows, NULL);
  for (int i = 0; i < psize; i++) {
    pthread_join(thread_subGrids, NULL);
  }

  *valid = params.isValid;
  *complete = params.isComplete;
}

// takes filename and pointer to grid[][]
// returns size of Sudoku puzzle and fills grid
int readSudokuPuzzle(char *filename, int ***grid) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Could not open file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  int psize;

  fscanf(fp, "%d", &psize);
  int **agrid = (int **)malloc((psize + 1) * sizeof(int *));
  for (int row = 1; row <= psize; row++) {
    agrid[row] = (int *)malloc((psize + 1) * sizeof(int));
    for (int col = 1; col <= psize; col++) {
      fscanf(fp, "%d", &agrid[row][col]);
    }
  }
  fclose(fp);
  *grid = agrid;
  return psize;
}

// takes puzzle size and grid[][]
// prints the puzzle
void printSudokuPuzzle(int psize, int **grid) {
  printf("%d\n", psize);
  for (int row = 1; row <= psize; row++) {
    for (int col = 1; col <= psize; col++) {
      printf("%d ", grid[row][col]);
    }
    printf("\n");
  }
  printf("\n");
}

// takes puzzle size and grid[][]
// frees the memory allocated
void deleteSudokuPuzzle(int psize, int **grid) {
  for (int row = 1; row <= psize; row++) {
    free(grid[row]);
  }
  free(grid);
}

// if sudoku is not complete, complete it
void completeSudoku(int psize, int **grid) {
  parameters params;
  params.sudokuSize = psize;
  params.grid = grid;

  // thread ID
  pthread_t thread_completeSudoku;
  // attributes
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  // thread to complete sudoku
  pthread_create(&thread_completeSudoku, &attr, completion, (void *)&params);
  printf("Completing the puzzle...\n");
  pthread_join(thread_completeSudoku, NULL);
}

// expects file name of the puzzle as argument in command line
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: ./sudoku puzzle.txt\n");
    return EXIT_FAILURE;
  }
  // grid is a 2D array
  int **grid = NULL;
  // find grid size and fill grid
  int sudokuSize = readSudokuPuzzle(argv[1], &grid);
  bool valid = false;
  bool complete = false;
  checkPuzzle(sudokuSize, grid, &complete, &valid);
  printf("Complete puzzle? ");
  printf(complete ? "true\n" : "false\n");
  if (complete) {
    printf("Valid puzzle? ");
    printf(valid ? "true\n" : "false\n");
  } else {
    completeSudoku(sudokuSize, grid);
    checkPuzzle(sudokuSize, grid, &complete, &valid);
    printf("Complete puzzle? ");
    printf(complete ? "true\n" : "false\n");
    printf("Valid puzzle? ");
    printf(valid ? "true\n" : "false\n");
  }
  printSudokuPuzzle(sudokuSize, grid);
  deleteSudokuPuzzle(sudokuSize, grid);
  return EXIT_SUCCESS;
}
