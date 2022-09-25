#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <graphics.h>

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
int map[30][30];

int entityMap[30][30];

int heroPosition[2] = {5, 5};
pthread_cond_t command_condition;
pthread_mutex_t command_mutex, health_mutex;
pthread_mutex_t *cells_mutex;

int heroHealth = 5;
int heroAD = 1; // Hero attack damage
char command = 0;
int runState = 0;

int *monsterHealths;
int *monsterStatus;

struct args {
  int id;
  int pos[2];
};

int rooms[30][2];
//int map[30][30];

// This function chooses what to place in any given room
int place(int m){
  if (m == 1){
   return 2; 
  }
  int room = (rand() % (11));
  if (room <= 3){
    return 5;
  }else if (3 < room && room <= 7){
    return 4;
  }else if (7 < room && room <= 10){
    return 3;
  } 
  return 3;
}

// This function determines if adjacent rooms can afford to have a new adjacent room
bool check2(int i, int j){
  int available = 0;
  if(i <= (n-1) && j <= (n-1) && i >= 0 && j >= 0){ //check if the coordinates are in the matrix   
    if(i+1 <= (n-1)){
      if(map[i+1][j] == 0){ //check if room is a wall
        available++;
      }
    }
    if(i-1 >= 0){
      if(map[i-1][j] == 0){
        available++;
      }
    }
    if(j+1 <= n){
      if(map[i][j+1] == 0){
        available++;
      }
    }
    if(j-1 >= 0){
      if(map[i][j-1] == 0){
        available++;
      }
    }
    if (available > 2){
      return true;
    }
    return false;
  }else{
    return false;
  }
}

// This function determines if placing a new room in the given coordinates is viable
bool check(int i, int j){
  int available = 0;
  bool secondCheck = false;
  if(i <= (n-1) && j <= (n-1) && i >= 0 && j >= 0){    
    if(i+1 <= (n-1)){
      secondCheck = check2(i+1, j);
      if(map[i+1][j] == 0 && secondCheck){
        available++;
      }
    }
    if(i-1 >= 0){
      secondCheck = check2(i-1, j);
      if(map[i-1][j] == 0 && secondCheck){
        available++;
      }
    }
    if(j+1 <= n){
      secondCheck = check2(i, j+1);
      if(map[i][j+1] == 0 && secondCheck){
        available++;
      }
    }
    if(j-1 >= 0){
      secondCheck = check2(i, j-1);
      if(map[i][j-1] == 0 && secondCheck){
        available++;
      }
    }
    if (available > 1){
      return true;
    }
    return false;
  }else{
    return false;
  }
}

// This function chooses where to branch off a new room
int choose(int quantity){
  bool viable = false;
  int room = 0;
  while(!viable){ //repeat until vible room is found
    room = (rand() % (quantity - 0 + 1));
    /*for(int i = 0; i < n; i++){
      for(int j = 0; j < n; j++){
        if(map[i][j] != 0){
          printf("%d\t",map[i][j]); 
        }else{
          printf(" \t"); 
        }
      }
      printf("\n");
    }
    sleep(1);*/
    viable = check(rooms[room][0], rooms[room][1]); //check if the room is viable
  }
  return room;
}

// This recursive function takes care of calling the necessary functions to put n rooms in the map
void fill(int i, int j,int m){
  bool placed = false;
  int room = 0;
  if(m > 0){
    while(!placed){
      room = (rand() % (4))+1;
      
      if(room==1 && map[i+1][j] == 0){
        placed = check(i+1, j);
        if (placed){
          map[i+1][j] = place(m);
          i++;
        }
      }
      if (room==2 && map[i-1][j] == 0){
        placed = check(i-1, j);
        if (placed){
          map[i-1][j] = place(m);
          i--;
        }
      }
      if (room==3 && map[i][j+1] == 0){
        placed = check(i, j+1);
        if (placed){
          map[i][j+1] = place(m);
          j++;
        }
      }
      if (room==4 && map[i][j-1] == 0){
        placed = check(i, j-1);
        if (placed){
          map[i][j-1] = place(m);
          j--;
        }
      }
      sleep(0);
    }
  m--;
  rooms[(n-1)-m][0] = i;
  rooms[(n-1)-m][1] = j; // add a new room to the list
  int chosenRoom = choose((n-1)-m);
  fill(rooms[chosenRoom][0],rooms[chosenRoom][1],m); // choose a new room to branch off a new path
  }else{
    return;
  }
}

