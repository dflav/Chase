#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BOARD_WIDTH 60 //Μήκος του χώρου (board)
#define BOARD_HEIGHT 20 //Πλάτος του χώρου (board)

struct Level {
    int current_level; //Το τρέχον επίπεδο
    int score;  //Το σκορ
    int max_score; //Το σκορ του παίκτη με το μεγαλύτερο σκορ
    char name[20];  //Το όνομα του παίκτη με το μεγαλύτερο σκορ
};
//Συντεταγμένες στον χώρο (board)
struct Position {
    int x;
    int y;
};

struct Player {
    struct Position position;  //Συντεταγμένες του παίκτη (Μ)
    int bombs; // Βόμβες που απομένουν
};

struct Robot {
    struct Position *position;  // Συντεταγμένες του κάθε ρομπότ(R)
    int robotNum;  //Αριθμός των ρομπότ του τρέχον επιπέδου
    int robots_left;   //Αριθμός των ρομπότ που απομένουν για το επόμενο επίπεδο
    int *crush_status; //Αριθμός που δείχνει την κατάσταση του ρομπότ( 0-Ζωντανό ρομπότ, 1-Συντρίμμια, 2-Νικήθηκε από βόμβα )
};

int getCord(int lower, int higher);//Επιστρέφει έναν αριθμό ανάμεσα από το 1-58
int movePlayer(WINDOW *win, char dir, struct Player *player, struct Robot robots);//Μετακινεί τον παίκτη
int moveRobots(WINDOW *win,struct Robot *robots,struct Player player,struct Level *level);//Μετακινεί τα ρομπότ
int distance(int x1,int x2,int y1,int y2);//Υπολογίζει την απόσταση από 2 σημεία
int check_obs(int x1, int x2,int y1, int y2);//Ελέγχει αν 2 σημεία έχουν τις ίδιες συντεταγμένες
void throw_bomb(struct Player *player,struct Robot *robots,struct Level *level);//Ενεργοποιεί την βόμβα
void teleport(struct Player *player,struct Robot robots); //Τηλεμεταφέρει τον παίκτη
void user_inter(struct Level level,struct Robot robots, struct Player player);//Αλλάζει τις πληροφορίες κάτω από τον ορθογώνιο χώρο
void next_level(WINDOW *win, struct Player *player,struct Robot *robots,struct Level *level);//Ενημερώνει τον ορθογώνιο χώρο για το επόμενο επίπεδο
void init_game(WINDOW *win, struct Level *level,struct Player *player, struct Robot *robots);//Ενημερώνει τον ορθογώνιο χώρο για την αρχή του παιχνιδιού

