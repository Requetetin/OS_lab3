#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <string.h>
#include <pthread.h>

int sudoku[9][9];

void* checkColumns() {
  omp_set_num_threads(9);
  omp_set_nested(true);
  int col;
  int row;
  int i;
  int hasAll[9];
  #pragma omp parallel for ordered
  for (col = 0; col < 9; col++) {
    omp_set_num_threads(9);
    #pragma omp ordered
    for (i = 0; i < 9; i++) {
      hasAll[i] = 0;
    }
    omp_set_num_threads(9);
    #pragma omp ordered
    for (row = 0; row < 9; row++) {
      hasAll[array[row][col] - 1] = 1;
    }
    omp_set_num_threads(9);
    #pragma omp ordered
    for (i = 0; i < 9; i++) {
      if (hasAll[i] == 0) printf("El sudoku ha fallado en la columna %d\n", i + 1);
    }
  }
  printf("Se ha terminado la revision de las columnas\n");
  pthread_exit(0);
}

void* checkRows() {
  omp_set_num_threads(9);
  omp_set_nested(true);
  int row;
  int col;
  int i;
  int hasAll[9];
  #pragma omp parallel for ordered schedule(dynamic)
  for (row = 0; row < 9; row++) {
    omp_set_num_threads(9);
    #pragma omp ordered
    for (i = 0; i < 9; i++) {
      hasAll[i] = 0;
    }
    omp_set_num_threads(9);
    #pragma omp ordered
    for (col = 0; col < 9; col++) {
      hasAll[array[row][col] - 1] = 1;
    }
    omp_set_num_threads(9);
    #pragma omp ordered
    for (i = 0; i < 9; i++) {
      if (hasAll[i] == 0) printf("El sudoku ha fallado en la fila %d\n", i + 1);
    }
  }
  printf("Se ha terminado la revision de las filas\n");
}

void* checkSquares(int i, int j) {
  omp_set_num_threads(9);
  omp_set_nested(true);
  int a;
  int col;
  int row;
  int hasAll[9];
  #pragma omp parallel for schedule(dynamic)
  for (a = 0; a < 9; a++) {
    hasAll[a] = 0;
  }
  omp_set_num_threads(3);
  #pragma omp parallel for collapse(2) schedule(dynamic)
  for (row = 0; row < 3; row++) {
    for (col = 0; col < 3; col++) {
      hasAll[array[i * 3 + row][j * 3 + col] - 1] = 1;
    }
  }
  omp_set_num_threads(9);
  #pragma omp parallel for schedule(dynamic)
  for (a = 0; a < 9; a++) {
    if (hasAll[a] == 0) printf("Los subarreglos fallaron en %d, %d\n", i, j);
  }
  printf("Se termina la revision de subarreglos\n");
}

int main(int argc, char* argv[]) {
  omp_set_num_threads(1);
  if (argc != 2) {
    printf("Not the correct amount of arguments\n");
    return 0;
  }
  int file = open(argv[1], O_RDONLY);
  if (file < 0) {
    printf("Error opening file\n");
    return 0;
  }

  struct stat statbuf;
  int err = fstat(file, &statbuf);
  if (err < 0) {
    printf("Error opening file\n");
    return 0;
  }
  char* ptr = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, file, 0);
  if (ptr == MAP_FAILED) {
    printf("Mapping failed\n");
    return 0;
  }
  close(file);

  if (strlen(ptr) != 82) {
    printf("File format isn't correct\n");
    return 0;
  }
  int s;
  for (s = 0; s < 81; s++) {
    int a = s / 9;
    int b = s % 9;
    char c = ptr[s];
    int val = c - '0';
    sudoku[a][b] = val;
  }

  int i;
  int j;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      checkSquares(i, j);
    }
  }

  int pid = getpid();
  int fork_pid = fork();
  if (fork_pid < 0) {
    printf("Failed fork\n");
    return 0;
  } else if (fork_pid == 0) {
    char string_pid[50];
    sprintf(string_pid, "%d", pid);
    execlp("ps", "ps", "-p", string_pid, "-lLf", (char*)NULL);
  } else {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, checkColumns, NULL);
    pthread_join(thread_id, NULL);
    pid_t tid;
    tid = syscall(SYS_gettid);
    printf("Thread id: %d\n" tid);
    wait(NULL);
  }
  checkRows();

  int fork2_pid = fork();
  if (fork2_pid < 0) {
    printf("Failed fork\n");
    return 0;
  } else if (fork2_pid == 0) {
    char string_pid[50];
    sprintf(string_pid, "%d", pid);
    execlp("ps", "ps", "-p", string_pid, "-lLf", (char*)NULL);
  } else {
    wait(NULL);
  }
  return 0;
}