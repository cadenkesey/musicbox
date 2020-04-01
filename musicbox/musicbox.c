/**
	@file
	musicbox - a max object that outputs algorithmically generated music
	Caden Kesey
*/

#include "ext.h"
#include "ext_obex.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "D:/music_algorithm/hash.h"
#include "D:/music_algorithm/midi_notes.h"
#include "D:/music_algorithm/extra.h"

// OBJECT STRUCT

typedef struct _musicbox
{
	t_object p_ob; // The object itself

	// Outlets

	void* bass_outlet_length; // rightmost outlet
	void* bass_outlet_value;

	void* melody_outlet_length;
	void* melody_outlet_value;
	
	void* hat_outlet_length;
	void* hat_outlet_value;

	void* ghost_outlet_length;
	void* ghost_outlet_value;

	void* snare_outlet_length;
	void* snare_outlet_value;

	void* kick_outlet_length;
	void* kick_outlet_value; // leftmost outlet

	// Max clocks

	void* m_clock; // Master

	void* bass_clock;
	void* melody_clock;
	void* hat_clock;
	void* ghost_clock;
	void* snare_clock;
	void* kick_clock;

	// Linked lists

	note* bass_current;
	note* melody_current;
	note* hat_current;
	note* ghost_current;
	note* snare_current;
	note* kick_current;

	// Phrases

	phrase* bass_phrase;
	phrase* melody_phrase;
	phrase* hat_phrase;
	phrase* ghost_phrase;
	phrase* snare_phrase;
	phrase* kick_phrase;

	// Other variables

	long seed; // Seed for random number generation
	long tempo; // Tempo of the song
	float beat; // Beat length in milliseconds
	long runs; // Loop repetitions

	ht_t* hash_note_names; // Hashtable for getting Midi note values

} t_musicbox;

// FUNCTION PROTOTYPES

void musicbox_bang(t_musicbox* x);
void musicbox_in1(t_musicbox* x, long n);
void musicbox_in2(t_musicbox* x, long n);
void *musicbox_new(t_symbol *s, long argc, t_atom *argv);
void musicbox_free(t_musicbox *x);
void musicbox_assist(t_musicbox *x, void *b, long m, long a, char *s);
void musicbox_task(t_musicbox* x);
void musicbox_bass_task(t_musicbox* x);
void musicbox_melody_task(t_musicbox* x);
void musicbox_hat_task(t_musicbox* x);
void musicbox_ghost_task(t_musicbox* x);
void musicbox_snare_task(t_musicbox* x);
void musicbox_kick_task(t_musicbox* x);
void musicbox_create_phrase(t_musicbox* x, phrase* current_phrase, int* repeats, char** filename, int sections);
void musicbox_loadfile(t_musicbox* x, note* current_note, char* filename);
void musicbox_create_melody(t_musicbox* x, note* current_note, note* follow_beat);
void musicbox_create_melody_phrase(t_musicbox* x, phrase* current_phrase, phrase* follow_phrase);
void musicbox_create_bass(t_musicbox* x, note* current_note, note* follow_beat, int* scale);
void musicbox_create_bass_phrase(t_musicbox* x, phrase* current_phrase, phrase* follow_phrase, int** progression);

// GLOBAL CLASS POINTER VARIABLE

void *musicbox_class;

// MAIN METHOD

void ext_main(void *r)
{
	t_class *c;

	c = class_new("musicbox", (method)musicbox_new, (method)musicbox_free, (long)sizeof(t_musicbox),
				  0L /* leave NULL!! */, A_GIMME, 0);

	/* you CAN'T call this from the patcher */
	class_addmethod(c, (method)musicbox_bang, "bang", 0);

	class_addmethod(c, (method)musicbox_assist,			"assist",		A_CANT, 0);

	class_addmethod(c, (method)musicbox_in1, "in1", A_LONG, 0);
	class_addmethod(c, (method)musicbox_in2, "in2", A_LONG, 0);

	class_register(CLASS_BOX, c); /* CLASS_NOBOX */
	musicbox_class = c;

	post("Music box object has been loaded");
}

// OBJECT CREATION