int main()
{
    int input;  // Η εντολή που θα δώσει ο παίκτης
    char dir,again='x'; // dir είναι η κατεύθυνση πού θα κινηθεί ο παίκτης, το again αν θέλει να αρχίσει νέο παιχνίδι και αρχικοποιείτε με x
    struct Player player;
    struct Robot robots;
    struct Level level;

    srand(time(NULL));

    initscr();
    keypad(stdscr,true);
    noecho();
    curs_set(0);
    //cbreak();

    printw(" Chase started! Press Q to quit...");
    level.max_score=0; // Αρχικοποίηση του μέγιστου σκορ με 0

    WINDOW *board = newwin(BOARD_HEIGHT, BOARD_WIDTH, 1, 1); //Δημιουργεί ένα παράθυρο μεγέθους 60x20
    refresh();//Ανανεώνει την οθόνη βάζοντας το νέο παράθυρο
    box(board,0,0);//Τοποθετεί ένα εξωτερικό πλαίσιο στον ορθογώνιο χώρο
    init_game(board,&level,&player,&robots);
    wrefresh(board);//Ανανεώνει το παράθυρο
    user_inter(level,robots,player);

    do{
        if(robots.robots_left==0) //Έλεγχος αν τα ζωντανά ρομπότ είναι 0 ώστε να αρχίσει το επόμενο επίπεδο
            next_level(board,&player,&robots,&level);

        wclear(board);
        box(board,0,0);
        input = getch();

        switch(input)
        {
            case KEY_UP://πάνω βελάκι
                dir='u';
               break;
            case KEY_DOWN://κάτω βελάκι
                dir='d';
               break;
            case KEY_RIGHT://δεξί βελάκι
                dir='r';
                break;
            case KEY_LEFT://αριστερό βελάκι
                dir='l';
                break;
            case KEY_HOME://home
                dir='h';
                break;
            case KEY_END://end
                dir='e';
                break;
            case KEY_NPAGE://PgDn
                dir='n';
                break;
            case KEY_PPAGE://PgUp
                dir='p';
                break;
            case '5':
                dir='s';//Το s αφήνει τον παίκτη στο σημείο που βρισκόταν
                break;
            case ' ':
                dir='s';
                break;
            case 'B':
                if(player.bombs > 0){ //Έλεγχος αν υπάρχουν βόμβες
                  throw_bomb(&player,&robots,&level);
                  user_inter(level,robots,player);
                  dir='s';
                  }else
                  dir='x';
                break;
            case 'T':
                teleport(&player,robots);
                dir='s';
                break;
            default:
                dir='x'; //Οτιδήποτε άλλο πληκτρολογήσει
                break;
        }

        if(movePlayer(board, dir, &player,robots)){     //Έλεγχος αν ο παίκτης κινήθηκε
           if(moveRobots(board,&robots,player, &level)) //Έλεγχος αν τα ρομπότ έπιασαν τον παίκτη
           {
               wrefresh(board);
               mvprintw(22,1,"You lost...\n Enter your name: ");
               echo();
               scanw("%s", level.name);

               if(level.max_score < level.score) //Έλεγχος αν το σκορ που έκανε ο παίκτης είναι μεγαλύτερο από το μέγιστο σκορ
                  level.max_score = level.score;

               user_inter(level,robots,player);

               mvprintw(24,1,"Do you want to start a new game? (y/n): "); //Εισάγει αν θέλει να παίξει ξανά
               do{
                   move(24,40);
                   again = getch();
               }while(again !='N' && again !='n' && again !='Y' && again !='y');

           }else
           wrefresh(board);
        }

        if(again == 'n' || again == 'N') //Έλεγχος αν δεν θέλει να παίξει ξανά
        {
               free(robots.position); //Αποδέσμευση μνήμης
               free(robots.crush_status); //Αποδέσμευση μνήμης
               delwin(board); //Αποδέσμευση μνήμης
               endwin();
               return 0;

        }else if(again == 'y' || again == 'Y') //Έλεγχος αν θέλει αν παίξει ξανά
        {
            noecho();
            again='x'; //Αρχικοποίηση του again με το x

            free(robots.position);
            free(robots.crush_status);
            //Δημιουργία νέο παιχνιδιού
            wclear(board);
            box(board,0,0);
            init_game(board,&level,&player,&robots);
            wrefresh(board);
            user_inter(level,robots,player);

            move(22,0);
            clrtoeol();//Σβήνει την σειρά στο y=22 από την οθόνη
            move(23,0);
            clrtoeol();
            move(24,0);
            clrtoeol();
        }

    }while(input != 'Q'); //Όσο ο παίκτης δεν βάλει 'Q'

    free(robots.position);
    free(robots.crush_status);
    delwin(board);
    endwin();
    return 0;
}

void init_game(WINDOW *win, struct Level *level,struct Player *player, struct Robot *robots){

    int i,j,temp;
//Αρχικοποίηση των πληροφοριών
    level->current_level=1;
    level->score=0;

    player->bombs = 1;
    //Τυχαίες συντεταγμένες στον παίκτη
    player->position.x = getCord(1,BOARD_WIDTH-2);
    player->position.y = getCord(1,BOARD_HEIGHT-2);
    mvwaddch(win,player->position.y,player->position.x,'M');//Εμφανίζει τον παίκτη στο σημείο που δόθηκε

    robots->robotNum = 5;
    robots->robots_left = 5;

    robots->position = (struct Position*)malloc(robots->robotNum * sizeof(struct Position)); //Δεσμεύει χώρο για 5 ρομπότ
    robots->crush_status = (int*)calloc(robots->robotNum, sizeof(int)); //Δεσμεύει χώρο και αρχικοποιεί με 0 τα crush_status

    if(robots->position == NULL || robots->crush_status == NULL){
        mvprintw(23,1,"Not enough memory, press any key to exit..");
        getch();
        delwin(win);
        exit(0);
    }

    for(i=0; i<robots->robotNum; i++){
        temp=1;
        do{         //Τυχαίες συντεταγμένες για τα ρομπότ
            robots->position[i].x = getCord(1,BOARD_WIDTH-2);
            robots->position[i].y = getCord(1,BOARD_HEIGHT-2);

            if((robots->position[i].x == player->position.x) && (robots->position[i].y == player->position.y)) //Έλεγχος αν είναι ίδιες με του παίκτη
                temp=0;
            else if(i != 0 )
            for(j=i-1; j>=0; j--){
                if((robots->position[i].x == robots->position[j].x)&&(robots->position[i].y == robots->position[j].y)) //έλεγχος αν είναι άλλο ρομπότ εκεί
                    temp=0;
            }

        }while(temp == 0);

        mvwaddch(win,robots->position[i].y,robots->position[i].x,'R');//Εμφανίζει το ρομπότ στο σημείο που δόθηκε
    }
}

