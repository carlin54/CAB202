#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#include "cpu_speed.h"
#include "lcd.h"
#include "graphics.h"
#include "sprite.h"

#define ZERO (0)


#define BUTTON_SW3		(128)	// 10000000
#define BUTTON_SW2		(64)	// 01000000
#define MOVE_UP 		(32) 	// 00100000
#define MOVE_DOWN 		(16) 	// 00010000
#define MOVE_LEFT 		(8)		// 00001000
#define MOVE_RIGHT 		(4)		// 00000100
#define MOVE_WAIT 		(2)		// 00000010
#define MOVE_NO_NEW 	(1)		// 00000001

#define MASK_MOVE_UP 	(16)	// 010000
#define MASK_MOVE_DOWN 	(32) 	// 100000
#define MASK_MOVE_LEFT 	(4)		// 000100
#define MASK_MOVE_RIGHT (8)		// 001000
#define MASK_MOVE_ALL	(15)	// 111100

#define T true
#define F false

#define DEBUG_ENABLED T
#define ERROR_ENABLED T
#define DEBUG_MESSAGE_TIME (1000)
#define ERROR_MESSAGE_TIME (2000)

void debug_message(bool proc, char* say){
	if(proc && DEBUG_ENABLED){
		clear_screen();
		int len = strlen(say);
		draw_string(LCD_X/2 - (len/2)*5, LCD_Y/2, say);
		show_screen();
		_delay_ms(DEBUG_MESSAGE_TIME);
	}
}
void debug_int(bool proc, int say){
	if(proc && DEBUG_ENABLED){
		clear_screen();
		char say_as_char[5];
		sprintf(say_as_char, "%d", say);
		draw_string(LCD_X/2, LCD_Y/2, say_as_char);
		show_screen();
		_delay_ms(DEBUG_MESSAGE_TIME);
	}
}



void error_message(bool error, char* say){
	if(error && ERROR_ENABLED){
		clear_screen();
		int len = strlen(say);
		draw_string(LCD_X/2 - (len/2)*5, LCD_Y/2, say);
		show_screen();
		_delay_ms(ERROR_MESSAGE_TIME);
	}
}
void error_message_int(bool error, int say){
	if(error && ERROR_ENABLED){
		clear_screen();
		char say_as_char[5];
		sprintf(say_as_char, "%d", say);
		draw_string(LCD_X/2, LCD_Y/2, say_as_char);
		show_screen();
		_delay_ms(ERROR_MESSAGE_TIME);
	}
}

struct point { int x; int y; };
inline bool points_are_equal(const struct point* a, const struct point* b){
	return a->x == b->x && a->y == b->y;
}
struct boundingbox { int x; int y; int width; int height; };
inline bool collides(const struct boundingbox* a, const struct boundingbox* b){
	return 	a->x 			< b->x + b->width 	&&
   			a->x + a->width 	> b->x 			&&
   			a->y 			< b->y + b->height 	&&
   			a->height +a->y 	> b->y;
}
inline void copy(const struct point* a, struct point* b){
	b->x = a->x;
	b->y = a->y;
}
inline void swap(struct point* a, struct point* b){
	int t = b->x;
	b->x = a->x;
	a->x = t;
	t = b->y;
	b->y = a->y;
	a->y = t;
}
inline int random_range(int min, int max){
	return min + rand() % (max - min);
}
inline int max(int a, int b){
	return a > b ? a : b;
}
inline int min(int a, int b){
	return a < b ? a : b;
}

inline int difference(int a, int b){
	return b - a;
}
inline int lcd_sum_pixels(){ 
	return LCD_X * LCD_Y;
}

inline void init_hardwear(){
	lcd_init(LCD_DEFAULT_CONTRAST);
	
	DDRD &= ~(1 << 1); // UP 		- PD1
	DDRB &= ~(1 << 7); // DOWN 		- PB7
	DDRB &= ~(1 << 1); // LEFT 		- PB1
	DDRD &= ~(1 << 0); // RIGHT 	- PD0
	DDRB &= ~(1 << 0); // CENTER 	- PB0

	DDRF &= ~(1 << 5); // LEFT 	-	PF5
	DDRF &= ~(1 << 6); // RIGHT -	PF6
}

#define WELCOME_SCREEN_TIME (2000)
void welcome_screen(){
	clear_screen();
	{
		draw_string(LCD_X/2-(14*5/2), LCD_Y/2-10, "Carlin Connell");
		draw_string(LCD_X/2-(8*5/2), LCD_Y/2+10, "N9159932");
	}
	show_screen();
	_delay_ms(WELCOME_SCREEN_TIME);
}