// This thread works as a listener of keyboard events
void* catchKeyEvent(void* data){
  while(heroHealth>0 && runState == 0){ //Run state
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

  while(heroHealth>0 && runState == 0){
    pthread_cond_wait(&command_condition, &command_mutex); // Wait until the listener thread send a signal
    //The different commands of the game
    if(command=='w'){
        printf("Go up!\n");
        if(map[heroPosition[0]-1][heroPosition[1]] != 0 && heroPosition[0] > 0 && entityMap[heroPosition[0]-1][heroPosition[1]] == 0){
          heroPosition[0] -= 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&health_mutex);
            heroHealth--;// Fall in a trap
            if(heroHealth <= 0){
              runState = 2; // Defeat runState Value
            } 
            pthread_mutex_unlock(&health_mutex);
            map[heroPosition[0]][heroPosition[1]] = 3;
          } else if(map[heroPosition[0]][heroPosition[1]] == 2){
            runState = 1; // Victory runState value
            pthread_exit(NULL);
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command=='a'){
        printf("Go left!\n");
        if(map[heroPosition[0]][heroPosition[1]-1] != 0 && heroPosition[1] > 0 && entityMap[heroPosition[0]][heroPosition[1]-1] == 0){
          heroPosition[1] -= 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&health_mutex);
            heroHealth--;// Fall in a trap
            if(heroHealth <= 0){
              runState = 2; // Defeat runState Value
            } 
            pthread_mutex_unlock(&health_mutex);
            map[heroPosition[0]][heroPosition[1]] = 3;
          } else if(map[heroPosition[0]][heroPosition[1]] == 2){
            runState = 1; // Victory runState value
            pthread_exit(NULL);
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command=='s'){
        printf("Go down!\n");
        if(map[heroPosition[0]+1][heroPosition[1]] != 0 && heroPosition[0] < n-1 && entityMap[heroPosition[0]+1][heroPosition[1]] == 0){
          heroPosition[0] += 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&health_mutex);
            heroHealth--;// Fall in a trap
            if(heroHealth <= 0){
              runState = 2; // Defeat runState Value
            } 
            pthread_mutex_unlock(&health_mutex);
            map[heroPosition[0]][heroPosition[1]] = 3;
          } else if(map[heroPosition[0]][heroPosition[1]] == 2){
            runState = 1; // Victory runState value
            pthread_exit(NULL);
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command=='d'){
        printf("Go right!\n");
        if(map[heroPosition[0]][heroPosition[1]+1] != 0 && heroPosition[1] < n-1 && entityMap[heroPosition[0]][heroPosition[1]+1] == 0){
          heroPosition[1] += 1;
          if(map[heroPosition[0]][heroPosition[1]] == 4){
            pthread_mutex_lock(&health_mutex);
            heroHealth--;// Fall in a trap
            if(heroHealth <= 0){
              runState = 2; // Defeat runState Value
            } 
            pthread_mutex_unlock(&health_mutex);
            map[heroPosition[0]][heroPosition[1]] = 3;
          } else if(map[heroPosition[0]][heroPosition[1]] == 2){
            runState = 1; // Victory runState value
            pthread_exit(NULL);
          }
        }
        printf("Hero position: [%d, %d]\n", heroPosition[0], heroPosition[1]);
    } else if(command==' '){
        int entity = entityMap[heroPosition[0]][heroPosition[1]];
        if(entity > 0 && monsterStatus[entity-1] == 0){
          printf("Attack monster!\n");
          monsterHealths[entity-1] -= heroAD;
        }
    } else if(command=='e'){
        if(map[heroPosition[0]][heroPosition[1]] == 5){
          printf("Open a treasure!\n");
          if(rand() % (2)){
            heroAD++; //Boost one point of the hero attack damage
          } else {
            pthread_mutex_lock(&health_mutex);
            heroHealth++;// Heal one health point
            pthread_mutex_unlock(&health_mutex);
          }
          map[heroPosition[0]][heroPosition[1]] = 3;
        }
    }
  }

  pthread_join(catchEvents, NULL); //Join with the listener thread
  pthread_exit(NULL);
}

void *monsterCycle(void *data) {
  struct args *info = data;
  // printf("monster in pos %d, %d\n", info->pos[0], info->pos[1]);
  while (monsterHealths[info->id] > 0 && runState == 0) {
    unsigned int randval;
    FILE *f = fopen("/dev/random", "r");
    fread(&randval, sizeof(randval), 1, f);
    fclose(f);
    int ranNum = randval % 1500000 + 1000000;
    //printf("monster %d\n\tpos %d, %d\n\thealth %d \n", info->id, info->pos[0], info->pos[1], monsterHealths[info->id]);
    if (info->pos[0] == heroPosition[0] && info->pos[1] == heroPosition[1]) { // If player is in the same room
      pthread_mutex_lock(&health_mutex);
      heroHealth--;// Attack player
      if(heroHealth <= 0){
        runState = 2; // Defeat runState Value
      } 
      pthread_mutex_unlock(&health_mutex);
    } 
    else { // If the monster is alone
      // Select a cell for it
      entityMap[info->pos[0]][info->pos[1]] = 0;
      pthread_mutex_unlock(&cells_mutex[info->pos[0]*10+info->pos[1]]);//Current cell
      int i = rand() % n;
      int j = rand() % n;
      pthread_mutex_lock(&cells_mutex[i*10+j]);//Possible future
      // If the selected cell isn't a room or already has a monster, change it
      while (map[i][j] < 3) {
        pthread_mutex_unlock(&cells_mutex[i*10+j]);//Imposible cell
        i = rand() % n;
        j = rand() % n;
        pthread_mutex_lock(&cells_mutex[i*10+j]);//New Possible future
      }
      entityMap[i][j] = info->id + 1; // New pos
      info->pos[0] = i;
      info->pos[1] = j;
    }
    //usleep MUST be a random between 100000 and 500000 microseconds
    monsterStatus[info->id] = 0; //Monster waiting
    usleep(ranNum);             // 100000 - 500000 microseconds = 0.1 - 0.5 seconds
    monsterStatus[info->id] = 1; //Active monster
  }
  entityMap[info->pos[0]][info->pos[1]] = 0;
  pthread_exit(NULL);
}

int main(void) {
  //Initialize mutexes and conditions
  pthread_mutex_init(&command_mutex, NULL);
  pthread_mutex_init(&health_mutex, NULL);
  pthread_cond_init(&command_condition, NULL);
  printf("Type the mode's numer: \n1 - Easy\n2 - Medium\n3 - Hard\n");  
  scanf("%d", &n);
  if(n == 1){
    n = 10;
    printf("Map 10x10\n");
  } else if(n == 2){
    n = 20;
    printf("Map 20x20\n");
  } else if (n == 3){
    n = 30;
    printf("Map 30x30\n");
  } else{
    printf("You have to choose between 1, 2 and 3\n");
    exit(1);
  }

  /**/
  int upper = n-1;
  for(int i = 0; i < 29; i++){
    for(int j = 0; j < 29; j++){
      map[i][j] = 0;
      entityMap[i][j] = 0;
    }
  }
  printf("CEROS");
  
  srand(time(0));
  int i = (rand() % (upper - 0 + 1));
  int j = (rand() % (upper - 0 + 1)); //choose the starting room
  map[i][j] = 1;
  heroPosition[0] = i;
  heroPosition[1] = j;
  rooms[0][0] = i;
  rooms[0][1] = j; //add first room to list
  int m = n-1;
  fill(i,j,m); //fill the map for the game
  printf("Dungeon Done\n");
  /**/

  pthread_mutex_t cells_mutex_local[n*n]; // Allocate memory to the global cells_mutex array
  cells_mutex = cells_mutex_local;
  int nMonsters = floor(n / 2);
  int monsterHLocal[n]; // Allocate memory to the global monsterHealths array
  monsterHealths = monsterHLocal;
  int monsterSLocal[n]; // Allocate memory to the global monsterStatus array
  monsterStatus = monsterSLocal;
  pthread_t monsters[nMonsters];
  for (int i = 0; i < n*n; i++) {
    pthread_mutex_init(&cells_mutex[i], NULL);
  }

  // Initialize monsters
  for (int i = 0; i < nMonsters; i++) {
    // init monster
    struct args *monster = (struct args *)malloc(sizeof(struct args));
    monster->id = i;
    monsterHLocal[i] = 3;//3 is the original health value
    monsterSLocal[i] = 1;//Monster active
    // Select a cell for it
    int j = rand() % n;
    int k = rand() % n;
    // If the selected cell isn't a room or already has a monster, change it
    while (map[j][k] < 3 || entityMap[j][k] > 0) {
      j = rand() % n;
      k = rand() % n;
    }
    entityMap[j][k] = i + 1; // Monster starting in that room
    monster->pos[0] = j;
    monster->pos[1] = k;
    pthread_mutex_lock(&cells_mutex[monster->pos[0]*10+monster->pos[1]]);//Current cell
    pthread_create(&monsters[i], NULL, &monsterCycle, (void *)monster);
  }

  // Hero part
  pthread_t chHeroState;

  pthread_create(&chHeroState, NULL, &changeHeroState, NULL);

  //Rendering cycle
  /*
  XInitThreads();
  int gd = DETECT, gm;
  initgraph(&gd, &gm, NULL);
  while(runState == 0){
    int x = 0;
    int y = 0;
    for(int i = 0; i < n; i++) {
      for(int j = 0; j < n; j++) {
        if(map[i][j]==0){
          setcolor(15);//WHITE
          bar(12+x,12+y,22+x,22+y);
        }
        else if(map[i][j]==1){//Start room
          setcolor(15);//WHITE
          rectangle(12+x,12+y,22+x,22+y);
          outtextxy(12+x+2,12+y+3, "*");
        }
        else if(map[i][j]==2){//Final room
          setcolor(4);//RED
          bar(12+x,12+y,22+x,22+y);
        }
        else if(map[i][j]>3){
          outtextxy(12+x+2,12+y, "?");
        }
        x+=12;
      }
      x=0;
      y+=12;
    }
    delay(20);
    cleardevice();
  }
  closegraph();
  /**/
  /**/
  while(runState==0){ //Map rendering by text
    system("clear");
    printf("\n\n\t Rogue Thread \n\n");
    printf("Health %d\t Attack %d\n\n", heroHealth, heroAD);
    for(int i = 0; i < n;i++) {
        for(int j = 0; j < n;j++) {
          if(heroPosition[0]==i && heroPosition[1]==j){
            printf("*");
          }
          else if(map[i][j] == 0){ //wall
            printf("■");
          }
          else if(map[i][j] == 1){//start
            printf("⛋");
          }
          else if(entityMap[i][j] == 0 && map[i][j]==3){//empty
            printf("□");
          }
          if(entityMap[i][j] > 0){
            printf("M", entityMap[i][j]);
          }
          else if(map[i][j]>3 || map[i][j] == 2){//treasure or trap
            printf("?");
          }
          printf("    ");
        }
        printf("\n");
    }
    printf("\n");
    delay(20);
  }
  if(runState == 1){
    printf("VICTORY\n");
  }else if(runState == 2){
    printf("YOU DIED!\n");
  }

  /**/
  //Synchronize threads
  pthread_join(chHeroState, NULL);
  for (int i = 0; i < nMonsters; i++) {
    pthread_join(monsters[i], NULL);
  }

  for (int i = 0; i < n*n; i++) {
    pthread_mutex_destroy(&cells_mutex[i]);
  }

  //Destroy mutexes and conditions
  pthread_mutex_destroy(&command_mutex);
  pthread_cond_destroy(&command_condition);
  pthread_mutex_destroy(&health_mutex);
  return 0;
}