int getCord(int lower, int higher)
{
    return (rand() % (higher - lower + 1)) + lower;
}

int movePlayer(WINDOW *win, char dir, struct Player *player, struct Robot robots)
{
    int i,x,y;

    x = player->position.x;
    y = player->position.y;

    switch(dir) //Έλεγχος αν ο παίκτης θα παραμείνει στα όρια του ορθογώνιου χώρου
    {
    case 'u': //Πάνω
        if (y-1 != 0)
            y--;
        else
            return 0;
       break;
    case 'd':   //Κάτω
        if(y+1 != BOARD_HEIGHT-1)
            y++;
        else
            return 0;
       break;
    case 'r':   //Δεξιά
        if (x+1 != BOARD_WIDTH-1)
            x++;
        else
            return 0;
        break;
    case 'l':   //Αριστερά
        if(x-1 != 0)
            x--;
        else
            return 0;
        break;
    case 'p':  //Πάνω-αριστερά
        if (y-1 != 0 && x-1 != 0){
            y--;
            x--;
        }else
            return 0;
       break;
    case 'h':  //Κάτω-αριστερά
        if (x-1 != 0 && y+1 != BOARD_HEIGHT-1){
            x--;
            y++;
        }else
            return 0;
       break;
    case 'e': //Πάνω-δεξιά
        if (y-1 != 0 && x+1 != BOARD_WIDTH-1){
            y--;
            x++;
        }else
            return 0;
       break;
    case 'n': //Κάτω-δεξιά
        if (y+1 != BOARD_HEIGHT-1 &&  x+1 != BOARD_WIDTH-1 ){
            y++;
            x++;
        }else
            return 0;
       break;
    case 's': //Ακίνητος
        break;
    default:  //Περίπτωση x
        return 0;
    }
        for(i=0; i<robots.robotNum; i++){
           if(robots.crush_status[i] != 2)  //Έλεγχος αν το ρομπότ βρίσκεται στον ορθογώνιο χώρο
             if(check_obs(x,robots.position[i].x,y,robots.position[i].y)) //Έλεγχος αν ο παίκτης προσπάθησε να κινηθεί πάνω σε ρομπότ ή συντρίμμια
               return 0;
        }

     mvwaddch(win,y,x,'M'); //Εμφανίζει τον παίκτη στο σημείο που δόθηκε

     player->position.x = x;
     player->position.y = y;

    return 1;
}


