/*
 * Sir_Indian.c
 *
 * Created: 6/1/2020 6:33:27 PM
 * Author : DeduA
 */

#include "kilolib/kilolib.h"

/* Macro-uri folosite pentru funcția set_motion */
#define STOP 0
#define FORWARD 1
#define LEFT 2
#define RIGHT 3

/* Macro ce reprezintă numărul de ticks dintr-o secundă */
#define SECOND 32

/* Macro ce reprezintă distanța la care se dorește să se facă apropierea prin
funcția goTo*/
#define GO_TO_DISTANCE 40

/* Macro-uri ce reprezintă distanțele față de care se verifică condiția de
poziție corectă */
#define IN_POSITION_DISTANCE 35
#define IN_POSITION_DISTANCE2 60

/* Macro-uri folosite în algoritmul de urmărire a conturului */
#define ORBIT_DISTANCE 37
#define ORBIT_DELAY 400

/* Variabile ce ține de mesajul recepționat */
message_t rcvd_message;
uint8_t new_message = 0;
uint8_t data1 = 0;
uint8_t data2 = 0;
uint8_t dist = 0;

/* Este o variabilă în care se stochează valoarea distanței
pentru a evita probleme când se execută o instrucțiune ce folosește distanța, iar
în același timp se intră în funcția callback de recepționare de mesaje și se
reactualizează valoarea acesteia */
uint8_t current_dist = 0;

/* Variabilă folosită pentru stocarea distanțelor anterioare necesare în
realizarea comparațiilor */
uint8_t last_dist = 255;

/* Variabile ce țin cont de direcția în care se deplasează robotul */
uint8_t current_motion = STOP;
uint8_t correct_motion = RIGHT;
uint8_t orbit_motion = RIGHT;
uint8_t orbit_counterMotion = LEFT;

/* Variabile ce stochează starea curentă în care se află cele două mașini cu
număr de stări finite */
uint8_t state = 1;
uint8_t goToState = 1;

/* Variabile folosite pentru algoritmul de urmărire a conturului */
uint8_t nextToId;
uint8_t nextToIdDistance;

/* Variabile semafor*/
uint8_t inPosition = 0;
uint8_t firstMove = 1;


/*Variabile ce memorează distanța față de roboții de interes */
uint8_t distanceTo1 = 255;
uint8_t distanceTo2 = 255;

/* Variabila ce stochează ID-ul găsit din șir și la care se dorește să se ajungă*/
uint8_t goToId = 0;

/* Variabila stochează ID-ul kilobot-ului, este asemănător cu kilo_uid, dar
într-o variantă mai ușor de modificatș*/
uint8_t ID;

/* Această variabilă reprezintă multiplicatorul folosit în efectuarea virajelor */
uint8_t turnMultiplier = 1;

/* Funcția callback de recepționare a mesajelor */
void message_rx(message_t *msg, distance_measurement_t *dist_measure) {

	rcvd_message = *msg;
	data1 = msg->data[0];
	data2 = msg->data[1];

	dist = estimate_distance(dist_measure);

	if(data1 == ID - 1) {
		distanceTo1 = dist;
		/* Dacă este îndeplinită prima condiție de poziționare robotul semnalează
		prin aprinderea intermitentă a LED-ului RGB în culoarea albastră*/
		if(dist < IN_POSITION_DISTANCE) {
			set_color(RGB(0,0,1));
			delay(100);
			set_color(RGB(0,0,0));
			delay(100);
		}
	}

	else if(data1 == ID - 2) {
		distanceTo2 = dist;
		/* Dacă este îndeplinită a doua condiție de poziționare robotul semnalează
		prin aprinderea intermitentă a LED-ului RGB în culoarea verde*/
		if(dist > IN_POSITION_DISTANCE2) {
			set_color(RGB(0,1,0));
			delay(100);
			set_color(RGB(0,0,0));
			delay(100);
		}

	}
	new_message = 1;
}

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

/* Sfărșit */

/* Funcție ce ușurează schimbările de direcție*/
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

/* Kilobot-ul are o probabilitate de 50% pentru a merge în față, de 25% pentru
un viraj la stânda și tot 25% pentru un viraj la dreapta*/
void move_random() {

	int random_number = rand_hard();
	int random_direction = (random_number % 4);

	if (random_direction == 0 || random_direction == 1) {
		set_motion(FORWARD);
	}

	else if (random_direction == 2){
		set_motion(LEFT);
	}

	else if (random_direction == 3) {
		set_motion(RIGHT);
	}
}

/* Funcția încearcă să apropie kilobot-ul de robotul cu ID-ul transmis ca parametru la distanța
trimisă ca parametru. Implementarea este sub forma unei mașini cu număr finit de stări */
void goTo(uint8_t id, uint8_t distance) {
	if(new_message && data1 == id) {
		new_message = 0;
		current_dist = dist;

		if(current_dist <= distance) {
			firstMove = 1;
			inPosition = 1;
		}
		else {

			/* În starea 1 robotul se mișcă în față atâta timp cât distanța scade */
			if(goToState == 1) {

				if(current_dist > last_dist) {
					/* Dacă distanța crește se trece în starea 2*/
					goToState = 2;
				}
				set_motion(FORWARD);
				delay(1000);
				set_color(RGB(1,0,0));

			}

			/* În stare 2 se încearcă o mișcare de rotație în direcția considerată corectă
			iar apoi se trece în starea 3*/
			else if(goToState == 2) {
				set_motion(STOP);
				set_color(RGB(0,0,1));
				set_motion(correct_motion);
				delay(turnMultiplier * 1500);
				turnMultiplier = 1;
				set_motion(FORWARD);
				delay(1500);
				goToState = 3;
			}

			/* În starea 3 se verifică dacă mișcarea din starea 2 a fost corectă.
			Dacă a fost mașina va trece în starea 1, dacă nu, se va schimba direcția
			considerată corectă și se va trece în starea 2 */
			else if(goToState == 3) {
				set_color(RGB(1,1,0));
				if(current_dist > last_dist) {
					if(correct_motion == RIGHT)
						correct_motion = LEFT;
					else if(correct_motion == LEFT)
						correct_motion = RIGHT;
					turnMultiplier = 3;
					goToState = 2;
				}
				else {
					goToState = 1;
				}
			}
		}

		last_dist = current_dist;
		set_motion(STOP);
	}
}