void* musicbox_new(t_symbol* s, long argc, t_atom* argv)
{
	t_musicbox* x;
	x = (t_musicbox*)object_alloc(musicbox_class);

	// Inlets

	intin(x, 1);
	intin(x, 2);

	// Outlets

	x->bass_outlet_length = floatout(x);
	x->bass_outlet_value = intout(x);

	x->melody_outlet_length = floatout(x);
	x->melody_outlet_value = intout(x);

	x->hat_outlet_length = floatout(x);
	x->hat_outlet_value = intout(x);

	x->ghost_outlet_length = floatout(x);
	x->ghost_outlet_value = intout(x);

	x->snare_outlet_length = floatout(x);
	x->snare_outlet_value = intout(x);

	x->kick_outlet_length = floatout(x);
	x->kick_outlet_value = intout(x);

	// Clocks

	x->m_clock = clock_new((t_musicbox*)x, (method)musicbox_task);
	x->bass_clock = clock_new((t_musicbox*)x, (method)musicbox_bass_task);
	x->melody_clock = clock_new((t_musicbox*)x, (method)musicbox_melody_task);
	x->hat_clock = clock_new((t_musicbox*)x, (method)musicbox_hat_task);
	x->ghost_clock = clock_new((t_musicbox*)x, (method)musicbox_ghost_task);
	x->snare_clock = clock_new((t_musicbox*)x, (method)musicbox_snare_task);
	x->kick_clock = clock_new((t_musicbox*)x, (method)musicbox_kick_task);

	// Phrases & Linked Lists

	x->bass_phrase = (struct phrase*)malloc(sizeof(struct phrase));
	x->bass_phrase->head = (struct note*)malloc(sizeof(struct note));

	x->melody_phrase = (struct phrase*)malloc(sizeof(struct phrase));
	x->melody_phrase->head = (struct note*)malloc(sizeof(struct note));

	x->hat_phrase = (struct phrase*)malloc(sizeof(struct phrase));
	x->hat_phrase->head = (struct note*)malloc(sizeof(struct note));

	x->ghost_phrase = (struct phrase*)malloc(sizeof(struct phrase));
	x->ghost_phrase->head = (struct note*)malloc(sizeof(struct note));

	x->snare_phrase = (struct phrase*)malloc(sizeof(struct phrase));
	x->snare_phrase->head = (struct note*)malloc(sizeof(struct note));

	x->kick_phrase = (struct phrase*)malloc(sizeof(struct phrase));
	x->kick_phrase->head = (struct note*)malloc(sizeof(struct note));

	// Other variables

	x->tempo = 0;
	x->runs = 0;
	x->seed = 0;

	// Create hashtable for midi note values

	x->hash_note_names = ht_create();

	for (int i = 0; note_names[i]; ++i) {
		ht_set(x->hash_note_names, note_names[i], i+21);
	}
	ht_set(x->hash_note_names, "rest", -1);

	post("New music box object instance added to patch");
	return(x);
}

void musicbox_assist(t_musicbox* x, void* b, long m, long a, char* s)
{
	// Inlets

	if (m == ASSIST_INLET) {
		if (a == 0) {
			sprintf(s, "Start playing a sequence of notes");
		}
		else if (a == 1) {
			sprintf(s, "Seed");
		}
		else if (a == 2) {
			sprintf(s, "Tempo");
		}
	}

	// Outlets

	else {
		if (a == 0) {
			sprintf(s, "Kick note value");
		}
		if (a == 1) {
			sprintf(s, "Kick note length");
		}
		if (a == 2) {
			sprintf(s, "Snare note value");
		}
		if (a == 3) {
			sprintf(s, "Snare note length");
		}
		if (a == 4) {
			sprintf(s, "Ghost note value");
		}
		if (a == 5) {
			sprintf(s, "Ghost note length");
		}
		if (a == 6) {
			sprintf(s, "Hi-hat note value");
		}
		if (a == 7) {
			sprintf(s, "Hi-hat note length");
		}
		if (a == 8) {
			sprintf(s, "Melody note value");
		}
		if (a == 9) {
			sprintf(s, "Melody note length");
		}
		if (a == 10) {
			sprintf(s, "Bass note value");
		}
		if (a == 11) {
			sprintf(s, "Bass note length");
		}
	}
}

void musicbox_free(t_musicbox* x)
{
	object_free(x->m_clock);
	object_free(x->bass_clock);
	object_free(x->melody_clock);
	object_free(x->hat_clock);
	object_free(x->ghost_clock);
	object_free(x->snare_clock);
	object_free(x->kick_clock);
}