int moveRobots(WINDOW *win,struct Robot *robots, struct Player player,struct Level *level){

    int i, j, dist[8], distx[8], disty[8], x, y, min, flag;

    for(i=0; i<robots->robotNum; i++){

        x=robots->position[i].x;
        y=robots->position[i].y;

       if(robots->crush_status[i] == 0){  //έλεγχος αν το ρομπότ υπάρχει στον χώρο και δεν είναι συντρίμμι

        flag=0;
    //Αρχικοποίηση των πινάκων
        for(j=0; j<8; j++){
            distx[j]=BOARD_WIDTH;
            disty[j]=BOARD_HEIGHT;
            dist[j]=100;
        }
    //Αποθήκευση των συντεταγμένων των πιθανών κινήσεων του ρομπότ
        if(y-1 != 0){
        distx[0] = x;
        disty[0] = y-1;
        }
        if(y+1 != BOARD_HEIGHT-1){
        distx[1] = x;
        disty[1] = y+1;
        }
        if(x+1 != BOARD_WIDTH-1){
        distx[2] = x+1;
        disty[2] = y;
        }
        if(x-1 != 0){
        distx[3] = x-1;
        disty[3] = y;
        }
        if(y-1 != 0 && x-1 != 0){
        distx[4] = x-1;
        disty[4] = y-1;
        }
        if(x-1 != 0 && y+1 != BOARD_HEIGHT-1){
        distx[5] = x-1;
        disty[5] = y+1;
        }
        if(y-1 != 0 && x+1 != BOARD_WIDTH-1){
        distx[6] = x+1;
        disty[6] = y-1;
        }
        if(y+1 != BOARD_HEIGHT-1 &&  x+1 != BOARD_WIDTH-1){
        distx[7] = x+1;
        disty[7] = y+1;
        }


        for(j=0; j<8; j++){
                if(distx[j] != BOARD_WIDTH && disty[j] != BOARD_HEIGHT) //Έλεγχος αν άλλαξε η αρχική τιμή
                    dist[j] = distance(distx[j],player.position.x,disty[j],player.position.y); //Επιστρέφη την απόσταση του σημείου απο τον παίκτη
        }

        min = dist[0];

        for(j=1; j<8; j++){
            if(dist[j] < min)  //Υπολογισμός της μικρότερης απόστασης από τον παίκτη
                min = dist[j];
        }

        for(j=0; j<8; j++){
            if(min == dist[j] && flag == 0){
                mvwaddch(win,disty[j],distx[j],'R'); //Εμφανίζει το ρομπότ στο σημείο που δόθηκε
                robots->position[i].x = distx[j];
                robots->position[i].y = disty[j];

                flag=1;
            }
        }
      }else if(robots->crush_status[i] == 1) //Αν το ρομπότ είναι συντρίμμι
      {
         mvwaddch(win,y,x,'#'); //Εμφανίζει τα συντρίμμια στο σημείο που δόθηκε
      }
    }

    for(i=0; i<robots->robotNum; i++)
    {
        if(robots->crush_status[i] != 2){ //έλεγχος αν το ρομπότ υπάρχει στον χώρο

        flag=0;

        for(j=i+1; j<robots->robotNum; j++)
        {
          if(robots->crush_status[j] != 2){ //έλεγχος αν το ρομπότ υπάρχει στον χώρο
          flag=check_obs(robots->position[i].x,robots->position[j].x,robots->position[i].y,robots->position[j].y);

            if(flag)    //Έλεγχος αν 2 ρομπότ έχουν μετακινηθεί στο ίδιο σημείο
            {
                if(robots->crush_status[i]==0 && robots->crush_status[j]==0){ //Άν και τα 2 ρομπότ δεν ήταν συντρίμμια
                    robots->robots_left -= 2;
                    level->score += 2;
                    user_inter(*level,*robots,player);
                }else if(robots->crush_status[i]==0 || robots->crush_status[j]==0){ //Άν ενα από τα 2 ρομπότ δεν ήταν συντρίμμι
                    robots->robots_left--;
                    level->score++;
                    user_inter(*level,*robots,player);
                }

                robots->crush_status[i] = 1;
                robots->crush_status[j] = 1;
                mvwaddch(win,robots->position[i].y,robots->position[i].x,'#'); //Εμφανίζει τα συντρίμμια
            }
          }
        }
      }
    }

     for(i=0; i<robots->robotNum; i++){ //Έλεγχος αν το ρομπότ έπιασε τον παίκτη
        if(robots->crush_status[i]!=2 && check_obs(robots->position[i].x,player.position.x,robots->position[i].y,player.position.y))
                   return 1;
     }

    return 0;
}

int distance(int x1,int x2,int y1,int y2) {
    return (abs(x2-x1)+abs(y2-y1));
}

int check_obs(int x1, int x2,int y1,int y2)
{
    if(x1==x2 && y1==y2)
        return 1;

    return 0;
}

