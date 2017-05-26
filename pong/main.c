#include <stdlib.h>
#include <math.h>
#include <cab202_graphics.h>
#include <cab202_sprites.h>
#include <cab202_timers.h>
#include <string.h>
#include <time.h> /*REMOVE ME LATER*/

#define DELAY (50) /* milliseconds */

bool game_over = false;
bool update_screen = true;

//Keyboard
#define KEY_CYCLE_LEVEL   'l'
#define KEY_MOVE_UP       'w'
#define KEY_MOVE_DOWN     's'
#define KEY_HELP_SCREEN   'h'
#define KEY_CYCLE_LEVEL   'l'

#define GAME_STATE_COUNTDOWN    (2)
#define GAME_STATE_LEVEL        (3)
#define GAME_STATE_GAMEOVER     (4)
#define GAME_STATE_SPAWN        (5)
int game_state = GAME_STATE_LEVEL;

#define LEVEL_HANDBALL    (1)
#define LEVEL_ROBOPONG    (2)
#define LEVEL_SINGULARITY (3)
#define LEVEL_RAILS       (4)
#define NUM_LEVELS        (4)

#define GUI_NUM_VAR (4)

// MISC
static int gui_lives;
static int gui_score;
static int gui_level = LEVEL_HANDBALL;
static int gui_time;
static char gui_boarder_char = '*';
static char paddle_char = '|';
static int h, hs;
static int w;


void draw_gui(void);
void cycle_level(void);

int ceiling_top(void);
int ceiling_bottom(void);
int center_x(void);
int center_y(void);
int left_center_x(void);
int left_center_y(void);
int right_center_x(void);
int right_center_y(void);

void reset(void);

bool is_move_frozen = true;
bool is_time_frozen = false;

double count_down_time = 0.9;
bool has_counted_down(void);
double singularity_time = 5.9;
bool has_singularity(void);
double last_update_time = 0;
double banked_time = 0;
double total_time_banked = 0;
void update_time(void);
void pause_time(void);
void start_time(void);
void reset_time(void);
void clear_time(void);

bool has_spawned = false;
void respawn(void);

sprite_id create_paddle(int x, int y);


// HAND BALL
static sprite_id paddle_right;  /*PLAYER*/
static sprite_id paddle_left;   /*PLAYER*/
static char* paddle_img = NULL;
static int paddle_height = 7;
static int paddle_width = 1;

static sprite_id* rails[2];
static int rails_width = 0;
static int rail_width = 1;
static int rail_height = 1;
static char* rail_img = "=";

static sprite_id singularity;
static double singularity_strength = 1;
static int singularity_height = 5;
static int singularity_width = 5;
static char* singularity_img = 
 "\\ | /"
 " \\|/ "
  "-- --"
  " /|\\ "
  "/ | \\";

static sprite_id ball;
static double ball_speed = 0.75;
static int ball_height = 1;
static int ball_width = 1;
static char* ball_img = 
  "o";



//GAME
void process(void);
void help_screen(void);
void process_countdown(void);
void process_gameover(void);
void process_spawn(void);
void process_level(void);
void update_level(void);
bool paddle_collision(sprite_id paddle);

void cleanup(void);
void setup(void);
void input(void);

int main(void) {
  srand(time(NULL)); /*remove me*/
	setup_screen();

	//auto_save_screen(true);
	setup();
	
  show_screen();

  help_screen();

  while ( !game_over ) {
		process();

		if ( update_screen ) {
			show_screen();
		}
		timer_pause(DELAY);
	}

	cleanup();

	return 0;

}

void cleanup(void){
}

int center_x(void)      {return (w/2);      }

int center_y(void)      {return ((h-hs)/2); }

int left_center_x(void) {return 0;          }

int left_center_y(void) {return ((h-hs)/2); }

int right_center_x(void){return (w);        }

int right_center_y(void){return ((h-hs)/2); }

int top_y (void)        {return hs + 1;     }

int bottom_y(void)      {return h - 1;      }

void setup(void){

  h = screen_height();
  hs = 2;
  w = screen_width();

  singularity_strength = 1.25;

  ball = sprite_create(0, 0, ball_width, ball_height, ball_img);
  singularity = sprite_create(0, 0, singularity_width, singularity_height, singularity_img);

  {
    if(h > 21){
      paddle_height = 7;
    }else{
      paddle_height = (h - hs - 1)/2;
    }
    paddle_img = (char*)malloc(sizeof(char)*2*paddle_height);
    for(int i = 0; i < paddle_height; i++){
      paddle_img[i] = paddle_char;
    }
    paddle_left = sprite_create(0, 0, paddle_width, paddle_height, paddle_img);
    paddle_right = sprite_create(0, 0, paddle_width, paddle_height, paddle_img);


  }

  rails_width = w / 2;

  rails[0] = (sprite_id*)malloc(rails_width * sizeof(sprite_id));
  rails[1] = (sprite_id*)malloc(rails_width * sizeof(sprite_id));  
  for(int i = 0; i < rails_width; i++){
    rails[0][i] = sprite_create(i+w/4, h/4, rail_width, rail_height, rail_img); 
    rails[1][i] = sprite_create(i+w/4,(h/4)*3, rail_width, rail_height, rail_img);
  }

  reset();

}