#define GAME_OVER_TIME (2000)
bool snake_is_dead;
void game_over(void){
	clear_screen();
	draw_string(LCD_X/2-(10*5/2), LCD_Y/2, "Game Over!");
	
	snake_is_dead = true;

	show_screen();
	_delay_ms(GAME_OVER_TIME);
}
bool is_game_over(){
	return snake_is_dead;
}


#define NUM_WALLS (3)
const int min_wall_length_x = 10;
const int max_wall_length_x = LCD_X/2;
const int min_wall_length_y = 10;
const int max_wall_length_y = LCD_Y/2;
struct point walls[NUM_WALLS][2];
bool walls_enabled = false;
int walls_sum_pixels = 0;
inline bool walls_are_enabled(){	
	return walls_enabled;	
}
inline int distance_between(const struct point* p1, const struct point* p2){
		return sqrt(pow(difference(p1->x, p2->x), 2.0) + pow(difference(p1->y, p2->y), 2.0));
}
int walls_calculate_sum_pixels(){
	int sum = 0;
	for(int i = 0; i < NUM_WALLS; i++){
		sum += distance_between(&walls[i][0], &walls[i][1]);
	}
	return sum;
}
void init_walls(void){
	int wall_length;
	int wall_anchor;
	for(int i = 0; i < NUM_WALLS; i++){
		if(i % 2 == 0){ // (X)
			wall_anchor = rand() % LCD_X;
			wall_length = rand() % (max_wall_length_y-min_wall_length_y)+min_wall_length_y;
			
			walls[i][0].x = wall_anchor;
			walls[i][1].x = wall_anchor;

			if(i % 3 == 0){
				walls[i][0].y = 0;
				walls[i][1].y = wall_length;
			}else{
				walls[i][0].y = LCD_Y;
				walls[i][1].y = LCD_Y-wall_length;
			}
			
		}else{  // (Y)
			wall_anchor = rand() % LCD_Y;
			wall_length = rand() % (max_wall_length_x-min_wall_length_x)+min_wall_length_x;
			
			walls[i][0].y = wall_anchor;
			walls[i][1].y = wall_anchor;

			if(i % 3 == 0){
				walls[i][0].x = 0;
				walls[i][1].x = wall_length;
			}else{
				walls[i][0].x = LCD_X;
				walls[i][1].x = LCD_X-wall_length;
			}
		}
	}

	walls_sum_pixels = walls_calculate_sum_pixels();
}


void draw_walls(void){
	for(int i = 0; i < NUM_WALLS; i++){
		draw_line(walls[i][0].x,walls[i][0].y,walls[i][1].x,walls[i][1].y);
	}
}

int snake_spawn_score = 0;
int snake_score = 0;
int snake_spawn_lives = 5;
int snake_lives = 5;
int snake_spawn_length = 2;
int snake_length = 2;
int snake_body_array_length = 3;
struct point* snake_body;
int snake_current_direction = MOVE_RIGHT;
int snake_new_direction = MOVE_RIGHT;
int snake_body_width = 3;
int snake_body_height = 3;


inline int snake_sum_pixels(){
	return snake_body_width * snake_body_height * snake_length;
}
inline int snake_food_eaten(){
	return snake_body_array_length - snake_spawn_length;
}

inline struct point* index_snake(int i){
	error_message(i >= snake_body_array_length, "error:index_snake()");
	error_message(i < 0, "error:index_snake()");
	return &snake_body[i];
}
inline struct point* snake_head(){
	return index_snake(0);
}

void init_snake(void){
	//init_sprite(&snake_body_sprite, LCD_X/2, LCD_Y/2, snake_body_width, snake_body_height, (unsigned char*)snake_body_img);
	snake_body = NULL;
}

void respawn_snake(void){
	
	if(snake_body != NULL){
		free(snake_body);
		snake_body = NULL;
	}

	snake_body_array_length = snake_spawn_length; 
	snake_body = (struct point*)calloc(snake_body_array_length, sizeof(struct point));
	error_message(snake_body == NULL, "error:out of memory");

	for(int i = 0; i < snake_body_array_length; i++){
		index_snake(i)->x = LCD_X/2 - (snake_body_width * i );
		index_snake(i)->y = LCD_Y/2;
	}
	snake_is_dead = false;
	snake_current_direction = MOVE_WAIT;
}