void throw_bomb(struct Player *player,struct Robot *robots,struct Level *level){

    int i,x,y,stat;

    player->bombs--;

    x=player->position.x;
    y=player->position.y;

    for(i=0; i<robots->robotNum; i++)
    {
       if(robots->crush_status[i] != 2){    //έλεγχος αν το ρομπότ υπάρχει στον χώρο και δεν είναι συντρίμμι

        stat=robots->crush_status[i];
        //Ενημερώνει το crush_status σε 2 ώστε όλα τα γειτονικά τετράγωνα να καθαρίσουν
        if(robots->position[i].x==x && robots->position[i].y==y-1)
            robots->crush_status[i]=2;
        if(robots->position[i].x==x && robots->position[i].y==y+1)
            robots->crush_status[i]=2;
        if(robots->position[i].x==x+1 && robots->position[i].y==y)
            robots->crush_status[i]=2;
        if(robots->position[i].x==x-1 && robots->position[i].y==y)
            robots->crush_status[i]=2;
        if(robots->position[i].x==x-1 && robots->position[i].y==y-1)
            robots->crush_status[i]=2;
        if(robots->position[i].x==x-1 && robots->position[i].y==y+1)
            robots->crush_status[i]=2;
        if(robots->position[i].x==x+1 && robots->position[i].y==y-1)
            robots->crush_status[i]=2;
        if(robots->position[i].x==x+1 && robots->position[i].y==y+1)
            robots->crush_status[i]=2;

       if(stat==0 && robots->crush_status[i]==2){
        robots->robots_left--;
        level->score++;
       }
    }
  }
}

void teleport(struct Player *player,struct Robot robots){

    int i,x,y,flag;

    x = player->position.x;
    y = player->position.y;

    do{

    flag=0;

    player->position.x = getCord(1,BOARD_WIDTH-2);
    player->position.y = getCord(1,BOARD_HEIGHT-2);

        for(i=0; i<robots.robotNum; i++)
        {
            if(robots.crush_status[i] != 2)
              if(check_obs(player->position.x,robots.position[i].x,player->position.y,robots.position[i].y))//έλεγχος αν είναι ελεύθερο το τετράγωνο που θα μεταφερθεί
                  flag=1;
        }

    }while((player->position.x == x && player->position.y == y) || flag == 1);
}

void user_inter(struct Level level,struct Robot robots, struct Player player){
    //Εμφανίζονται οι πληροφορίες στις συγκεκριμένες συντεταγμένες
    mvprintw(21,1,"Level: %d ",level.current_level);
    mvprintw(21,11,"Robots: %d  ",robots.robots_left);
    mvprintw(21,23,"Score: %d  ",level.score);
    mvprintw(21,34,"Bombs: %d ",player.bombs);

    if(level.max_score != 0 && level.score == level.max_score)
    mvprintw(21,45,"High-score: %d  (%s)",level.max_score,level.name);
    else if(level.score == 0)
    mvprintw(21,45,"High-score: %d",level.max_score);
}

void next_level(WINDOW *win,struct Player *player,struct Robot *robots,struct Level *level){

    int i,j,temp;

    wclear(win);
    box(win,0,0);
//Ενημέρωση πληροφοριών
    level->current_level++;
    player->bombs++;
    player->position.x = getCord(1,BOARD_WIDTH-2);
    player->position.y = getCord(1,BOARD_HEIGHT-2);
    mvwaddch(win,player->position.y,player->position.x,'M');

    robots->robotNum = 5*level->current_level;
    robots->robots_left = robots->robotNum;
//Δέσμευση επιπλέον μνήμης για τα νέα ρομπότ
    robots->position = (struct Position*)realloc(robots->position,robots->robotNum * sizeof(struct Position));
    free(robots->crush_status);
    robots->crush_status = (int*)calloc(robots->robotNum, sizeof(int));

    if(robots->position == NULL || robots->crush_status == NULL){
        mvprintw(23,1,"Not enough memory, press any key to exit..");
        getch();
        delwin(win);
        exit(0);
    }
//Έλεγχοι για το αν είναι ελεύθερο το τετράγωνο που πάει το ρομπότ
    for(i=0; i<robots->robotNum; i++){
        temp=1;
        do{
            robots->position[i].x = getCord(1,BOARD_WIDTH-2);
            robots->position[i].y = getCord(1,BOARD_HEIGHT-2);

            if((robots->position[i].x == player->position.x) && (robots->position[i].y == player->position.y))
                temp=0;
            else if(i != 0 )
            for(j=i-1; j>=0; j--){
                if((robots->position[i].x == robots->position[j].x)&&(robots->position[i].y == robots->position[j].y))
                    temp=0;
            }

        }while(temp == 0);

        mvwaddch(win,robots->position[i].y,robots->position[i].x,'R');
    }
        user_inter(*level,*robots,*player);
        wrefresh(win);
}