bool sprite_collides_with_gui(sprite_id s){
  int x = sprite_x(s);
  int y = sprite_y(s);
  int width = sprite_width(s);
  int height = sprite_height(s);


  if(x <= 1)          return true; /*LEFT*/
  if(x+width >= w-1)  return true; /*RIGHT*/

  if(y <= hs)         return true; /*UP*/
  if(y+height >= h)   return true; /*DOWN*/
  
  return false;
}

int distance_top_to_ceiling(sprite_id s){
  #define TOP_BOARDER (1)
  return sprite_y(s) - hs - TOP_BOARDER;
}

int distance_bottom_to_ceiling(sprite_id s){
  #define BOTTOM_BOARDER (1)
  return h - (sprite_y(s) + sprite_height(s)) - BOTTOM_BOARDER;
}

void move_paddle(sprite_id p, double y){
  if(y < 0 && y < -distance_top_to_ceiling(p)){
    sprite_move(p, 0, -distance_top_to_ceiling(p));
  }else if(y > 0 && y > distance_bottom_to_ceiling(p)){
    sprite_move(p, 0, distance_bottom_to_ceiling(p));
  }else{
    sprite_move(p, 0, y);
  }

}

bool can_move(void) {return has_counted_down() && !is_move_frozen;}

void input(void){
  char key = get_char();
  switch(key){
    case KEY_MOVE_UP:
      #define MOVE_UP (-1)
      if(can_move()) move_paddle(paddle_right, MOVE_UP);
      break;
    case KEY_MOVE_DOWN:
      #define MOVE_DOWN (1)
      if(can_move()) move_paddle(paddle_right, MOVE_DOWN);
      break;
    case KEY_CYCLE_LEVEL:
      cycle_level();
      break;
    case KEY_HELP_SCREEN:
      help_screen();
      break;
  }
}

void reset(void){
  gui_lives = 3;
  gui_score = 0;
  gui_time = 0;

  has_spawned = false;

  sprite_move_to(paddle_left, left_center_x()+3, left_center_y()-(paddle_height/2));
  sprite_move_to(paddle_right, right_center_x()-3-1, right_center_y()-(paddle_height/2));
  sprite_move_to(singularity, center_x()-singularity_width/2, center_y()-(singularity_height/2));
  sprite_move_to(ball, center_x(), center_y());

  clear_time();


  for(int i = 0; i < rails_width; i++){
    sprite_show(rails[0][i]);
    sprite_show(rails[1][i]);
  }
}

void cycle_level(void){
  clear_screen();
  reset();
  gui_level = (gui_level+1) % (NUM_LEVELS+1);
  if(gui_level == 0) gui_level = 1;
}

void update_time(void){
  if(!is_time_frozen){
    banked_time += get_current_time() - last_update_time;
    last_update_time = get_current_time();
    gui_time = (int)(total_time_banked)+(banked_time - count_down_time);
    if(gui_time < 0){gui_time = 0;}
  }
}

void pause_time(void){
  if(!is_time_frozen){
    is_time_frozen = true;
  }
}

void start_time(void){
  if(is_time_frozen){
    is_time_frozen = false;
    last_update_time = get_current_time();
    update_time();
  }
}

void clear_time(void){
  total_time_banked = 0;
  banked_time = 0;
  last_update_time = get_current_time();
}

void reset_time(void){
  total_time_banked += banked_time;
  banked_time = 0;
  last_update_time = get_current_time();
}

bool has_counted_down(void)           {  return banked_time >= count_down_time;  }

bool has_ai(void)                     {  return gui_level == LEVEL_ROBOPONG || gui_level == LEVEL_SINGULARITY  || gui_level == LEVEL_RAILS;  }

bool has_singularity_spawned(void)    {  return banked_time >= singularity_time; }

bool should_process_singularity(void) {  return has_singularity_spawned() && gui_level == LEVEL_SINGULARITY; }

bool should_process_rails(void)       {  return gui_level == LEVEL_RAILS; }

bool should_process_ai(void)          {  return has_ai();  }

