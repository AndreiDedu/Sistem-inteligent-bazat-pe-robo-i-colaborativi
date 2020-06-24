/*
 * Gather_2_Grupuri.c
 *
 * Created: 5/31/2020 4:56:12 AM
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

 /* Valoarea de prag a distanței pentru care se
 consideră că un kilobot este în poziția corectă */
 uint8_t inPositionDistance = 33;

 /* Variabile folosite pentru contorizarea mesajelor recepționate */
 message_t rcvd_message;
 uint8_t new_message1 = 0;
 uint8_t new_message2 = 0;

 /* Variabile ce vor stoca datele primite și distanța față de transmițător */
 uint8_t data = 0;
 uint8_t dist = 0;

 /* Variabile folosite pentru stocarea distanțelor anterioare necesare în
 realizarea comparațiilor */
 uint8_t last_dist = 255;
 uint8_t last_dist2 = 255;

 /* Variabile semafor*/
 uint8_t isInPosition = 0;
 uint8_t mayGetOutOfRange = 0;
 uint8_t firstTimeInposition = 1;
 uint8_t firstMove = 1;
 uint8_t startingMove = 1;

 /* Variabile ce țin cont de direcția în care se deplasează robotul */
 uint8_t current_motion = STOP;
 uint8_t correct_motion = RIGHT;

 /* Variabile ce reprezintă numărul de ticks de când nu s-a mai primit un mesaj*/
 uint32_t last_update1 = 0;
 uint32_t last_update2 = 0;

 /* Variabilă ce ține cont de starea în care se află mașina cu stări finite
 din funcția move_to_beacon */
 uint8_t state = 1;


 /* Această variabilă reprezintă multiplicatorul folosit în efectuarea virajelor */
 uint8_t turnMultiplier = 1;

 /* Pentru transmisunea de mesaje */

 message_t transmit_message;
 uint8_t message_sent = 0;

 message_t *message_tx() {
 	return &transmit_message;
 }

 void message_tx_success() {
 	message_sent = 1;
 }

 /* Sfarsit */

 /* Funcție ce ușurează schimbările de direcție*/
 void set_motion(int new_motion)
 {
 	if (current_motion != new_motion) {
 		current_motion = new_motion;

 		if (current_motion == STOP)
 			set_motors(0, 0);
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

 /* Funcția callback de recepționare a mesajelor */
 void message_rx(message_t *msg, distance_measurement_t *dist_measure) {

 	rcvd_message = *msg;
 	data = msg->data[0];

 	dist = estimate_distance(dist_measure);

 	new_message1 = 1;
 	new_message2 = 1;

 }

 /*Funcția ce efectuează apropierea de beacon. Aceasta se apelează de fiecare
 dată când există o valoare nouă pentru distanța dintre roboți.
 Implementarea este făcută sub formă unei mașini cu număr finit de stări.*/
 void move_to_beacon() {

 	/* În starea 1 robotul se mișcă în față atâta timp cât distanța scade */
 	if(state == 1) {
 		if(firstMove) {
 			firstMove = 0;
 			set_motion(FORWARD);
 			set_color(RGB(1,0,0));
 		}

 		/* Dacă distanța începe să crească se va trece în starea 2 */
 		else if (dist > last_dist) {
 			set_motion(STOP);
 			set_color(RGB(0,0,1));
 			state = 2;
 			firstMove = 1;
 		}
 	}

 	/* În starea 2 robotul încearcă să găsească direcția în care se găsește beacon-ul*/
 	else if(state == 2) {
 		/* Prima dată robotul încearcă să se rotească în direcția considerată corectă
 		și să meargă puțin în față pentru a putea compara distanțele */
 		if(firstMove) {
 			last_dist2 = dist;
 			set_motion(correct_motion);
 			set_color(RGB(1,0,1));
 			mayGetOutOfRange = 1;
 			delay(turnMultiplier * 1500);
 			turnMultiplier = 1;
 			set_motion(FORWARD);
 			delay(2000);
 			firstMove = 0;
 		}

 		/* Dacă direcția nu este corectă se modifică direcția considerată corectă
 		și se efectuează o rotație mai îndelungată în această direcție pentru
 		a compensa pentru decizia eronată în primă instanță */
 		else if (firstMove == 0 && dist >= last_dist2) {
 			set_motion(STOP);

 			set_color(RGB(1,1,0));

 			if(correct_motion == RIGHT)
 				correct_motion = LEFT;
 			else if(correct_motion == LEFT)
 				correct_motion = RIGHT;

 				turnMultiplier = 3;
 				firstMove = 1;
 				mayGetOutOfRange = 0;

 		}

 		/* Dacă direcția este corectă se va trece în starea 1 */
 		else if (firstMove == 0 && dist < last_dist2) {
 			mayGetOutOfRange = 0;
 			state = 1;
 			firstMove = 1;
 		}

 	}

 	last_dist = dist;
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

 	/* Impune robotului să înceapă să se miște aleator dacă programul debea
 	și-a început execuția */
 	if (startingMove) {
 		move_random();
 		startingMove = 0;
 	}

 	if(new_message1 == 1) {

 		/* Dacă s-a recepționat mesajul de la un robot beacon
 		și distanța este mai mică decât cea de prag aleasă */
 		if(dist <= inPositionDistance && (data == 1 || data == 2)) {

 			set_motion(STOP);
 			set_color(RGB(1,1,1));
 			if(firstTimeInposition) {

 				/* Robotul începe să transmită și el mesaje specifice
 				roboților beacon */
 				transmit_message.type = NORMAL;
 				transmit_message.data[0] = 2;
 				transmit_message.crc = message_crc(&transmit_message);

 				firstTimeInposition = 0;
 				isInPosition = 1;
 			}

 		}
 		else {
 			/* Dacă s-a recepționat un mesaj de la beacon, dar distanța este
 			mai mare decât pragul ales */
 			if(data == 1 && !isInPosition)
 				move_to_beacon();
 		}
 		new_message1 = 0;
 	}

 	/* Dacă nu s-a recepționat un mesaj de la beacon de mai mult de 3 secunde
 	robotul încearcă să își găsească drumul înapoi spre beacon, iar dacă nu
 	reușește se mișcă random până primește un nou mesaj */
 	if (kilo_ticks > last_update2 + 3 * SECOND)
 	{
 		last_update2 = kilo_ticks;

 		if(new_message2)
 			new_message2 = 0;
 		else {

 			/* încercarea robotului de a reveni în aria de acțiune a beacon-ului*/
 			if (mayGetOutOfRange == 1) {
 				if (correct_motion == RIGHT)
 					correct_motion = LEFT;
 				else if (correct_motion == LEFT)
 					correct_motion = RIGHT;
 				set_motion(correct_motion);
 				delay(3*1500);
 				set_motion(FORWARD);
 				delay(2000);
 			}

 			/* Dacă nu s-a reușit întoarcerea în zona de acțiune, se resetează
 			variabilele folosite și se reia mișcarea aleatoare */
 			else {
 				transmit_message.type = NORMAL;
 				transmit_message.data[0] = 0;
 				transmit_message.crc = message_crc(&transmit_message);

 				correct_motion = RIGHT;
 				isInPosition = 0;
 				state = 1;
 				firstMove = 1;
 				last_dist = 255;
 				last_dist2 = 255;
 				move_random();
 				set_color(RGB(0,1,1));
 			}
 		}
 	}
 }

 /* Sfarsit */

 int main()
 {
 	kilo_init();
	/* Roboții cu ID-ul 3 și 9 vor fi beacon */
 	if(kilo_uid == 3 || kilo_uid == 9) {
 		/* Robotii beacon */
 		kilo_message_tx = message_tx;
 		kilo_message_tx_success = message_tx_success;
 		kilo_start(setup1, loop1);
 	}
	/* Restul roboților vor încerca să ajungă la beacon */
 	else {
 		/* Robotii care se deplaseaza */
 		kilo_message_rx = message_rx;
 		kilo_message_tx = message_tx;
 		kilo_message_tx_success = message_tx_success;
 		kilo_start(setup2, loop2);
 	}
 	return 0;
 }
