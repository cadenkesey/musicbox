/**
	@file
	hash - a simple hashtable implementation - credit to Engineer Man
	Caden Kesey
*/

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TABLE_SIZE 100 // Hash table size

// Structs

typedef struct entry_t {
	char* key;
	int value;
	struct entry_t* next;
} entry_t;

typedef struct {
	entry_t** entries;
} ht_t;

// Hash Functions

ht_t* ht_create(void) {
	// allocate table
	ht_t* hashtable = malloc(sizeof(ht_t) * 1);

	// allocate table entries
	hashtable->entries = malloc(sizeof(entry_t*) * TABLE_SIZE);

	// set each to null (needed for proper operation)
	int i = 0;
	for (; i < TABLE_SIZE; ++i) {
		hashtable->entries[i] = NULL;
	}

	return hashtable;
}

entry_t* ht_pair(const char* key, const int value) {
	// allocate the entry
	entry_t* entry = malloc(sizeof(entry) * 1);
	entry->key = malloc(strlen(key) + 1);
	entry->value = malloc(sizeof(int));

	// copy the key and value in place
	strcpy(entry->key, key);
	entry->value = value;

	// next starts out null but may be set later on
	entry->next = NULL;

	return entry;
}

void ht_set(ht_t* hashtable, const char* key, const int value) {
	unsigned int slot = hash(key);

	// try to look up an entry set
	entry_t* entry = hashtable->entries[slot];

	// no entry means slot empty, insert immediately
	if (entry == NULL) {
		hashtable->entries[slot] = ht_pair(key, value);
		return;
	}

	entry_t* prev;

	// walk through each entry until either the end is reached or a matching key is found
	while (entry != NULL) {
		// check key
		if (strcmp(entry->key, key) == 0) {
			// match found, replace value
			free(entry->value);
			entry->value = malloc(sizeof(int));
			entry->value = value;
			return;
		}

		// walk to next
		prev = entry;
		entry = prev->next;
	}

	// end of chain reached without a match, add new
	prev->next = ht_pair(key, value);
}

int ht_get(ht_t* hashtable, const char* key) {
	unsigned int slot = hash(key);

	// try to find a valid slot
	entry_t* entry = hashtable->entries[slot];

	// no slot means no entry
	if (entry == NULL) {
		return NULL;
	}

	// walk through each entry in the slot, which could just be a single thing
	while (entry != NULL) {
		// return value if found
		if (strcmp(entry->key, key) == 0) {
			return entry->value;
		}

		// proceed to next key if available
		entry = entry->next;
	}

	// reaching here means there were >= 1 entries but no key match
	return NULL;
}

void ht_dump(ht_t* hashtable) {
	for (int i = 0; i < TABLE_SIZE; ++i) {
		entry_t* entry = hashtable->entries[i];

		if (entry == NULL) {
			continue;
		}

		post("slot[%4d]: ", i);

		for (;;) {
			post("%s=%d ", entry->key, entry->value);

			if (entry->next == NULL) {
				break;
			}

			entry = entry->next;
		}

		post("\n");
	}
}

unsigned int hash(const char* key) {
	unsigned long int value = 0;
	unsigned int i = 0;
	unsigned int key_len = strlen(key);

	// do several rounds of multiplication
	for (; i < key_len; ++i) {
		value = value * 37 + key[i];
	}

	// make sure value is 0 <= value < table size
	value = value % TABLE_SIZE;

	return value;
}