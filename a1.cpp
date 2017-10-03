#include <unistd.h>
#include <stdlib.h>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <math.h>
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/Shell.h>
#include <X11/Xlib.h> 
#include <sys/time.h>
#define PI 3.14159265
using namespace std;

#include "simon.h"

unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

struct XInfo {
	int screen;	
	Display* display;
	Window window; 
	GC gc;
};
struct Info {
	int w = 800;//default width
	int h = 400;//default height
	int d = 100;//dimention
	int n = 4;//default number of buttons
	string text_on_screen = "Press SPACE to play or q to exit. ";
};
XInfo xinfo;
Info info;
class Button {
public: 
	Button(int x, int y, int diameter, int attraction_x, int num, bool animate_on, bool attract_on):
	x(x), y(y), diameter(diameter), attraction_x(attraction_x), num(num), animate_on(animate_on), attract_on(attract_on){};//constructor
	void draw(){
		//draw a circle first
		if(animate_on){//if animation switch is on
			if(diameter - ring_diameter > 4){
				int line_w = (diameter - ring_diameter - 4)/2;
				XSetLineAttributes(xinfo.display, xinfo.gc, line_w, LineSolid, CapRound, JoinMiter);
				XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x + line_w/2 - diameter/2, y + line_w/2 - diameter/2, diameter - line_w, diameter - line_w, 0, 360*64);//if it's on, background always black
				XSetLineAttributes(xinfo.display, xinfo.gc, 1, LineSolid, CapRound, JoinMiter);
			}
			XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x - ring_diameter/2, y - ring_diameter/2, 
					ring_diameter, ring_diameter, 0, 360 * 64);
			XFillArc(xinfo.display, xinfo.window, xinfo.gc, x - ring_diameter/2, y - ring_diameter/2, 
					ring_diameter, ring_diameter, 0, 360 * 64);
		}else{
			if(sqrt(pow(x - mouse_x, 2) + pow(y - mouse_y, 2)) <= diameter/2){//if mouse moves inside
				XSetLineAttributes(xinfo.display, xinfo.gc, 4, LineSolid, CapRound, JoinMiter);
				if(attract_on){
					XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x - diameter/2, y - diameter/2 - 10 * sin (attraction_x*PI/180), 
					diameter, diameter, 0, 360 * 64);
				}else{
					XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x - diameter/2, y - diameter/2, 
					diameter, diameter, 0, 360 * 64);
				}
				XSetLineAttributes(xinfo.display, xinfo.gc, 1, LineSolid, CapRound, JoinMiter);
			}else{
				if(attract_on){
					XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x - diameter/2, y - diameter/2 - 10 * sin (attraction_x*PI/180), 
					diameter, diameter, 0, 360 * 64);
				}else{
					XDrawArc(xinfo.display, xinfo.window, xinfo.gc, x - diameter/2, y - diameter/2, 
					diameter, diameter, 0, 360 * 64);
				}
			}
			//draw the number inside the circle
			if(attract_on){
				XDrawImageString( xinfo.display, xinfo.window, xinfo.gc, x, y - 10 * sin (attraction_x*PI/180), to_string(num).c_str(), to_string(num).length());
			}else{
				XDrawImageString( xinfo.display, xinfo.window, xinfo.gc, x, y, to_string(num).c_str(), to_string(num).length());
			}
		}
		XFlush( xinfo.display );
	}
	void toggle(){
		animate_on = !animate_on;
		return;
	}
	void toggle_attraction(){
		attract_on = !attract_on;
		return;
	}
	void set_x (int new_x){
		x = new_x;
	}
	void set_y (int new_y){
		y = new_y;
	}
	void set_mouse_x (int new_mouse_x){
		mouse_x = new_mouse_x;
	}
	void set_mouse_y (int new_mouse_y){
		mouse_y = new_mouse_y;
	}
	void set_ring_diameter (int new_ring_diameter){
		ring_diameter = new_ring_diameter;
	}
	void set_attraction_x (int new_attraction_x){
		attraction_x = new_attraction_x;
	}
	int get_x () {
		return this->x;
	}
	int get_y () {
		return this->y;
	}
	int get_num () {
		return this->num;	
	}
	int get_diameter () {
		return this->diameter;
	}
	int get_attraction_x () {
		return this->attraction_x;
	}
	int get_ring_diameter () {
		return this->ring_diameter;
	}
	bool get_animation_cond () {
		return this->animate_on;
	}
private: 
	int x;//x and y are coordinates of wehere it should be drawed
	int y;
	int mouse_x;//coordinates of mouse position
	int mouse_y;
	int ring_diameter;
	bool animate_on;
	bool attract_on;
	int attraction_x; //always change 
	int diameter;
	int num;//the number in the button (1 - 6)
};


int FPS = 60;

