/*
 * Orbit.c
 *
 * Created: 5/31/2020 8:39:46 PM
 * Author : DeduA
 */

#include "kilolib/kilolib.h"

/* Macro-uri folosite pentru funcția set_motion */
#define STOP 0
#define FORWARD 1
#define LEFT 2
#define RIGHT 3

/* Variabile ce țin cont de direcția în care se deplasează robotul */
uint8_t current_motion = STOP;
uint8_t direction = LEFT;
uint8_t counterDirection = RIGHT;

/* Variabila reprezintă distanța la care se dorește să se facă orbitarea */
uint8_t orbitDistance = 40;

/* Variabile ce ține de mesajul recepționat */
message_t rcvd_message;
uint8_t new_message = 0;
uint8_t data = 0;
uint8_t dist;

/* Pentru transmisunea de mesaje */

message_t transmit_message;
uint8_t message_sent = 0;

message_t *message_tx()
{
	return &transmit_message;
}

void message_tx_success()
{
	message_sent = 1;
}

/* Sfarsit */

/* Funcția callback de recepționare a mesajelor */
void message_rx(message_t *msg, distance_measurement_t *dist_measure) {

	rcvd_message = *msg;
	data = msg->data[0];

	dist = estimate_distance(dist_measure);
	new_message = 1;
}

/* Funcție ce ușurează schimbările de direcție*/
void set_motion(int new_motion)
{
	if (current_motion != new_motion) {
		current_motion = new_motion;

		if (current_motion == STOP) {
			set_motors(0, 0);
		}
		else if (current_motion == FORWARD) {
			spinup_motors();
			set_motors(kilo_straight_left, kilo_straight_right);
		}
		else if (current_motion == LEFT) {
			spinup_motors();
			set_motors(kilo_turn_left, 0);
		}
		else if (current_motion == RIGHT) {
			spinup_motors();
			set_motors(0, kilo_turn_right);
		}
	}
}


/* Pentru robotii de tip beacon */

void setup1()
{
	transmit_message.type = NORMAL;
	transmit_message.data[0] = 1;
	transmit_message.crc = message_crc(&transmit_message);
}

void loop1() {
	if(message_sent == 1) {
		message_sent = 0;
		set_color(RGB(1, 0, 0));
		delay(100);
		set_color(RGB(0, 0, 0));
	}
}

/* Sfarsit */

/* Pentru robotii care se deplaseaza */

void setup2() {}

void loop2() {
	if(new_message) {
		new_message = 0;

		/* Dacă roboții orbitează prea aproape se impune depărtarea de beacon */
		if(dist < orbitDistance) {
			set_motion(counterDirection);
			set_color(RGB(2,0,0));
		}
		/* Dacă roboții orbitează prea departe se impune apropierea de beacon */
		else if(dist >= orbitDistance) {
			set_motion(direction);
			set_color(RGB(0,0,2));
		}
	}
}

/* Sfarsit */

int main(void)
{
	kilo_init();
	/* Roboții cu ID-ul 3, 9 și 2 vor vi beacon*/
	if(kilo_uid == 3 || kilo_uid == 9 || kilo_uid == 2) {
		/* Robotii beacon */
		kilo_message_tx = message_tx;
		kilo_message_tx_success = message_tx_success;
		kilo_start(setup1, loop1);
	}
	/* Restul roboților vor orbina în jurul celor de tip beacon */
	else {
		/* Robotii care se deplaseaza */
		kilo_message_rx = message_rx;
		kilo_start(setup2, loop2);
	}
	return 0;
}
