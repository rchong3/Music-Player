#include <stdio.h>
#include "avr.h"
#include "lcd.h"

float freqs[] = {261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000, 466.164, 493.883};
const char period = 100; //ms
const char SONG_MAX = 120; //# of periods

void music_wait(unsigned short time) {
	TCCR2 = 3;
	while (--time) {
		TCNT2 = (unsigned char)(256 - (XTAL_FRQ / 32) * 0.001);
		SET_BIT(TIFR, TOV2);
		while (!GET_BIT(TIFR, TOV2));
	}
	TCCR2 = 0;
}

short is_pressed(const short r, const short c) {
	DDRC = 0;
	PORTC = 0;

	SET_BIT(DDRC, r);
	CLR_BIT(PORTC, r);
	SET_BIT(PORTC, c + 4);

	NOP();
	NOP();

	if (GET_BIT(PINC, c + 4) == 0) {
		return 1;
	}

	return 0;
}

ISR (TIMER1_COMPA_vect) {
	PORTA ^= (1 << PA0);
}

void set_note(const float freq) {
	TCNT1 = 0;
	OCR1A = XTAL_FRQ / freq / 2;
	sei();
}

void set_note2(const float freq) {
	if (freq > 0) {
		TCCR0 = 0x1B;
		TCNT0 = 0;
		OCR0 = XTAL_FRQ / 64 / freq / 2;
	} else {
		TCCR0 = 0;
	}
}

void print_main_menu(char* buf) {
	lcd_pos(0, 0);
	sprintf(buf, "Rec Play Up Down");
	lcd_puts(buf);
	lcd_pos(1, 0);
	sprintf(buf, " A   B   C   D  ");
	lcd_puts(buf);
}

void print_selection_menu(char* buf) {
	lcd_pos(0, 0);
	sprintf(buf, " <-   1   2   3 ");
	lcd_puts(buf);
	lcd_pos(1, 0);
	sprintf(buf, " A    B   C   D ");
	lcd_puts(buf);
}

void print_song_progress(char * buf, const short rem) {
	lcd_pos(0, 0);
	sprintf(buf, " Time remaining ");
	lcd_puts(buf);
	lcd_pos(1, 0);
	sprintf(buf, "   %2i seconds   ", rem / 10);
	lcd_puts(buf);
}

void update_song_progress(char * buf, const short rem) {
	lcd_pos(1, 3);
	sprintf(buf, "%2i", rem / 10);
	lcd_puts(buf);
}

int main(void) {
	SET_BIT(DDRA, PA0);
	SET_BIT(DDRB, PB3);

	CLR_BIT(TCCR1A, WGM10);
	CLR_BIT(TCCR1A, WGM11);
	SET_BIT(TCCR1B, WGM12);
	CLR_BIT(TCCR1B, WGM13);
	SET_BIT(TCCR1B, CS10);
	CLR_BIT(TCCR1B, CS11);
	CLR_BIT(TCCR1B, CS12);
	SET_BIT(TIMSK, OCIE1A);

	lcd_init();
	float songs[3][SONG_MAX];
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < SONG_MAX; ++j) {
			songs[i][j] = -1;
		}
	}
	char lengths[3];
	for (int i = 0; i < 3; ++i) {
		lengths[i] = 0;
	}
	float note = -1;
	char buf[17];
	char state = 0; //{free play, recording slot selection, replay slot selection, recording, replaying}
	char slot = 0;
	char pos = 0;
	char rem;

	print_main_menu(buf);

	for (;;) {
		for (short button = 0; button < 12; ++button) {
			float freq = freqs[button];
			if (is_pressed(button / 3, button % 3)) {
				if (note != freq) {
					note = freq;
					set_note(note);
				}
			} else if (note == freq) {
				note = -1;
				cli();
			}
		}
		
		switch (state) {
			case 0:
				if (is_pressed(0, 3)) {
					print_selection_menu(buf);
					state = 1;
					while (is_pressed(0, 3));
				} else if (is_pressed(1, 3)) {
					state = 2;
					print_selection_menu(buf);
					while (is_pressed(1, 3));
				} else if (is_pressed(2, 3)) {
					for (short i = 0; i < 12; ++i) {
						freqs[i] *= 2;
					}
					while (is_pressed(2, 3));
				} else if (is_pressed(3, 3)) {
					for (short i = 0; i < 12; ++i) {
						freqs[i] /= 2;
					}
					while (is_pressed(3, 3));
				}
				break;
			case 1:
				if (is_pressed(0, 3)) {
					state = 0;
					print_main_menu(buf);
					while (is_pressed(0, 3));
				} else if (is_pressed(1, 3)) {
					state = 3;
					slot = 0;
					pos = 0;
					print_song_progress(buf, SONG_MAX - pos);
					while (is_pressed(1, 3));
				} else if (is_pressed(2, 3)) {
					state = 3;
					slot = 1;
					pos = 0;
					print_song_progress(buf, SONG_MAX - pos);
					while (is_pressed(2, 3));
				} else if (is_pressed(3, 3)) {
					state = 3;
					slot = 2;
					pos = 0;
					print_song_progress(buf, SONG_MAX - pos);
					while (is_pressed(3, 3));
				}
				break;
			case 2:
				if (is_pressed(0, 3)) {
					state = 0;
					print_main_menu(buf);
					while (is_pressed(0, 3));
				} else if (is_pressed(1, 3)) {
					state = 4;
					slot = 0;
					pos = 0;
					print_song_progress(buf, lengths[slot] - pos);
					while (is_pressed(1, 3));
				} else if (is_pressed(2, 3)) {
					state = 4;
					slot = 1;
					pos = 0;
					print_song_progress(buf, lengths[slot] - pos);
					while (is_pressed(2, 3));
				} else if (is_pressed(3, 3)) {
					state = 4;
					slot = 2;
					pos = 0;
					print_song_progress(buf, lengths[slot] - pos);
					while (is_pressed(3, 3));
				}
				break;
			case 3:
				songs[slot][pos] = note;
				rem = SONG_MAX - ++pos;

				if (is_pressed(2, 3)) {
					for (short i = 0; i < 12; ++i) {
						freqs[i] *= 2;
					}
					while (is_pressed(2, 3));
				} else if (is_pressed(3, 3)) {
					for (short i = 0; i < 12; ++i) {
						freqs[i] /= 2;
					}
					while (is_pressed(3, 3));
				}

				if (rem <= 0 || is_pressed(0, 3)) {
					lengths[slot] = pos;
					state = 0;
					print_main_menu(buf);
					while (is_pressed(0, 3));
				} else if (rem % 10 == 0) {
					update_song_progress(buf, rem);
				}

				break;
			case 4:
				if (pos == 0 || songs[slot][pos] != songs[slot][pos-1]) {
					set_note2(songs[slot][pos]);
				}

				rem = lengths[slot] - ++pos;

				if (is_pressed(2, 3)) {
					for (short i = 0; i < 12; ++i) {
						freqs[i] *= 2;
					}
					while (is_pressed(2, 3));
				} else if (is_pressed(3, 3)) {
					for (short i = 0; i < 12; ++i) {
						freqs[i] /= 2;
					}
					while (is_pressed(3, 3));
				}

				if (rem <= 0 || is_pressed(0, 3)) {
					state = 0;
					print_main_menu(buf);
					set_note2(-1);
					while (is_pressed(0, 3));
				} else if (rem % 10 == 0) {
					update_song_progress(buf, rem);
				}
		}

		music_wait(period);
	}
}