void spawn_snake(void){
	snake_score = snake_spawn_score;
	snake_lives = snake_spawn_lives;

	respawn_snake();
}
int clamp(int min, int max, int val){
	if(val > max) return max;
	if(val < min) return min;
	return val;
}
int clamp_min(int min,int val){
	if(val < min) return min;
	return val;
}
int clamp_max(int max, int val){
	if(val > max) return max;
	return val;
}
inline void draw_snake_segment_line(int x, int y, int size_x, int size_y){
	int xp = clamp_max(LCD_X, x % LCD_X + size_x);
	int yp = clamp_max(LCD_Y, y % LCD_Y + size_y);
	int xm = clamp_min(0, x % LCD_X - size_x);
	int ym = clamp_min(0, y % LCD_Y - size_y);
	draw_line(xp, yp, xm, yp); //rt, lt : warning overdraw
	draw_line(xm, yp, xm, ym); //lt, lb : warning overdraw
	draw_line(xm, ym, xp, ym); //lb, rb : warning overdraw
	draw_line(xp, ym, xp, yp); //rb, rt : warning overdraw
}
inline int direction(int x1, int x2){
	return (difference(x1, x2))/abs(difference(x1, x2));
}
inline int interpolate(int ax1, int ax2, int step_size, int step_number){
	return ax1 + (direction(ax1, ax2) * step_number * step_size);  //warning: excess division
}
inline int draw_snake_segments(const struct point* p1, const struct point* p2, int max_allowed_steps){
	
	int size_x = (snake_body_width-1)/2; 
	int size_y = (snake_body_height-1)/2;

	int max_possible_steps = max(abs(difference(p1->x, p2->x))/snake_body_width, abs(difference(p1->y, p2->y))/snake_body_height);
	int steps = min(max_allowed_steps, max_possible_steps);
	
	error_message(difference(p1->x, p2->x) != ZERO && difference(p1->y, p2->y) != ZERO, "Error:Diagonal Snake");

	if(difference(p1->x, p2->x) != ZERO){
		for(int step = 0; step < steps; step++){
			draw_snake_segment_line(interpolate(p1->x, p2->x, snake_body_width, step) % LCD_X, //waring: excess calculations
			 p1->y % LCD_Y, size_x, size_y);
		}
	}else if(difference(p1->y, p2->y) != ZERO){
		for(int step = 0; step < steps; step++){
			draw_snake_segment_line(p1->x % LCD_X, interpolate(p1->y, p2->y % LCD_Y, snake_body_height, step),  //waring: excess calculations
			 size_x, size_y);
		}		
	}else{
		draw_snake_segment_line(p1->x, p1->y, size_x, size_y); //warning: redraw
		return 1; //case: segment is apple segment passing though 
	}
	
	return steps;
}

void draw_snake(void){
	int size_x = (snake_body_width-1)/2; 
	int size_y = (snake_body_height-1)/2;
	for(int i = 0; i < snake_body_array_length; i++){
		draw_snake_segment_line(index_snake(i)->x, index_snake(i)->y, size_x, size_y);
	}

}

void hurt_snake(void){
	snake_lives--;
	if(snake_lives == 0) 
		game_over();
	else
		respawn_snake();

}




inline bool is_turn_same_direction(int current_direction, int new_direction){
	return current_direction & new_direction;
}

bool is_turn_safe_direction(int current_direction, int new_direction){

	switch(current_direction){
		case MOVE_UP:
			return MASK_MOVE_UP ^ new_direction;
			break;
		case MOVE_DOWN:
			return MASK_MOVE_DOWN ^ new_direction;
			break;
		case MOVE_LEFT:
			return MASK_MOVE_LEFT ^ new_direction;
			break;
		case MOVE_RIGHT:
			return MASK_MOVE_RIGHT ^ new_direction;
			break;
	}
	return false;
}

void turn_snake(int new_direction){

	if(!is_turn_same_direction(snake_current_direction, new_direction)){
		//debug_message(true, "SAME");
		if(is_turn_safe_direction(snake_current_direction, new_direction)){
			//debug_message(true, "SAFE");
			snake_current_direction = new_direction;
		}else{
			hurt_snake();
		}
	}
}