void draw_gui(void){
  {
    {/*Boarders*/
      draw_line(0               ,0                ,screen_width()-1,0                ,gui_boarder_char);
      draw_line(screen_width()-1,0                ,screen_width()-1,screen_height()-1,gui_boarder_char);
      draw_line(screen_width()-1,screen_height()-1,0               ,screen_height()-1,gui_boarder_char);
      draw_line(0               ,screen_height()-1,0               ,0                ,gui_boarder_char);
    }

    {/*Score*/
      draw_line(0, 2, screen_width()-1, 2, gui_boarder_char);
      
      static char* labels[GUI_NUM_VAR] = {" Lives = %d"," Score = %d"," Level = %d"," Time = %02d:%02d"};
      static int* values[GUI_NUM_VAR]  = { &gui_lives,   &gui_score,   &gui_level,   &gui_time }; 
      
      for(int i = 0; i < GUI_NUM_VAR; i++){
        int p = (i)*(screen_width()/GUI_NUM_VAR);
        draw_line(p, 0, p, 2, gui_boarder_char);
        if(i == GUI_NUM_VAR-1){
          draw_formatted(p+1, 1, labels[i], *values[i] / 60, *values[i] % 60);
        }else{
          draw_formatted(p+1, 1, labels[i], *values[i]);
        }
      } 
    }
  }
}

void draw_ai(void){
  sprite_draw(paddle_left);
}

void draw_singularity(void){
  sprite_draw(singularity);
}

void draw_rails(void){
  for(int i = 0; i < rails_width; i++){
    sprite_draw(rails[0][i]);
    sprite_draw(rails[1][i]);
  }
}

void draw_level(void){
  
  draw_gui();

  sprite_draw(ball);

  if(should_process_singularity())
    draw_singularity();
  
  if(should_process_rails())
    draw_rails();

  if(should_process_ai()) 
    draw_ai();

  sprite_draw(paddle_right);
    
   
}
void draw_countdown(void){
  draw_formatted(center_x(), center_y(), "%d", (int)(3-(banked_time*10/3)));
}
void process(void) {
  clear_screen();

  is_move_frozen = false;
  input();
  is_move_frozen = true;

  update_time();
  if(has_counted_down()){ 
    draw_level();
    process_level();
    
  }else{             
    if(!has_spawned) 
      respawn();
    draw_level();
    draw_countdown();
  }
  show_screen();

}

void respawn(void){
  sprite_move_to(ball, center_x(), center_y());
  double dir = (rand() % 90) - 45.0;
  sprite_turn_to(ball, ball_speed, 0);
  sprite_turn(ball, dir); //dir
  has_spawned = true;
}

void help_screen(void){ 
  
  #define NUM_HELP_SCREEN_LABELS  (7)
  
  pause_time();
  clear_screen();
  
  static char* help_screen_labels[NUM_HELP_SCREEN_LABELS] =
  {
    "Student Number   :: N9159932      ",
    "Student Name     :: Carlin Connell",
    "Move Paddle Up   :: W             ",
    "Move Paddle Down :: S             ",
    "Cycle Level      :: L             ",
    "Help Screen      :: H             ",
    "Exit Help Screen :: [Any Key]     "
  };

  for(int i = 0; i < NUM_HELP_SCREEN_LABELS; i++){
    draw_string(((screen_width()-1)/2)-(strlen(help_screen_labels[i])/2),
    ((screen_height()-1)/2)+(i-(NUM_HELP_SCREEN_LABELS/2)),
    help_screen_labels[i]);
  }

  show_screen();
  wait_char();
  start_time();

}

void process_gameover(void){
  #define NUM_GAMEOVER_SCREEN_LABELS  (3)
  clear_screen();
  static char* gameover_labels[NUM_GAMEOVER_SCREEN_LABELS] =
  {
    "Play again!",
    " ",
    "[Any key]"
  };

  for(int i = 0; i < NUM_GAMEOVER_SCREEN_LABELS; i++){
    draw_string(((screen_width()-1)/2)-(strlen(gameover_labels[i])/2),
    ((screen_height()-1)/2)+(i-(NUM_GAMEOVER_SCREEN_LABELS/2)),
    gameover_labels[i]);
  }

  show_screen();
  wait_char();
  help_screen();
  reset();
}

bool sprite_collides(sprite_id a, sprite_id b){
  //sprite
  int ax = sprite_x(a);
  int ay = sprite_y(a);
  int awidth = sprite_width(a);
  int aheight = sprite_height(a);

  //ball
  int bx = sprite_x(b);
  int by = sprite_y(b);
  int bwidth = sprite_width(b);
  int bheight = sprite_height(b);

  return 
          (ax            <= bx + bwidth   &&
           ax + awidth   >  bx            &&
           ay            <= by + bheight  &&
           aheight + ay  >  by);
}

bool is_ball_moving_downwards(void) {return sprite_dy(ball) > 0;}
bool is_ball_moving_upwards(void)   {return sprite_dy(ball) < 0;}