// INPUTS

void musicbox_bang(t_musicbox* x)
{
	// Unset all clocks

	clock_unset(x->m_clock);
	clock_unset(x->bass_clock);
	clock_unset(x->melody_clock);
	clock_unset(x->hat_clock);
	clock_unset(x->ghost_clock);
	clock_unset(x->snare_clock);
	clock_unset(x->kick_clock);

	//Set random number seed

	srand(x->seed);

	// Load instrument

	char* f_hat = "D:/music_algorithm/patterns/hat.txt";
	char* f_hat2 = "D:/music_algorithm/patterns/hat2.txt";
	char* sections_hat[2] = {f_hat, f_hat2};
	int repeats_hat[2] = {2,2};
	musicbox_create_phrase(x, x->hat_phrase, repeats_hat, sections_hat, 2);

	char* f_ghost = "D:/music_algorithm/patterns/ghost.txt";
	char* sections_ghost[1] = { f_ghost };
	int repeats_ghost[1] = { 4 };
	musicbox_create_phrase(x, x->ghost_phrase, repeats_ghost, sections_ghost, 1);

	char* f_snare = "D:/music_algorithm/patterns/snare.txt";
	char* sections_snare[1] = { f_snare };
	int repeats_snare[1] = { 4 };
	musicbox_create_phrase(x, x->snare_phrase, repeats_snare, sections_snare, 1);

	char* f_kick = "D:/music_algorithm/patterns/kick.txt";
	char* sections_kick[1] = {f_kick};
	int repeats_kick[1] = {4};
	musicbox_create_phrase(x, x->kick_phrase, repeats_kick, sections_kick, 1);

	int** progression = load_chords("D:/music_algorithm/patterns/chords.txt");

	musicbox_create_bass_phrase(x, x->bass_phrase, x->kick_phrase, progression);

	musicbox_create_melody_phrase(x, x->melody_phrase, x->snare_phrase);

	// Play song

	x->runs = 4;
	clock_fdelay(x->m_clock, 0.);
}

void musicbox_in1(t_musicbox* x, long n)
{
	x->tempo = n;
	x->beat = tempo_to_mil(x->tempo);
}

void musicbox_in2(t_musicbox* x, long n)
{
	x->seed = n;
}

// CLOCK TASKS

void musicbox_task(t_musicbox* x)
{
	/*double time;
	clock_getftime(&time);
	post("Time: %f", time);*/

	x->bass_phrase = next_phrase(x->bass_phrase);
	x->bass_current = x->bass_phrase->head;
	
	x->melody_phrase = next_phrase(x->melody_phrase);
	x->melody_current = x->melody_phrase->head;

	x->hat_phrase = next_phrase(x->hat_phrase);
	x->hat_current = x->hat_phrase->head;

	x->ghost_phrase = next_phrase(x->ghost_phrase);
	x->ghost_current = x->ghost_phrase->head;

	x->snare_phrase = next_phrase(x->snare_phrase);
	x->snare_current = x->snare_phrase->head;

	x->kick_phrase = next_phrase(x->kick_phrase);
	x->kick_current = x->kick_phrase->head;

	if (x->runs > 0) {
		post("Run: %ld", x->runs);
		clock_fdelay(x->m_clock, 4*(double)x->beat);

		clock_fdelay(x->bass_clock, .0);
		clock_fdelay(x->melody_clock, .0);
		clock_fdelay(x->hat_clock, .0);
		clock_fdelay(x->ghost_clock, .0);
		clock_fdelay(x->snare_clock, .0);
		clock_fdelay(x->kick_clock, .0);
		x->runs -= 1;
	}
}

void musicbox_bass_task(t_musicbox* x) {
	note* current = x->bass_current;
	outlet_int(x->bass_outlet_value, current->value);
	outlet_float(x->bass_outlet_length, current->length * (double)x->beat);
	if (current->next->value != NULL) {
		clock_fdelay(x->bass_clock, current->length * (double)x->beat);
		x->bass_current = current->next;
	}
}

void musicbox_melody_task(t_musicbox* x) {
	note* current = x->melody_current;
	outlet_int(x->melody_outlet_value, current->value);
	outlet_float(x->melody_outlet_length, current->length * (double)x->beat);
	if (current->next->value != NULL) {
		clock_fdelay(x->melody_clock, current->length * (double)x->beat);
		x->melody_current = current->next;
	}
}

