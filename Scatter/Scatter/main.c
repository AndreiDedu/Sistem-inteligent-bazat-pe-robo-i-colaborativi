/*
 * Scatter.c
 *
 * Created: 5/26/2020 6:49:22 PM
 * Author : DeduA
 */

#include "kilolib/kilolib.h"

/* Macro-uri folosite pentru funcția set_motion */
#define STOP 0
#define FORWARD 1
#define LEFT 2
#define RIGHT 3

/* Macro-ul reprezintă valoarea de prag a distanței*/
#define Distance_Threshold 75

/* Variabile folosite în transmiterea și recepționarea de mesaje */
message_t tx, rx;
int message_sent = 0, message_rcved = 0;
uint8_t rcvd_data = 0;
int dist = 0;

/* Variabilă ce arată numărul de ticks în momentul în care s-a primit
ultimul mesaj */
uint32_t last_changed = 0;

/* Variabile necesare pentru generarea de numere aleatoare */
int random_number = 0;
int dice = 0;

/* Funcție ce ușurează schimbările de direcție*/
int current_motion = STOP;
void set_motion(int new_motion) {
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

/* Funcția callback de recepționare a mesajelor */
void message_rx(message_t *msg, distance_measurement_t *dist_measure) {
	rx = *msg;
	rcvd_data = msg->data[0];
	dist = estimate_distance(dist_measure);
	/* Mesajul se ia în considerare doar dacă distanța este mai mică ca pragul ales */
	if(dist < Distance_Threshold) {
		message_rcved = 1;
	}
}


/* Pentru transmisunea de mesaje */
message_t *message_tx()
{
	return &tx;
}

void message_tx_success()
{
	message_sent = 1;
}

/* Sfârșit */

void setup() {
	tx.type = NORMAL;
	tx.data[0] = kilo_uid;
	tx.crc = message_crc(&tx);
}

void loop() {

	/* Se vefică în fiecare secundă dacă s-a recepționat un mesaj nou */
	if(kilo_ticks > (last_changed + 32)) {
		last_changed = kilo_ticks;
		/* Se verifică dacă s-a primit un mesaj nou, dacă s-a primit înseamnă ca kilobot-ul
		trebuie să se deplaseze */
		if(message_rcved == 1) {
			message_rcved = 0;

			/* Se generează un număr aleator pe baza căruia se decide în ce direcție se va merge.
			Este o probabilitate de 50% ca roboul să meargă în față, 25% ca robotul să meargă în
			stânga și tot 25% ca robotul să meargă la dreapta */
			random_number = rand_hard();
			dice = random_number % 4;

			if (dice == 0 || dice == 1) {
				set_motion(FORWARD);
				set_color(RGB(1,0,0));
			} else if (dice == 2) {
				set_motion(LEFT);
				set_color(RGB(0,0,1));
			} else if (dice == 3) {
				set_motion(RIGHT);
				set_color(RGB(0,1,1));;
			} else {
				set_motion(STOP);
				set_color(RGB(0,0,0));
			}
		}
		/* Dacă nu s-a primit înseamnă că robotul nu mai are niciun alt kilobot
		în aria definită*/
		else {
			set_motion(STOP);
			set_color(RGB(0,1,0));
		}
	}
}

int main(void)
{
	kilo_init();
	kilo_message_tx = message_tx;
	kilo_message_tx_success = message_tx_success;
	kilo_message_rx = message_rx;

	kilo_start(setup,loop);
}
