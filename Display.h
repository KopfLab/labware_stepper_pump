/*
 * Display.h
 * Created on: Feb 4, 2016
 * Author: Sebastian Kopf <seb.kopf@gmail.com>
 * Requires LiquidCrystal_I2C_Spark library from https://github.com/BulldogLowell/LiquidCrystal_I2C_Spark
 *
 * Convenience wrapper for LCD I2C display that allows easy displaying of temporary info msgs that show up in a reserverd part of the display.
 *
 * Create Display object with the lcd i2c address, the size of the display (columns and rows) and, optionally the width of space (in the last 2 lines) reserved for the temporary msgs.
 *
 * Most basic code example for a 20x4 display showing a 5s temporary msg at the beginning of startup:
 *
 * Display lcd (0x27, 20, 4, 7, 3000);
 * void setup(void) { lcd.init(); lcd.show_message("temp", "msg", 5000); }
 * void loop(void) { lcd.update(); }
 *
 */

#pragma once

// requirement: https://github.com/BulldogLowell/LiquidCrystal_I2C_Spark
#include "LiquidCrystal_I2C_Spark.h" // Particle Dev (folder structure does not matter, everything gets flattened)
// #include "LiquidCrystal_I2C_Spark/LiquidCrystal_I2C_Spark.h" // script in particle cloud

// PIN definitions
#define LCDData			4 // I2C SDA, physicially this is A4 on Arduino Uno, 20 on Mega and D0 on photon
#define LCDClock		5 // I2C SCL, physically this is A5 on Arduino Uno, 21 on Mega and D1 on photon

// Default msg show time
#define INFO_MSG_DEFAULT_SHOW_TIME 2000 //ms

// Display class handles displaying information
class Display : public LiquidCrystal_I2C {
public:
	int cols, rows; // display layout
	const char space[1] = {' '};
	const int info_msg_default_show_time;

	int info_width;			// width of info msgs
	int info_msg_show_time;		// how long current info msg is being shown for (in ms), 0 = no msg shown
	long info_msg_show_start;	// when the last info msg was started

	Display (int lcd_Addr, int lcd_cols, int lcd_rows) :
		   Display(lcd_Addr, lcd_cols, lcd_rows, 0) {}

	Display (int lcd_Addr, int lcd_cols, int lcd_rows, int info_width) :
 		   Display(lcd_Addr, lcd_cols, lcd_rows, info_width, INFO_MSG_DEFAULT_SHOW_TIME) {}

	 Display (int lcd_Addr, int lcd_cols, int lcd_rows, int info_width, int info_msg_default_show_time) :
 		   LiquidCrystal_I2C(lcd_Addr, lcd_cols, lcd_rows), cols(lcd_cols), rows(lcd_rows),
			 info_width(info_width), info_msg_default_show_time(info_msg_default_show_time),
			 info_msg_show_time(0), info_msg_show_start(0) {}


	// convenience functions for writing text
	// clears the line (starting from the beginning of the text) and the writes the String over it
	// both line and start are indexed from 1
	void init ();
	void go_to_line (int line);
	void clear_line (int line);
	void clear_line (int line, int start);
	void clear_line (int line, int start, int end);
	void print_line (int line, const String &text);
	void print_line (int line, const String &text, int start);
	void print_line (int line, const String &text, int start, int end);
	void print_line_right (int line, const String &text);

	void clear_screen(); // clears everything except the msgs
	void clear_screen(int start_line); // clears everything except the msgs
	void show_message (const String &line1, const String &line2);
	void show_message (const String &line1, const String &line2, int info_msg_show_time);
	void clear_msg ();
	void update();
};


void Display::init() {
	LiquidCrystal_I2C::init();
	backlight();
	clear();
}

void Display::go_to_line(int line) {
	setCursor(0, line - 1);
}

void Display::clear_line(int line) {
	clear_line(line, 1);
}

void Display::clear_line(int line, int start) {
	clear_line(line, start, cols);
}

void Display::clear_line(int line, int start, int end) {
	setCursor(start - 1, line - 1);
	for (int i = start - 1; i < end; i++) {
		print(space[0]);
	}
	setCursor(start - 1, line - 1);
}

void Display::print_line (int line, const String &text, int start, int end) {
	// overwrite existing text in the line
	clear_line(line, start, end);

	// truncate msg if too long
	print(text.substring(0, end - start + 1));
}

void Display::print_line (int line, const String &text, int start) {
	if (line == rows - 1 || line == rows) {
		print_line(line, text, 1, cols - info_width);
	} else {
		print_line(line, text, 1, cols);
	}
}

void Display::print_line (int line, const String &text) {
	print_line(line, text, 1);
}

void Display::print_line_right (int line, const String &text) {
	print_line(line, text, cols - text.length() + 1, cols);
}

void Display::clear_screen() {
	clear_screen(1);
}

void Display::clear_screen(int start_line) {
	for (int i = start_line; i <= rows; i++) {
		print_line(i, "");
	}
}

void Display::show_message (const String &line1, const String &line2) {
	show_message(line1, line2, info_msg_default_show_time);
}

void Display::show_message (const String &line1, const String &line2, int show_time) {
	clear_msg();
	info_msg_show_start = millis();
	info_msg_show_time = show_time;
	print_line_right(rows - 1, line1.substring(0, info_width));
	print_line_right(rows, line2.substring(0, info_width));
}

void Display::clear_msg () {
	clear_line(rows - 1, cols - info_width + 1);
	clear_line(rows, cols - info_width + 1);
	info_msg_show_time = 0;
}

void Display::update() {
	if (info_msg_show_time > 0 && (millis() - info_msg_show_start) > info_msg_show_time) {
		clear_msg();
	}
}