void musicbox_hat_task(t_musicbox* x) {
	note* current = x->hat_current;
	outlet_int(x->hat_outlet_value, current->value);
	outlet_float(x->hat_outlet_length, current->length * (double)x->beat);
	if (current->next->value != NULL) {
		clock_fdelay(x->hat_clock, current->length * (double)x->beat);
		x->hat_current = current->next;
	}
}

void musicbox_ghost_task(t_musicbox* x) {
	note* current = x->ghost_current;
	outlet_int(x->ghost_outlet_value, current->value);
	outlet_float(x->ghost_outlet_length, current->length * (double)x->beat);
	if (current->next->value != NULL) {
		clock_fdelay(x->ghost_clock, current->length * (double)x->beat);
		x->ghost_current = current->next;
	}
}

void musicbox_snare_task(t_musicbox* x) {
	note* current = x->snare_current;
	outlet_int(x->snare_outlet_value, current->value);
	outlet_float(x->snare_outlet_length, current->length * (double)x->beat);
	if (current->next->value != NULL) {
		clock_fdelay(x->snare_clock, current->length * (double)x->beat);
		x->snare_current = current->next;
	}
}

void musicbox_kick_task(t_musicbox* x) {
	note* current = x->kick_current;
	outlet_int(x->kick_outlet_value, current->value);
	outlet_float(x->kick_outlet_length, current->length * (double)x->beat);
	if (current->next->value != NULL) {
		clock_fdelay(x->kick_clock, current->length * (double)x->beat);
		x->kick_current = current->next;
	}
}

// Additional

void musicbox_create_phrase(t_musicbox* x, phrase* current_phrase, int* repeats, char** filename, int sections) {
	for (int i = 0; i < sections; i++) {
		musicbox_loadfile(x, current_phrase->head, *(filename + i));
		current_phrase->repetitions = *(repeats + i);

		current_phrase->next = (phrase*)malloc(sizeof(phrase));
		current_phrase->next->head = (note*)malloc(sizeof(note));
		current_phrase->next->repetitions = 0;
		current_phrase->next->next = NULL;
		current_phrase = current_phrase->next;
	}
}

void musicbox_loadfile(t_musicbox* x, note* current_note, char* filename) {
	FILE* fp;
	char str[MAXCHAR];

	int local_test = NULL;

	int line = 1;

	int rand_line = get_random(1, number_of_lines(filename));

	fp = fopen(filename, "r");
	if (fp == NULL) {
		post("Could not open file %s", filename);
	}
	else {
		while (fgets(str, MAXCHAR, fp) != NULL) {
			const char delim[2] = " ";
			char* token = strtok(str, delim);
			while (token != NULL) {
				if (token[strlen(token) - 1] == '\n') {
					token[strlen(token) - 1] = 0;
				}
				local_test = ht_get(x->hash_note_names, token);
				if (local_test == NULL) {
					//post("Can't find %s", token);
					//post("LINE & RAND: %ld %ld", line, rand_line);
					if (line == rand_line) {
						current_note->length = (float)strtof(token, (char**)NULL, 10);
						current_note->next = (note*)malloc(sizeof(note));
						current_note->next->value = NULL;
						current_note->next->length = 0;
						current_note->next->next = NULL;
						current_note = current_note->next;
					}
				}
				else {
					//post("%s = %d", token, local_test);
					//post("LINE & RAND: %ld %ld", line, rand_line);
					if (line == rand_line) {
						current_note->value = local_test;
					}
				}
				token = strtok(NULL, delim);
			}

			line++;
		}
		fclose(fp);
	}
	//printList(x->inst_a_head);
	//printList(x->inst_b_head);
}

