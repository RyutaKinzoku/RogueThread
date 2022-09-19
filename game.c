#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
//#include <graphics.h>

// Test function ----------------------------------------------------------------------------
int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}
// ------------------------------------------------------------------------------------------

int n;
int map[10][10] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 2, 3, 3, 0, 0, 0, 0, 0}, 
    {0, 0, 3, 0, 3, 0, 0, 0, 0, 0},
    {0, 0, 3, 3, 3, 3, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

int entityMap[10][10] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

int heroPosition[2] = {5, 5};
pthread_cond_t command_condition;
pthread_mutex_t command_mutex, mutex_entitys;
int heroHealth = 5;
int heroAD = 1; // Hero attack damage
char command = 0;

int *monsterHealths;

struct args {
  int id;
  int attack;
  int pos[2];
};

// This thread works as a listener of keyboard events
void* catchKeyEvent(void* data){
  while(heroHealth>0){
    pthread_mutex_lock(&command_mutex);
    if(kbhit())
    {
        command = getchar(); //Fill the command buffer
        pthread_cond_signal(&command_condition); //Send a signal to inform the thread of changing the state of the hero that a key has just pressed
    }
    pthread_mutex_unlock(&command_mutex);
    sleep(0.1); //This line avoid active waiting
  }
  pthread_exit(NULL);
}

// This thread applies user commads to hero state
void* changeHeroState(void* data){
  pthread_t catchEvents;
  pthread_create(&catchEvents, NULL, &catchKeyEvent, NULL); // Create a listener thread

  while(heroHealth>0){
    pthread_cond_wait(&command_condition, &command_mutex); // Wait until the listener thread send a signal

    //The different commands of the game
    if(command=='w'){
        printf("Go up!\n");
        if(map[heroPosition[0]-1][heroPosition[1]] != 0 && heroPosition[0] > 0 && entityMap[heroPosition[0]-1][heroPosition[1]] == 0){
          heroPosition[0] -= 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&mutex_entitys);
            heroHealth--;// Fall in a trap
            pthread_mutex_unlock(&mutex_entitys);
            map[heroPosition[0]][heroPosition[1]] = 3;
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command=='a'){
        printf("Go left!\n");
        if(map[heroPosition[0]][heroPosition[1]-1] != 0 && heroPosition[1] > 0 && entityMap[heroPosition[0]][heroPosition[1]-1] == 0){
          heroPosition[1] -= 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&mutex_entitys);
            heroHealth--;// Fall in a trap
            pthread_mutex_unlock(&mutex_entitys);
            map[heroPosition[0]][heroPosition[1]] = 3;
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command=='s'){
        printf("Go down!\n");
        if(map[heroPosition[0]+1][heroPosition[1]] != 0 && heroPosition[0] < n-1 && entityMap[heroPosition[0]+1][heroPosition[1]] == 0){
          heroPosition[0] += 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&mutex_entitys);
            heroHealth--;// Fall in a trap
            pthread_mutex_unlock(&mutex_entitys);
            map[heroPosition[0]][heroPosition[1]] = 3;
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command=='d'){
        printf("Go right!\n");
        if(map[heroPosition[0]][heroPosition[1]+1] != 0 && heroPosition[1] < n-1 && entityMap[heroPosition[0]][heroPosition[1]+1] == 0){
          heroPosition[1] += 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&mutex_entitys);
            heroHealth--;// Fall in a trap
            pthread_mutex_unlock(&mutex_entitys);
            map[heroPosition[0]][heroPosition[1]] = 3;
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command==' '){
        if(entityMap[heroPosition[0]][heroPosition[1]] > 0){
          printf("Attack monster!\n");
          monsterHealths[entityMap[heroPosition[0]][heroPosition[1]]-1] -= heroAD;
        }
    } else if(command=='e'){
        if(entityMap[heroPosition[0]][heroPosition[1]] == 5){
          printf("Open a treasure!\n");
          unsigned int randval;
          FILE *f = fopen("/dev/random", "r");
          fread(&randval, sizeof(randval), 1, f);
          fclose(f);
          int ranNum = randval % 1;
          if(ranNum){
            heroAD++; //Boost one point of the hero attack damage
          } else {
            pthread_mutex_lock(&mutex_entitys);
            heroHealth++;// Heal one health point
            pthread_mutex_unlock(&mutex_entitys);
          }
        }
    }
  }

  pthread_join(catchEvents, NULL); //Join with the listener thread
  pthread_exit(NULL);
}

void *monsterCycle(void *data) {
  struct args *info = data;
  // printf("monster in pos %d, %d\n", info->pos[0], info->pos[1]);
  while (monsterHealths[info->id] > 0) {
    unsigned int randval;
    FILE *f = fopen("/dev/random", "r");
    fread(&randval, sizeof(randval), 1, f);
    fclose(f);
    int ranNum = randval % 400000 + 100000;
    printf("monster %d\n\tpos %d, %d\n\thealth %d \n", info->id, info->pos[0], info->pos[1], monsterHealths[info->id]);
    if (info->pos[0] == heroPosition[0] && info->pos[1] == heroPosition[1]) { // If player is in the same room
      pthread_mutex_lock(&mutex_entitys);
      heroHealth--;// Attack player
      pthread_mutex_unlock(&mutex_entitys);
    } 
    else { // If the monster is alone
      int futurePos[4], overflow[4];
      int j;
      pthread_mutex_lock(&mutex_entitys); // Lock the new monster position
      for (int i = 0; i < 4; i++) {
        j = i < 2; // To jump between rows and columns cheking the 4 neighbors cells
        // future positions
        /*
          futurePos[0] left
          futurePos[1] right
          futurePos[2] up
          futurePos[3] down
        */
        if (i % 2) {
          futurePos[i] = info->pos[j] + 1;
        } else {
          futurePos[i] = info->pos[j] - 1;
        }
        // Check if it's out of the matrix
        overflow[i] = (futurePos[i] > 9 || futurePos[i] < 0);
        // Check if there's a monster in the cell or if it is a wall, start or
        // end cell
        if (overflow[i] == 0) {
          if (j) { // i < 2
            // Check left & right
            overflow[i] = entityMap[info->pos[0]][futurePos[i]] > 0 ||
                          map[info->pos[0]][futurePos[i]] < 3;
          } else {
            // Check up & down
            overflow[i] = entityMap[futurePos[i]][info->pos[1]] > 0 ||
                          map[futurePos[i]][info->pos[1]] < 3;
          }
        }
      }
      // Select any neighbor cell
      int cell = rand() % 4;
      //If it can move, then do it
      if(!(overflow[0]==overflow[1]==overflow[2]==overflow[3]==1)){
        // If it has any problem select one that does not have it
        while (overflow[cell] == 1) {
          cell = rand() % 4;
        }
        if (cell < 2) { // left & right - change column
          entityMap[info->pos[0]][info->pos[1]] = 0;
          info->pos[1] = futurePos[cell];
          entityMap[info->pos[0]][info->pos[1]] = info->id+1;
        } else { // up & down - change row
          entityMap[info->pos[0]][info->pos[1]] = 0;
          info->pos[0] = futurePos[cell];
          entityMap[info->pos[0]][info->pos[1]] = info->id+1;
        }
      }
      pthread_mutex_unlock(&mutex_entitys);
      printf("NEW monster %d\n\tpos %d, %d\n\thealth %d \n", info->id,
             info->pos[0], info->pos[1], monsterHealths[info->id]);
    }
    //usleep MUST be a random between 100000 and 500000 microseconds
    usleep(ranNum);             // 100000 - 500000 microseconds = 0.1 - 0.5 seconds
  }
  pthread_mutex_lock(&mutex_entitys);
  entityMap[info->pos[0]][info->pos[1]] = 0;
  pthread_mutex_unlock(&mutex_entitys);
  pthread_exit(NULL);
}

int main(void) {
  n = 10;//It MUST be changed for the user input
  int nMonsters = floor(n / 2);
  int monsterHLocal[n]; // Allocate memory to the global monsterHealths array
  monsterHealths = monsterHLocal;
  pthread_t monsters[nMonsters];
  pthread_mutex_init(&mutex_entitys, NULL);

  // Initialize monsters
  for (int i = 0; i < nMonsters; i++) {
    // init monster
    struct args *monster = (struct args *)malloc(sizeof(struct args));
    monster->id = i;
    monster->attack = 1;
    monsterHLocal[i] = 3;//3 is the original health value
    // Select a cell for it
    int j = rand() % 10;
    int k = rand() % 10;
    // If the selected cell isn't a room or already has a monster, change it
    while (map[j][k] < 3 || entityMap[j][k] > 0) {
      j = rand() % 10;
      k = rand() % 10;
    }
    entityMap[j][k] = i + 1; // Monster starting in that room
    monster->pos[0] = j;
    monster->pos[1] = k;
    pthread_create(&monsters[i], NULL, &monsterCycle, (void *)monster);
  }

  // Hero part
  //Initialize mutexes and conditions
  pthread_mutex_init(&command_mutex, NULL);
  pthread_cond_init(&command_condition, NULL);
  pthread_t chHeroState;

  pthread_create(&chHeroState, NULL, &changeHeroState, NULL);
  
  pthread_join(chHeroState, NULL);
  for (int i = 0; i < nMonsters; i++) {
    pthread_join(monsters[i], NULL);
  }
  //Destroy mutexes and conditions
  pthread_mutex_destroy(&command_mutex);
  pthread_cond_destroy(&command_condition);
  pthread_mutex_destroy(&mutex_entitys);
  return 0;
}