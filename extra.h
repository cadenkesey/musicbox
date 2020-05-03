/**
	@file
	extra - holds auxillary functions
	Caden Kesey
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int phrase_rep_hold[6] = {0,0,0,0,0,0};

typedef struct note {
	int value;
	float length;
	struct note* next;
} note;

typedef struct phrase {
	struct note* head;
	int repetitions;
	struct phrase* next;
} phrase;

typedef struct section {
	struct phrase* head;
	int repetitions;
	struct section* next;
} section;

void printList(struct note* n) {
	int i = 0;
	while (n != NULL) {
		post("NOTE %d: %d %f", i, n->value, n->length);
		n = n->next;
		i++;
	}
}

float* get_beats(struct note* n) {
	static float r[20]; // Hold
	int i = 0;
	float sum = 0;
	while (n != NULL) {
		if (n->value > -1) {
			r[i] = sum;
			//post("r[%d] = %f", i, n->value, n->length);
			i++;
		}
		sum = sum + n->length;
		n = n->next;
	}
	r[i] = -1;
	return r;
}

float tempo_to_mil(int tempo)
{
	float beat_length = 60 / ((float)tempo / 1000);
	return beat_length;
}

/*
https://www.sanfoundry.com/c-program-number-lines-text-file/
*/
int number_of_lines(char* filename)
{
	FILE* fileptr;
	int count_lines = 1;
	char chr;

	fileptr = fopen(filename, "r");

	//extract character from file and store in chr
	chr = getc(fileptr);
	while (chr != EOF)
	{
		//Count whenever new line is encountered
		if (chr == '\n')
		{
			count_lines = count_lines + 1;
		}
		//take next character from file.
		chr = getc(fileptr);
	}
	fclose(fileptr); //close file.
	return count_lines;
}

int** load_chords(char* filename) {
	static int chord_1[4] = { 0,0,0,0 };
	static int chord_2[4] = { 0,0,0,0 };
	static int chord_3[4] = { 0,0,0,0 };
	static int chord_4[4] = { 0,0,0,0 };
	static int* progression[4] = {chord_1, chord_2, chord_3, chord_4};

	FILE* fp;
	char str[MAXCHAR];
	int line = 1;
	int rand_line = get_random(1, number_of_lines(filename));

	fp = fopen(filename, "r");
	if (fp == NULL) {
		post("Could not open file %s", filename);
	}
	else {
		while (fgets(str, MAXCHAR, fp) != NULL) {
			if (line == rand_line) {
				char* token_A;
				char* rest_A = str;
				int i = 0;
				while ((token_A = strtok_s(rest_A, ",", &rest_A))) {
					if (token_A[strlen(token_A) - 1] == '\n') {
						token_A[strlen(token_A) - 1] = 0;
					}

					char* token_copy = "";
					strcpy(token_copy, token_A);
					char* rest_B = token_copy;
					const char delim_B[2] = " ";
					char* token_B;
					int j = 0;
					while ((token_B = strtok_s(rest_B, " ", &rest_B))) {
						progression[i][j] = (int)strtol(token_B, (char**)NULL, 10);
						//post("IN THE OTHER [%d] [%d]: %d", i, j, progression[i][j]);
						j++;
					}
					i++;
				}
			}
			line++;
		}
		fclose(fp);
	}
	return progression;
}

int get_random(int lower, int upper)
{
	int num = (rand() % (upper - lower + 1)) + lower;
	return num;
}

phrase* next_phrase(phrase* current_phrase, int inst) {
	if (current_phrase->repetitions < 1) {
		if (current_phrase->next != NULL) {
			//post("SETTING REPETITIONS OF %d TO %d", inst, phrase_rep_hold[inst]);
			current_phrase->repetitions = phrase_rep_hold[inst];
			current_phrase = current_phrase->next;
			phrase_rep_hold[inst] = 0;
		}
	}

	if (current_phrase->repetitions > phrase_rep_hold[inst]) {
		phrase_rep_hold[inst] = current_phrase->repetitions;
	}

	current_phrase->repetitions = current_phrase->repetitions - 1;

	return current_phrase;
}

phrase* reset_phrase(phrase* current_phrase, int inst) {
	//post("RESETTING REPETITIONS OF %d TO %d", inst, phrase_rep_hold[inst]);
	current_phrase->repetitions = phrase_rep_hold[inst];
	return current_phrase;
}

section* next_section(section* current_section) {
	if (current_section->repetitions < 1) {
		if (current_section->next != NULL) {
			current_section = current_section->next;
		}
	}

	current_section->repetitions = current_section->repetitions - 1;

	return current_section;
}