void musicbox_create_melody(t_musicbox* x, note* current_note, note* follow_beat) {
	float* back_beat = get_beats(follow_beat); //Get beats to match to

	float current_beat = 0.0; // Current beat
	float rand_length = 0.0; // Current note length
	int rand_note = 0; // Current note value

	int scale[7] = {57, 59, 60, 62, 64, 65, 67};
	//int scale[3] = {57, 60, 64};

	//Generate melody
	while (current_beat < 4.0) {
		// Test to see if current note is on the back beat
		int on_back_beat = 0;
		int i = 0;
		while (*(back_beat + i) != -1) {
			if (current_beat == *(back_beat + i)) {
				on_back_beat = 1;
				break;
			}
			else if (current_beat < *(back_beat + i)) {
				break;
			}
			i++;
		}

		rand_length = ((float)get_random(1, 16)) / 4.0; //Get a random note length

		int rand_note_index = get_random(0, 6);

		int j = 0;
		if (on_back_beat == 1) { //On the back beat
			j = i+1;
			rand_note = scale[rand_note_index] + 12;
		}
		else { //Not on backbeat
			j = i;
			rand_note = scale[rand_note_index];
		}

		if (*(back_beat + j) == -1) { //No more back beats
			if ((rand_length + current_beat) > 4.0) {
				rand_length = 4.0 - current_beat;
			}
		}
		else { //Check for further beats
			while (*(back_beat + j) != -1) {
				if ((rand_length + current_beat) > * (back_beat + j)) {
					rand_length = *(back_beat + j) - current_beat;
					break;
				}
				j++;
			}
		}

		current_note->length = rand_length;
		current_note->value = rand_note;

		current_beat = current_beat + rand_length;

		current_note->next = (note*)malloc(sizeof(note));
		current_note->next->value = NULL;
		current_note->next->length = 0;
		current_note->next->next = NULL;
		current_note = current_note->next;
	}
}

void musicbox_create_melody_phrase(t_musicbox* x, phrase* current_phrase, phrase* follow_phrase) {
	for (int i = 0; i < 1; i++) {
		musicbox_create_melody(x, current_phrase->head, follow_phrase->head);
		current_phrase->repetitions = 4;

		current_phrase->next = (phrase*)malloc(sizeof(phrase));
		current_phrase->next->head = (note*)malloc(sizeof(note));
		current_phrase->next->repetitions = 0;
		current_phrase->next->next = NULL;
		current_phrase = current_phrase->next;
	}
}

void musicbox_create_bass(t_musicbox* x, note* current_note, note* follow_beat, int* scale) {
	float* back_beat = get_beats(follow_beat); //Get beats to match to

	float current_beat = 0.0; // Current beat
	float rand_length = 0.0; // Current note length
	int rand_note = 0; // Current note value

	//int scale[7] = {57, 59, 60, 62, 64, 65, 67};
	//int scale[3] = {48, 52, 57};

	//Generate melody
	while (current_beat < 4.0) {
		// Test to see if current note is on the back beat
		int on_back_beat = 0;
		int i = 0;
		while (*(back_beat + i) != -1) {
			if (current_beat == *(back_beat + i)) {
				on_back_beat = 1;
				break;
			}
			else if (current_beat < *(back_beat + i)) {
				break;
			}
			i++;
		}

		rand_length = ((float)get_random(1, 16)) / 4.0; //Get a random note length

		int rand_note_index = get_random(1, 3);

		int j = 0;
		if (on_back_beat == 1) { //On the back beat
			j = i + 1;
			rand_note = *(scale + 0);
		}
		else { //Not on backbeat
			j = i;
			rand_note = *(scale + rand_note_index);
		}

		if (*(back_beat + j) == -1) { //No more back beats
			if ((rand_length + current_beat) > 4.0) {
				rand_length = 4.0 - current_beat;
			}
		}
		else { //Check for further beats
			while (*(back_beat + j) != -1) {
				if ((rand_length + current_beat) > * (back_beat + j)) {
					rand_length = *(back_beat + j) - current_beat;
					break;
				}
				j++;
			}
		}

		current_note->length = rand_length;
		current_note->value = rand_note;

		current_beat = current_beat + rand_length;

		current_note->next = (note*)malloc(sizeof(note));
		current_note->next->value = NULL;
		current_note->next->length = 0;
		current_note->next->next = NULL;
		current_note = current_note->next;
	}
}

void musicbox_create_bass_phrase(t_musicbox* x, phrase* current_phrase, phrase* follow_phrase, int** progression) {
	for (int i = 0; i < 4; i++) {
		musicbox_create_bass(x, current_phrase->head, follow_phrase->head, *(progression + i));
		current_phrase->repetitions = 1;

		current_phrase->next = (phrase*)malloc(sizeof(phrase));
		current_phrase->next->head = (note*)malloc(sizeof(note));
		current_phrase->next->repetitions = 0;
		current_phrase->next->next = NULL;
		current_phrase = current_phrase->next;
	}
}