#define COLLISION_PADDLE_TOP          ( 1)
#define COLLISION_PADDLE_MIDDLE       ( 2)
#define COLLISION_PADDLE_BOTTOM       ( 3)
#define COLLISION_PADDLE_NO_COLLISION (-1)
int point_of_contact(sprite_id ball, sprite_id paddle){
  if(sprite_y(ball) == sprite_y(paddle))
    return COLLISION_PADDLE_TOP;
  else if(sprite_y(ball) == sprite_y(paddle) + paddle_height)
    return COLLISION_PADDLE_BOTTOM;
  else if (sprite_y(ball) > sprite_y(paddle) && 
           sprite_y(ball) < sprite_y(paddle) + paddle_height)
    return COLLISION_PADDLE_MIDDLE;
  else
    return COLLISION_PADDLE_NO_COLLISION;
}

void change_ball_dir(int x, int y){
  sprite_turn_to(ball,sprite_dx(ball)*x,sprite_dy(ball)*y);
}

bool paddle_collision(sprite_id paddle){
  bool collided = false;
  if(sprite_collides(paddle, ball)){
    collided = true;

    switch(point_of_contact(ball, paddle)){
      case COLLISION_PADDLE_TOP:
        if(is_ball_moving_downwards() && 
           distance_top_to_ceiling(paddle) > sprite_height(ball))
        {
          change_ball_dir(1, -1);
        }else{
          change_ball_dir(-1, 1);
        }
        break;
      case COLLISION_PADDLE_MIDDLE:
        change_ball_dir(-1, 1);
        break;
      case COLLISION_PADDLE_BOTTOM:
        if(is_ball_moving_upwards() &&
           distance_bottom_to_ceiling(paddle) > sprite_height(ball))
        {
          change_ball_dir( 1,-1);
        }else{
          change_ball_dir(-1, 1);
        }
        break;
      case COLLISION_PADDLE_NO_COLLISION:
        break;
    }
  }
  return collided;
}



void process_singularity(void){

  double x_diff = sprite_x(singularity) - sprite_x(ball);
  double y_diff = sprite_y(singularity) - sprite_y(ball);

  double dist_squared = pow(x_diff, 2.0) + pow(y_diff, 2.0);
  
  if(dist_squared < 1e-10){
    dist_squared = 1e-10;
  }

  double dist = sqrt(dist_squared);
  dist = pow(dist, 1.0/singularity_strength);

  double dx = sprite_dx(ball);
  double dy = sprite_dy(ball);

  double GM = 0.2;
  double a = GM/dist_squared;

  dx = dx + (a * x_diff / dist);
  dy = dy + (a * y_diff / dist);

  double v = sqrt(pow(dx, 2.0) + pow(dy, 2.0));

  if(v > 1.0){
    dx = dx / v;
    dy = dy / v;
  }

  sprite_turn_to(ball, dx, dy);

}

void process_rails(void){
  for(int i = 0; i < rails_width; i++){
    if(sprite_visible(rails[0][i])){
      if(sprite_collides(rails[0][i], ball)){
        sprite_back(ball);
        sprite_hide(rails[0][i]);
        change_ball_dir(1,-1);
      }
    }
    if(sprite_visible(rails[1][i])){
      if(sprite_collides(rails[1][i], ball)){
        sprite_back(ball);
        sprite_hide(rails[1][i]);
        change_ball_dir(1,-1);
      }
    }
  }  
}

void process_ai(void) {

  int ball_y = sprite_y(ball);
  int paddle_y = sprite_y(paddle_left) + (paddle_height/2);
  double move_direction = (double)(ball_y - paddle_y);
  
  if(move_direction != 0){

    move_direction = abs(ball_y - paddle_y)/move_direction;    
    double move = move_direction;
    move_paddle(paddle_left, move);
    
  }

  if(paddle_collision(paddle_left)){

  }
}

void process_ball(void){

    if(sprite_x(ball) > right_center_x()-1){
      gui_lives--;
      if(gui_lives == 0){
        process_gameover();
      }else{
        reset_time();
        has_spawned = false;
      }
    }else if(sprite_x(ball) < left_center_x()+1){
      change_ball_dir(-1, 1);
    }

    if(sprite_y(ball) < hs + 1){
      change_ball_dir(1,-1);
    }else if (sprite_y(ball) > h - 2){
      change_ball_dir(1,-1);
    }

    sprite_step(ball);
    

}

void process_player(void){
    if(paddle_collision(paddle_right)){
        gui_score++;
    }
}

void process_level(void){

  process_player();

  process_ball();

  if(should_process_singularity())
    process_singularity();

  if(should_process_rails())
    process_rails();

  if(should_process_ai()){
    process_ai();
  }

  
    
}