XEvent event;	//my xevent: detect keypress
KeySym key;		/* a dealie-bob to handle KeyPress Events */	
XSetWindowAttributes attribute;
vector <Button *> buttons;

int calculate_x (int w, int d, int i, int n){
	return (i + 1) * (w - n * d)/(n + 1) + d * (2 * i + 1) / 2;
}
int calculate_y (int h, int d) {
	return (h - d) / 2 + d / 2;
}
void write_message (int x, int y, string text){//write press space to play, text always unchanged
	XDrawImageString( xinfo.display, xinfo.window, xinfo.gc, x, y, text.c_str(), text.length() );
	XFlush( xinfo.display );
}
void write_score (int x, int y, int score){//write current score, score may change
	XDrawImageString( xinfo.display, xinfo.window, xinfo.gc, x, y, to_string(score).c_str(), to_string(score).length()); 
	XFlush( xinfo.display );
}

void event_loop (Simon *simon, char mode, int temp_x, int temp_y) {//mode: m -> machine; h -> human s -> start
	unsigned long start = now();//start time when running event_loop, used for counting time and return
	char text_key_press[255];	//keypress text buffer
	// time of last window paint
	unsigned long lastRepaint = 0;
	if(mode=='m'){//if machine calls, we manually set up
		for(int i=0;i<buttons.size();i++){
		    int button_x = buttons[i]->get_x();
		    int button_y = buttons[i]->get_y();
		    int button_diameter = buttons[i]->get_diameter();
		    if(sqrt(pow(button_x - temp_x, 2) + pow(button_y - temp_y, 2)) <= button_diameter/2){
		    	buttons[i]->toggle();
		    	buttons[i]->set_ring_diameter(100);//initial ring diameter value
		    	break;
		    }
		}
	}
	while (true) {
		if(mode == 'm'){//means machine call
			if(now()-start>=1250000){//how many microseconds for one click animation
				return;
			}
		}
		if(XPending(xinfo.display) > 0){
			XNextEvent( xinfo.display, &event );
			switch (event.type){
				case KeyPress: 
					if(XLookupString(&event.xkey,text_key_press, 255, &key, 0)==1){
						if (text_key_press[0]=='q') {
							exit(0);
						}else if(text_key_press[0]==' '){
							simon->newRound();
							info.text_on_screen = "Watch what I do ...";
							return;//after new round, quit the event loop
						}
					}
					break;
				case ConfigureNotify:
					XClearWindow(xinfo.display, xinfo.window);
					write_message (100, 5 * info.h / 16, info.text_on_screen);
					write_score(100, info.h / 5, simon->getScore());
					for(std::vector<int>::size_type i = 0; i != buttons.size(); i++) {
					    buttons[i]->set_x(calculate_x(event.xconfigure.width, info.d, i, info.n));
					    buttons[i]->set_y(calculate_y(event.xconfigure.height, info.d));
					    buttons[i]->draw();
					}
					break;
				case MotionNotify://move mouse
		            for(std::vector<int>::size_type i = 0; i != buttons.size(); i++) {
		            	buttons[i]->set_mouse_x(event.xmotion.x);
		            	buttons[i]->set_mouse_y(event.xmotion.y);
		            }
		            XClearWindow(xinfo.display, xinfo.window);
					write_message (100, 5 * info.h / 16, info.text_on_screen);
					write_score(100, info.h / 5, simon->getScore());
		   			for(std::vector<int>::size_type i = 0; i != buttons.size(); i++) {
						buttons[i]->draw();
					}
					break;
				case ButtonPress: 
					int cx = event.xbutton.x;
					int cy = event.xbutton.y;
					if(mode == 'm'){//if it is a machine call
						cx = temp_x;
						cy = temp_y;
					}
					for(int i=0;i<buttons.size();i++){
		            	int button_x = buttons[i]->get_x();
		            	int button_y = buttons[i]->get_y();
		            	int button_diameter = buttons[i]->get_diameter();
		            	if(sqrt(pow(button_x - cx, 2) + pow(button_y - cy, 2)) <= button_diameter/2){
		            		buttons[i]->toggle();
		            		buttons[i]->set_ring_diameter(100);//initial ring diameter value
		            		if(mode == 'h'){
			            		if (!simon->verifyButton(i)) {
									cout << "wrong" << endl;
								}
								return;
							}
		            		break;
		            	}
		            }
		            break;
			}
		}

		unsigned long end = now();

		if(end - lastRepaint > 1000000/FPS) {
			XClearWindow(xinfo.display, xinfo.window);
			write_message (100, 5 * info.h / 16, info.text_on_screen);
			write_score(100, info.h / 5, simon->getScore());
	   		for(std::vector<int>::size_type i = 0; i != buttons.size(); i++) {
				buttons[i]->draw();
				if(buttons[i]->get_attraction_x() >= 360){
					buttons[i]->set_attraction_x(0);
				}else{
					buttons[i]->set_attraction_x(buttons[i]->get_attraction_x()+4);
				}
				if(buttons[i]->get_animation_cond()){
					int temp_ring_diameter = buttons[i]->get_ring_diameter();
					if(temp_ring_diameter<=0){
						buttons[i]->toggle();
					}else{
						buttons[i]->set_ring_diameter(temp_ring_diameter - 4);
					}
				}
			}
			lastRepaint = now(); 
		}

		if (XPending(xinfo.display) == 0) {
			usleep(1000000 / FPS - (end - lastRepaint));
		}
	}
}

