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

	void* piano_outlet_length; // rightmost outlet
	void* piano_outlet_value_1;
	void* piano_outlet_value_2;
	void* piano_outlet_value_3;
	void* piano_outlet_value_4;

	void* bass_outlet_length;
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
	void* measure_clock;

	void* piano_clock;
	void* bass_clock;
	void* melody_clock;
	void* hat_clock;
	void* ghost_clock;
	void* snare_clock;
	void* kick_clock;

	// Linked lists

	note* piano1_current;
	note* piano2_current;
	note* piano3_current;
	note* piano4_current;

	note* bass_current;
	note* melody_current;
	note* hat_current;
	note* ghost_current;
	note* snare_current;
	note* kick_current;

	// Phrases

	phrase* piano1_phrase;
	phrase* piano2_phrase;
	phrase* piano3_phrase;
	phrase* piano4_phrase;
	
	phrase* bass_phrase;
	phrase* melody_phrase;
	phrase* hat_phrase;
	phrase* ghost_phrase;
	phrase* snare_phrase;
	phrase* kick_phrase;

	// Sections

	section* piano1_section;
	section* piano2_section;
	section* piano3_section;
	section* piano4_section;

	section* bass_section;
	section* melody_section;
	section* hat_section;
	section* ghost_section;
	section* snare_section;
	section* kick_section;

	// Other variables

	unsigned int seed; // Seed for random number generation
	long tempo; // Tempo of the song
	float beat; // Beat length in milliseconds
	long runs; // Loop repetitions
	long measures;

	int play;

	ht_t* hash_note_names; // Hashtable for getting Midi note values

} t_musicbox;

// FUNCTION PROTOTYPES