void feed_snake(void){

	snake_body = (struct point*)realloc((void*)snake_body, (snake_body_array_length + 1) * sizeof(struct point));
	error_message(snake_body == NULL, "error:out of memory");
	snake_body_array_length++;
	
	for(int i = snake_body_array_length-1; i > 1; i--){
		copy(index_snake(i-1), index_snake(i));
	}
	copy(snake_head(), index_snake(1));

	if(walls_are_enabled()){
		snake_score += 2;
	}else{
		snake_score += 1;
	}	
}

void step_snake(void){
	

	for(int i = snake_body_array_length-1; i > 0; i--){
		if(points_are_equal(index_snake(i), index_snake(i-1))){
			swap(index_snake(i-1), index_snake(i));
		}else{
			copy(index_snake(i-1), index_snake(i));
		}
	}


	switch(snake_current_direction){
		case MOVE_UP:
			snake_head()->y = (snake_head()->y - snake_body_height) % LCD_Y;
			break;
		case MOVE_DOWN:
			snake_head()->y = (snake_head()->y + snake_body_height) % LCD_Y;
			break;
		case MOVE_LEFT:
			snake_head()->x = (snake_head()->x - snake_body_width) % LCD_X;
			break;
		case MOVE_RIGHT:
			snake_head()->x = (snake_head()->x + snake_body_width) % LCD_X;
			break;
	}
}
inline void snake_segment_boundingbox(const struct point* a, struct boundingbox* seg){
	seg->x = a->x-1;
	seg->y = a->y-1;
	seg->width = snake_body_width;
	seg->height = snake_body_height;
}

inline void snake_segment_line_boundingbox(const struct point* a, const struct point* b, struct boundingbox* seg){
	seg->x = min(a->x, b->x)-1;
	seg->y = min(a->y, b->y)-1;
	seg->width = abs(difference(a->x, b->x)+1);
	seg->height = abs(difference(a->y, b->y)+1);
}

bool collides_with_snake(const struct boundingbox* b){
	struct boundingbox a;
	for(int i = snake_length; i < 0; i--){
		snake_segment_line_boundingbox(index_snake(i), index_snake(i-1), &a);
		if(collides(&a, b)){
			return true;
		}
	}
	return false;
}

struct boundingbox apple_aabb;
int apple_width = 3;
int apple_height = 3;

inline bool is_space_for_apple(){ //warning: unused
	if(walls_are_enabled())
		return snake_sum_pixels() + walls_sum_pixels >= lcd_sum_pixels();
	else
		return snake_sum_pixels() >= lcd_sum_pixels();	
}
void apple_new_location(){
	while(true){	//warning: possible spin, and real possible spin
		apple_aabb.x = random_range(0, LCD_X-apple_width); //rand() % LCD_X;
		apple_aabb.y = random_range(0, LCD_Y-apple_height);//rand() % LCD_Y;
		if(!collides_with_snake(&apple_aabb)){
			break;
		}
	}
}

void init_apple(void){
	apple_aabb.width = apple_width;
	apple_aabb.height = apple_height;
	apple_new_location();
}

void draw_apple(void){

	for(int i = 0; i < apple_aabb.height; i++){
		draw_line(
			apple_aabb.x 					, apple_aabb.y + i, 
			apple_aabb.x + apple_width-1 	, apple_aabb.y + i);
	}
}



inline void wall_boundingbox(const struct point* a, const struct point* b, struct boundingbox* wall_aabb){
	wall_aabb->x = min(a->x, b->x)-1;
	wall_aabb->y = min(a->y, b->y)-1;
	wall_aabb->width = abs(difference(a->x, b->x)+1);
	wall_aabb->height = abs(difference(a->y, b->y)+1);
}

bool collides_with_wall(const struct boundingbox* b){
	struct boundingbox a; 
	for(int i = 0; i < NUM_WALLS; i++){
		wall_boundingbox(&walls[i][0], &walls[i][1], &a);
		if(collides(&a, b)){
			return true;
		}
	}
	return false;
}
bool snake_convexes(){
	struct boundingbox snake_head_aabb, snake_segment_aabb;
	snake_segment_boundingbox(snake_head(), &snake_head_aabb);
	for(int i = 1; i < snake_body_array_length; i++){
		snake_segment_boundingbox(index_snake(i), &snake_segment_aabb);
		if(collides(&snake_head_aabb, &snake_segment_aabb)){
			return true;
		}
	}
	return false;
}

bool collides_with_apple(const struct boundingbox* b){
	return collides(&apple_aabb, b);
}