int main ( int argc, char* argv[] ) {
	// get the number of buttons from args
	// (default to 4 if no args)
	xinfo.display = XOpenDisplay("");//open display
    if (!xinfo.display) exit (-1);//if err exit
    //set up window
    xinfo.screen = DefaultScreen(xinfo.display);
	xinfo.window = XCreateSimpleWindow(
		xinfo.display,
		DefaultRootWindow(xinfo.display), // window's parent
		10, 10, // location: x,y
		info.w, info.h, // size: width, height
		2, // width of border
		BlackPixel(xinfo.display, xinfo.screen), // foreground colour
		WhitePixel(xinfo.display, xinfo.screen)
	); // background colour
	XMapRaised(xinfo.display, xinfo.window); // put window on screen
	XFlush(xinfo.display); // flush the output buffer
	usleep(10 * 1000); 
	//set up
	xinfo.gc = XCreateGC(xinfo.display, xinfo.window, 0, 0);       // create a graphics context
    XSetForeground(xinfo.display, xinfo.gc, BlackPixel(xinfo.display, xinfo.screen));
    XSetBackground(xinfo.display, xinfo.gc, WhitePixel(xinfo.display, xinfo.screen));
    //set larger font
    XFontStruct * font;
 	font = XLoadQueryFont (xinfo.display, "12x24");
 	XSetFont (xinfo.display, xinfo.gc, font->fid);

    if (argc > 1) {
        info.n = atoi(argv[1]);
    }
    info.n = max(1, min(info.n, 9));
    // create the Simon game object
	Simon *simon = new Simon(info.n, true);

	cout << "Playing with " << simon->getNumButtons() << " buttons." << endl;

	//initial message function
	//std::string text_init_message("Press SPACE to play or q to exit. ");

	//init messgae
	write_message (100, 5 * info.h / 16, info.text_on_screen);
	write_score(100, info.h / 5, simon->getScore());
	//draw n buttons
    for(int i=0;i<info.n;i++){
    	int my_x = calculate_x(info.w, info.d, i, info.n);
    	int my_y = calculate_y(info.h, info.d);
    	Button *button = new Button(my_x, my_y, info.d, 360 * i/info.n, i+1, false, false);
    	button->draw();
    	buttons.push_back(button);
    }

	XFlush(xinfo.display);

	XSelectInput(xinfo.display, xinfo.window, PointerMotionMask | KeyPressMask 
		| StructureNotifyMask | ButtonMotionMask | PointerMotionMask | KeyPressMask
		| ButtonPressMask); 
	//game
	while (true) {
		//always turn the attraction button on at the very start
		for(std::vector<int>::size_type i = 0; i != buttons.size(); i++) {
			buttons[i]->toggle_attraction();
		}
		switch (simon->getState()) {
		// will only be in this state right after Simon object is contructed
		case Simon::START:
			info.text_on_screen = "Press SPACE to play or q to exit. ";
			break;
		// they won last round
		// score is increased by 1, sequence length is increased by 1
		case Simon::WIN:
			info.text_on_screen = "You won! Press SPACE to continue.";
			break;
		// they lost last round
		// score is reset to 0, sequence length is reset to 1
		case Simon::LOSE:
			info.text_on_screen =  "You lose. Press SPACE to play again.";
			break;
		default:
			// should never be any other state at this point ...
			break;
		}
		event_loop(simon, 's', 0, 0);//need the space or q event, so call event_loop
		// start new round with a new sequence
		//once user click space, turn all attraction buttons off
		for(std::vector<int>::size_type i = 0; i != buttons.size(); i++) {
			buttons[i]->toggle_attraction();
		}
		// computer plays
		cout << "Watch what I do ..." << endl;
		// just output the values 
		// (obviously this demo is not very challenging)
		while (simon->getState() == Simon::COMPUTER) {
			int btn_num =  simon->nextButton();
			cout << btn_num << endl;
			event_loop(simon, 'm', buttons[btn_num]->get_x(), buttons[btn_num]->get_y());//assume mouse click on the center point
		}

		// now human plays
		cout << "Your turn ..." << endl;
		info.text_on_screen = "Your turn ...";
		while (simon->getState() == Simon::HUMAN) {
			event_loop(simon, 'h', 0, 0);
		}

		// print the score
		cout << "score: " << simon->getScore() << endl;

	}
}