void musicbox_bang(t_musicbox* x);
void musicbox_in1(t_musicbox* x, long n);
void musicbox_in2(t_musicbox* x, unsigned int n);
void *musicbox_new(t_symbol *s, long argc, t_atom *argv);
void musicbox_free(t_musicbox *x);
void musicbox_assist(t_musicbox *x, void *b, long m, long a, char *s);
void musicbox_task(t_musicbox* x);
void musicbox_measure_task(t_musicbox* x);
void musicbox_piano_task(t_musicbox* x);
void musicbox_bass_task(t_musicbox* x);
void musicbox_melody_task(t_musicbox* x);
void musicbox_hat_task(t_musicbox* x);
void musicbox_ghost_task(t_musicbox* x);
void musicbox_snare_task(t_musicbox* x);
void musicbox_kick_task(t_musicbox* x);
void musicbox_create_phrase(t_musicbox* x, phrase* current_phrase, char* filename, int rand_line);
void musicbox_create_section(t_musicbox* x, section* current_section, char** filename, int rand_line);
void musicbox_loadfile(t_musicbox* x, note* current_note, char* filename, int rand_line);
void musicbox_create_melody(t_musicbox* x, note* current_note, note* follow_beat);
void musicbox_create_melody_phrase(t_musicbox* x, phrase* current_phrase, phrase* follow_phrase);
void musicbox_create_melody_section(t_musicbox* x, section* current_section, phrase* follow_phrase);
void musicbox_create_bass(t_musicbox* x, note* current_note, note* follow_beat, int* scale);
void musicbox_create_bass_phrase(t_musicbox* x, phrase* current_phrase, phrase* follow_phrase, int** progression);
void musicbox_create_bass_section(t_musicbox* x, section* current_section, phrase* follow_phrase, int*** progressions);
void musicbox_create_piano(t_musicbox* x, note* current_note, int* scale, int piano_note);
void musicbox_create_piano_phrase(t_musicbox* x, phrase* current_phrase, int** progression, int piano_note);
void musicbox_create_piano_section(t_musicbox* x, section* current_section, int*** progressions, int piano_note);

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

	x->piano_outlet_length = floatout(x);
	x->piano_outlet_value_1 = intout(x);
	x->piano_outlet_value_2 = intout(x);
	x->piano_outlet_value_3 = intout(x);
	x->piano_outlet_value_4 = intout(x);

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
	x->measure_clock = clock_new((t_musicbox*)x, (method)musicbox_measure_task);
	x->piano_clock = clock_new((t_musicbox*)x, (method)musicbox_piano_task);
	x->bass_clock = clock_new((t_musicbox*)x, (method)musicbox_bass_task);
	x->melody_clock = clock_new((t_musicbox*)x, (method)musicbox_melody_task);
	x->hat_clock = clock_new((t_musicbox*)x, (method)musicbox_hat_task);
	x->ghost_clock = clock_new((t_musicbox*)x, (method)musicbox_ghost_task);
	x->snare_clock = clock_new((t_musicbox*)x, (method)musicbox_snare_task);
	x->kick_clock = clock_new((t_musicbox*)x, (method)musicbox_kick_task);

	// Phrases & Linked Lists

	x->piano1_section = (struct section*)malloc(sizeof(struct section));
	x->piano1_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->piano1_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->piano2_section = (struct section*)malloc(sizeof(struct section));
	x->piano2_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->piano2_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->piano3_section = (struct section*)malloc(sizeof(struct section));
	x->piano3_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->piano3_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->piano4_section = (struct section*)malloc(sizeof(struct section));
	x->piano4_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->piano4_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->bass_section = (struct section*)malloc(sizeof(struct section));
	x->bass_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->bass_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->melody_section = (struct section*)malloc(sizeof(struct section));
	x->melody_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->melody_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->hat_section = (struct section*)malloc(sizeof(struct section));
	x->hat_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->hat_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->ghost_section = (struct section*)malloc(sizeof(struct section));
	x->ghost_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->ghost_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->snare_section = (struct section*)malloc(sizeof(struct section));
	x->snare_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->snare_section->head->head = (struct note*)malloc(sizeof(struct note));

	x->kick_section = (struct section*)malloc(sizeof(struct section));
	x->kick_section->head = (struct phrase*)malloc(sizeof(struct phrase));
	x->kick_section->head->head = (struct note*)malloc(sizeof(struct note));

	// Other variables

	x->tempo = 0;
	x->runs = 0;
	x->seed = 0;
	x->measures = 0;
	x->play = 0;

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
			sprintf(s, "Seed 1");
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
	object_free(x->measure_clock);
	object_free(x->piano_clock);
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
	clock_unset(x->measure_clock);
	clock_unset(x->piano_clock);
	clock_unset(x->bass_clock);
	clock_unset(x->melody_clock);
	clock_unset(x->hat_clock);
	clock_unset(x->ghost_clock);
	clock_unset(x->snare_clock);
	clock_unset(x->kick_clock);

	if (x->play == 0) {

		x->play = 1;

		// Set random number seed

		srand(x->seed);

		// Load instrument

		char* f_hat = "D:/music_algorithm/patterns/hat.txt";
		char* f_hat2 = "D:/music_algorithm/patterns/hat2.txt";
		char* sections_hat[2] = { f_hat, f_hat2 };
		int rand_line_hat = get_random(1, number_of_lines(f_hat));
		musicbox_create_section(x, x->hat_section, sections_hat, rand_line_hat);

		char* f_ghost = "D:/music_algorithm/patterns/ghost.txt";
		char* sections_ghost[2] = {f_ghost, f_ghost};
		int rand_line_ghost = get_random(1, number_of_lines(f_ghost));
		musicbox_create_section(x, x->ghost_section, sections_ghost, rand_line_ghost);

		char* f_snare = "D:/music_algorithm/patterns/snare.txt";
		char* sections_snare[2] = {f_snare, f_snare};
		int rand_line_snare = get_random(1, number_of_lines(f_snare));
		musicbox_create_section(x, x->snare_section, sections_snare, rand_line_snare);

		char* f_kick = "D:/music_algorithm/patterns/kick.txt";
		char* sections_kick[2] = {f_kick, f_kick};
		int rand_line_kick = get_random(1, number_of_lines(f_kick));
		musicbox_create_section(x, x->kick_section, sections_kick, rand_line_kick);

		// Load chords

		static int** progressions[2];
		static int chord_1_v[4] = { 0,0,0,0 };
		static int chord_2_v[4] = { 0,0,0,0 };
		static int chord_3_v[4] = { 0,0,0,0 };
		static int chord_4_v[4] = { 0,0,0,0 };
		static int* progression_verse[4] = { chord_1_v, chord_2_v, chord_3_v, chord_4_v };
		static int chord_1_c[4] = { 0,0,0,0 };
		static int chord_2_c[4] = { 0,0,0,0 };
		static int chord_3_c[4] = { 0,0,0,0 };
		static int chord_4_c[4] = { 0,0,0,0 };
		static int* progression_chorus[4] = { chord_1_c, chord_2_c, chord_3_c, chord_4_c };
		memcpy(load_chords("D:/music_algorithm/patterns/chords.txt"), progression_verse, 32);
		memcpy(load_chords("D:/music_algorithm/patterns/chords2.txt"), progression_chorus, 32);
		progressions[1] = progression_verse;
		progressions[0] = progression_chorus;

		musicbox_create_piano_section(x, x->piano1_section, progressions, 1);
		musicbox_create_piano_section(x, x->piano2_section, progressions, 2);
		musicbox_create_piano_section(x, x->piano3_section, progressions, 3);
		musicbox_create_piano_section(x, x->piano4_section, progressions, 4);

		musicbox_create_bass_section(x, x->bass_section, x->kick_section->head, progressions);
		//musicbox_create_bass_phrase(x, x->bass_phrase, x->kick_phrase, progression);

		musicbox_create_melody_section(x, x->melody_section, x->snare_section->head);
		//musicbox_create_melody_phrase(x, x->melody_phrase, x->snare_phrase);

		// Play song

		x->runs = 4;
		clock_fdelay(x->m_clock, 0.);
	}
	else {
		x->play = 0;
	}
}