void collision_detection(void){
	struct boundingbox snake_head_aabb;
	snake_segment_boundingbox(snake_head(), &snake_head_aabb);

	if(snake_convexes()){ // must be before, feed_snake()
		hurt_snake();
	}
	
	if(collides_with_apple(&snake_head_aabb)){
		feed_snake();
		apple_new_location();
	}
	
	if(walls_are_enabled() && collides_with_wall(&snake_head_aabb)){
		hurt_snake();
	}

}

char buffer_gui[2+1+1+1] = {
	'F','F','(','L',')',
};

void draw_gui(void){
	//(Food, Lives)
	//5, (3)
	itoa(snake_score, &buffer_gui[0], 10);
	itoa(snake_lives, &buffer_gui[3], 10);
	if(buffer_gui[0] == '\0') buffer_gui[0] = '0';
	if(buffer_gui[1] == '\0') buffer_gui[1] = ' ';
	if(buffer_gui[2] == '\0') buffer_gui[2] = '(';
	if(buffer_gui[3] == '\0') buffer_gui[3] = '0';
	if(buffer_gui[4] == '\0') buffer_gui[4] = ')';
	
	draw_string(0, 0, buffer_gui);
}

bool is_s1_up_pressed(){
	return PIND >> 1 & 1;
}
bool is_s1_down_pressed(){
	return PINB >> 7 & 1;
}
bool is_s1_left_pressed(){
	return PINB >> 1 & 1;
}
bool is_s1_right_pressed(){
	return PIND >> 0 & 1;
}
bool is_sw2_pressed(){
	return (PINF>>PF6) & 1;
}
bool is_sw3_pressed(){
	return (PINF>>PF5) & 1;
}

bool speed_stick = false;
int get_input(){
	if(is_s1_up_pressed()){
		return MOVE_UP;
	}
	if(is_s1_down_pressed()){
		return MOVE_DOWN;
	}
	if(is_s1_left_pressed()){
		return MOVE_LEFT;
	}
	if(is_s1_right_pressed()){
		return MOVE_RIGHT;
	}
	if(is_sw2_pressed()){
		return BUTTON_SW2;
	}
	if(is_sw3_pressed()){
		return BUTTON_SW3;
	}
	return MOVE_NO_NEW;
}
bool double_pressed = false;
void process_input(int input){

	switch(input){
		case BUTTON_SW2:
			walls_enabled = false;
			break;
		case BUTTON_SW3:
			walls_enabled = true;
			break;
		case MOVE_UP:
			double_pressed = (snake_current_direction == MOVE_UP);
			snake_current_direction = MOVE_UP;
			break;
		case MOVE_DOWN:
			double_pressed = (snake_current_direction == MOVE_DOWN);
			snake_current_direction = MOVE_DOWN;
			break;
		case MOVE_LEFT:
			double_pressed = (snake_current_direction == MOVE_LEFT);
			snake_current_direction = MOVE_LEFT;
			break;	
		case MOVE_RIGHT:
			double_pressed = (snake_current_direction == MOVE_RIGHT);
			snake_current_direction = MOVE_RIGHT;
			break;	
	}

}
void draw_all(){
	if(walls_are_enabled()) draw_walls();
	draw_gui();
	draw_apple();
	draw_snake();
	show_screen();
}
#define TIME_STEP (250)
#define TIME_DIVIDER (10)
void cycle(){
	if(double_pressed){
		_delay_ms(TIME_STEP/TIME_DIVIDER);
		double_pressed = false;
	}
	else
		_delay_ms(TIME_STEP);
}
bool is_move_button(int input){
	return input == MOVE_UP || input == MOVE_DOWN || input == MOVE_LEFT || input == MOVE_RIGHT;
}
int main(void){
	
	set_clock_speed(CPU_8MHz);

	srand(2);

	init_hardwear();
	init_snake();
	init_walls();
	init_apple();

	welcome_screen();

	while(true){
		spawn_snake();

		while(!is_game_over()){

			if(snake_current_direction == MOVE_WAIT){
				clear_screen();
				while(snake_current_direction == MOVE_WAIT){
					process_input(get_input());
					draw_all();
				}
				turn_snake(snake_current_direction);
				cycle();
				clear_screen();
			}


			process_input(get_input());
			
			turn_snake(snake_current_direction);

			step_snake();

			collision_detection();
			
			draw_all();

			cycle();

			clear_screen();

		}
		

	}

		

	return 0;
}