/* Pentru robotul de tip beacon care arată de unde se va forma șirul */

void setup1() {
	transmit_message.type = NORMAL;
	transmit_message.data[0] = ID;
	transmit_message.data[1] = 1;
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

/* Sfârșit */

/* Pentru roboții care vor forma linia */

void setup2() {
	transmit_message.type = NORMAL;
	transmit_message.data[0] = ID;
	transmit_message.data[1] = 0;
	transmit_message.crc = message_crc(&transmit_message);
}


/* Roboții folosiți pentru formarea liniei au implementată o mașină cu număr finit
de stări */
void loop2() {

	/* Starea 1 reprezintă mișcarea aleatoare a roboților până când găsește un
	mesaj de la un robot din linie */
	if(state == 1) {
		move_random();
		state = 2;
	}

	/* În starea 2 se verifică dacă s-a recepționat un mesaj de la robotul care
	are ID-ul mai mic cu 1 decât cel al robotului curent.*/
	else if(state == 2) {
		set_color(RGB(1,0,1));
		/* Dacă acel robot nu a ajuns încă în linie se oprește mișcarea și se așteaptă
		ca acesta să ajungă în poziția corectă */
		if(new_message && data1 == ID - 1 && data2 == 0) {
			set_motion(STOP);
			new_message = 0;
		}

		/* După ce ajunge în linie robotul semnalează și permite trecerea robotului
		curent în starea 3 */
		else if(new_message && data1 == ID - 1 && data2 == 1) {
			state = 3;
			new_message = 0;
		}
	}

	/* În starea 3 kilobot-ul încearcă să se apropie de linie prin apelul funcției goTo */
	else if(state == 3) {
		set_color(RGB(0,1,1));
		if(!inPosition) {

			if (new_message && data1 != ID - 1 && data2 == 1 && dist < GO_TO_DISTANCE) {
				new_message = 0;
				state = 4;
				nextToId = data1;
				nextToIdDistance = dist;
			}
			else {
				if(ID == 2)
					goTo(ID - 1, IN_POSITION_DISTANCE);
				else
					goTo(ID - 1, GO_TO_DISTANCE);
			}
		}

		else {
			if(ID == 2) {
				/* Pentru kilobot-ul cu ID-ul 2 se sare direct la starea 5,
				deoarece acesta nu trebuie să urmărească conturul liniei, el
				fiind al doilea robot care formează linia */
				state = 5;
			}
			/* După ce s-a ajuns lângă linie se trece în starea 4 */
			else if (new_message && data1 == goToId) {
				nextToId = goToId;
				nextToIdDistance = dist;
				state = 4;
				firstMove = 1;
			}
		}
	}

	/* În starea 4 robotul urmărește conturul liniei până când ajunge în poziția potrivită */
	else if(state == 4) {

		set_color(RGB(0,1,1));
		set_motion(STOP);

		if (new_message && data1 != nextToId && dist < nextToIdDistance){
			nextToId = data1;
			new_message = 0;
		}

		else if(new_message && data1 == nextToId) {

			new_message = 0;
			current_dist = dist;
			nextToIdDistance = current_dist;

			if(current_dist < ORBIT_DISTANCE) {
				set_motion(orbit_motion);
				delay(ORBIT_DELAY);
			}
			else if(current_dist >= ORBIT_DISTANCE ) {
				set_motion(orbit_counterMotion);
				delay(ORBIT_DELAY);
			}

		}

		/* Aceasta este condiția care verifică dacă s-a ajuns în poziția corectă */
		if(distanceTo1 < IN_POSITION_DISTANCE && distanceTo2 >= IN_POSITION_DISTANCE2) {
			set_motion(STOP);
			state = 5;
		}
		new_message = 0;
	}

	/* În starea 5 robotul transmite mesajul cum că a ajuns în poziția corectă
	și trece în starea 6 care reprezintă ieșirea din mașina de stări finite și
	plasează kilobot-ul în repaus */
	else if(state == 5) {
		set_motion(STOP);
		set_color(RGB(1,1,1));
		transmit_message.data[1] = 1;
		transmit_message.crc = message_crc(&transmit_message);
		state = 6;
	}
}

int main(void)
{
	kilo_init();
	ID = kilo_uid - 1;
	kilo_message_rx = message_rx;
	kilo_message_tx = message_tx;
	kilo_message_tx_success = message_tx_success;
	/* Kilobot-ul cu ID-ul va marca unde se formează șirul */
	if(ID == 1)
		kilo_start(setup1, loop1);
	else
		kilo_start(setup2, loop2);
}