void musicbox_in1(t_musicbox* x, long n)
{
	x->tempo = n;
	x->beat = tempo_to_mil(x->tempo);
}

void musicbox_in2(t_musicbox* x, unsigned int n)
{
	x->seed = n;
}

// CLOCK TASKS

void musicbox_task(t_musicbox* x)
{
	x->measures = 4;

	x->piano1_section = next_section(x->piano1_section);
	x->piano1_phrase = x->piano1_section->head;
	x->piano2_section = next_section(x->piano2_section);
	x->piano2_phrase = x->piano2_section->head;
	x->piano3_section = next_section(x->piano3_section);
	x->piano3_phrase = x->piano3_section->head;
	x->piano4_section = next_section(x->piano4_section);
	x->piano4_phrase = x->piano4_section->head;

	x->bass_section = next_section(x->bass_section);
	x->bass_phrase = x->bass_section->head;

	x->melody_section = next_section(x->melody_section);
	x->melody_phrase = x->melody_section->head;

	x->hat_section = next_section(x->hat_section);
	x->hat_phrase = x->hat_section->head;

	x->ghost_section = next_section(x->ghost_section);
	x->ghost_phrase = x->ghost_section->head;

	x->snare_section = next_section(x->snare_section);
	x->snare_phrase = x->snare_section->head;

	x->kick_section = next_section(x->kick_section);
	x->kick_phrase = x->kick_section->head;

	if (x->runs > 0) {
		//post("Runs: %ld", x->runs);
		double measure = 4 * (double)x->beat;
		clock_fdelay(x->m_clock, 4*measure);
		clock_fdelay(x->measure_clock, .0);
		x->runs -= 1;
	}
	else {
		clock_unset(x->m_clock);
		clock_unset(x->measure_clock);
		clock_unset(x->piano_clock);
		clock_unset(x->bass_clock);
		clock_unset(x->melody_clock);
		clock_unset(x->hat_clock);
		clock_unset(x->ghost_clock);
		clock_unset(x->snare_clock);
		clock_unset(x->kick_clock);
	}
}

void musicbox_measure_task(t_musicbox* x)
{
	x->piano1_phrase = next_phrase(x->piano1_phrase, 0);
	x->piano1_current = x->piano1_phrase->head;
	x->piano2_phrase = next_phrase(x->piano2_phrase, 0);
	x->piano2_current = x->piano2_phrase->head;
	x->piano3_phrase = next_phrase(x->piano3_phrase, 0);
	x->piano3_current = x->piano3_phrase->head;
	x->piano4_phrase = next_phrase(x->piano4_phrase, 0);
	x->piano4_current = x->piano4_phrase->head;
	
	x->bass_phrase = next_phrase(x->bass_phrase, 0);
	x->bass_current = x->bass_phrase->head;

	x->melody_phrase = next_phrase(x->melody_phrase, 1);
	x->melody_current = x->melody_phrase->head;

	x->hat_phrase = next_phrase(x->hat_phrase, 2);
	x->hat_current = x->hat_phrase->head;

	x->ghost_phrase = next_phrase(x->ghost_phrase, 3);
	x->ghost_current = x->ghost_phrase->head;

	x->snare_phrase = next_phrase(x->snare_phrase, 4);
	x->snare_current = x->snare_phrase->head;

	x->kick_phrase = next_phrase(x->kick_phrase, 5);
	x->kick_current = x->kick_phrase->head;
	
	if (x->measures < 2) {
		x->piano1_phrase = reset_phrase(x->piano1_phrase, 0);
		x->piano2_phrase = reset_phrase(x->piano2_phrase, 0);
		x->piano3_phrase = reset_phrase(x->piano3_phrase, 0);
		x->piano4_phrase = reset_phrase(x->piano4_phrase, 0);

		x->bass_phrase = reset_phrase(x->bass_phrase, 0);
		x->melody_phrase = reset_phrase(x->melody_phrase, 1);
		x->hat_phrase = reset_phrase(x->hat_phrase, 2);
		x->ghost_phrase = reset_phrase(x->ghost_phrase, 3);
		x->snare_phrase = reset_phrase(x->snare_phrase, 4);
		x->kick_phrase = reset_phrase(x->kick_phrase, 5);
	}

	if (x->measures > 0) {
		//post("Measures: %ld", x->measures);
		clock_fdelay(x->measure_clock, 4 * (double)x->beat);

		clock_fdelay(x->piano_clock, .0);
		clock_fdelay(x->bass_clock, .0);
		clock_fdelay(x->melody_clock, .0);
		clock_fdelay(x->hat_clock, .0);
		clock_fdelay(x->ghost_clock, .0);
		clock_fdelay(x->snare_clock, .0);
		clock_fdelay(x->kick_clock, .0);
		x->measures -= 1;
	}
}

void musicbox_piano_task(t_musicbox* x) {
	note* current1 = x->piano1_current;
	note* current2 = x->piano2_current;
	note* current3 = x->piano3_current;
	note* current4 = x->piano4_current;
	outlet_int(x->piano_outlet_value_1, current1->value);
	outlet_int(x->piano_outlet_value_2, current2->value);
	outlet_int(x->piano_outlet_value_3, current3->value);
	outlet_int(x->piano_outlet_value_4, current4->value);
	outlet_float(x->piano_outlet_length, current1->length * (double)x->beat);
	if (current1->next->value != NULL) {
		clock_fdelay(x->piano_clock, current1->length * (double)x->beat);
		x->piano1_current = current1->next;
		x->piano2_current = current2->next;
		x->piano3_current = current3->next;
		x->piano4_current = current4->next;
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

void musicbox_create_section(t_musicbox* x, section* current_section, char** filename, int rand_line) {
	for (int i = 0; i < 2; i++) {
		musicbox_create_phrase(x, current_section->head, *(filename + i), rand_line);
		current_section->repetitions = 2;

		current_section->next = (section*)malloc(sizeof(section));
		current_section->next->head = (phrase*)malloc(sizeof(phrase));
		current_section->next->head->head = (note*)malloc(sizeof(note));
		current_section->next->repetitions = 0;
		current_section->next->next = NULL;
		current_section = current_section->next;
	}
}

void musicbox_create_phrase(t_musicbox* x, phrase* current_phrase, char* filename, int rand_line) {
	for (int i = 0; i < 1; i++) {
		musicbox_loadfile(x, current_phrase->head, filename, rand_line);
		current_phrase->repetitions = 4;

		current_phrase->next = (phrase*)malloc(sizeof(phrase));
		current_phrase->next->head = (note*)malloc(sizeof(note));
		current_phrase->next->repetitions = 0;
		current_phrase->next->next = NULL;
		current_phrase = current_phrase->next;
	}
}

void musicbox_loadfile(t_musicbox* x, note* current_note, char* filename, int rand_line) {
	FILE* fp;
	char str[MAXCHAR];

	int local_test = NULL;

	int line = 1;

	//int rand_line = get_random(1, number_of_lines(filename));

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
			rand_note = scale[rand_note_index]; //+ 12;
		}
		else { //Not on backbeat
			j = i;
			rand_note = scale[rand_note_index] - 12;
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
	for (int i = 0; i < 2; i++) {
		musicbox_create_melody(x, current_phrase->head, follow_phrase->head);

		if (i == 0) {
			current_phrase->repetitions = 3;
		}
		else {
			current_phrase->repetitions = 1;
		}

		current_phrase->next = (phrase*)malloc(sizeof(phrase));
		current_phrase->next->head = (note*)malloc(sizeof(note));
		current_phrase->next->repetitions = 0;
		current_phrase->next->next = NULL;
		current_phrase = current_phrase->next;
	}
}

void musicbox_create_melody_section(t_musicbox* x, section* current_section, phrase* follow_phrase) {
	for (int i = 0; i < 2; i++) {
		musicbox_create_melody_phrase(x, current_section->head, follow_phrase);
		current_section->repetitions = 2;

		current_section->next = (section*)malloc(sizeof(section));
		current_section->next->head = (phrase*)malloc(sizeof(phrase));
		current_section->next->head->head = (note*)malloc(sizeof(note));
		current_section->next->repetitions = 0;
		current_section->next->next = NULL;
		current_section = current_section->next;
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

void musicbox_create_bass_section(t_musicbox* x, section* current_section, phrase* follow_phrase, int*** progressions) {
	for (int i = 0; i < 2; i++) {
		musicbox_create_bass_phrase(x, current_section->head, follow_phrase, *(progressions + i));
		current_section->repetitions = 2;

		current_section->next = (section*)malloc(sizeof(section));
		current_section->next->head = (phrase*)malloc(sizeof(phrase));
		current_section->next->head->head = (note*)malloc(sizeof(note));
		current_section->next->repetitions = 0;
		current_section->next->next = NULL;
		current_section = current_section->next;
	}
}

void musicbox_create_piano(t_musicbox* x, note* current_note, int* scale, int piano_note) {
	int index = piano_note - 1;
	int note_value = *(scale + index);
	current_note->length = 4.0;
	current_note->value = note_value + 24;

	current_note->next = (note*)malloc(sizeof(note));
	current_note->next->value = NULL;
	current_note->next->length = 0;
	current_note->next->next = NULL;
	current_note = current_note->next;
}

void musicbox_create_piano_phrase(t_musicbox* x, phrase* current_phrase, int** progression, int piano_note) {
	for (int i = 0; i < 4; i++) {
		musicbox_create_piano(x, current_phrase->head, *(progression + i), piano_note);
		current_phrase->repetitions = 1;

		current_phrase->next = (phrase*)malloc(sizeof(phrase));
		current_phrase->next->head = (note*)malloc(sizeof(note));
		current_phrase->next->repetitions = 0;
		current_phrase->next->next = NULL;
		current_phrase = current_phrase->next;
	}
}

void musicbox_create_piano_section(t_musicbox* x, section* current_section, int*** progressions, int piano_note) {
	for (int i = 0; i < 2; i++) {
		musicbox_create_piano_phrase(x, current_section->head, *(progressions + i), piano_note);
		current_section->repetitions = 2;

		current_section->next = (section*)malloc(sizeof(section));
		current_section->next->head = (phrase*)malloc(sizeof(phrase));
		current_section->next->head->head = (note*)malloc(sizeof(note));
		current_section->next->repetitions = 0;
		current_section->next->next = NULL;
		current_section = current_section->next;
	}
}