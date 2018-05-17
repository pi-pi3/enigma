#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ALSIZE 26
#define BUFFER_SIZE 128

// the two important characteristics a single rotor has
typedef struct {
    char *rotor;
    char rotate;
} rotor;

// first five rotors used in enigma
const rotor rotors[] = {
    {"EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'R' - 'A'},
    {"AJDKSIRUXBLHWTMCQGZNPYFVOE", 'F' - 'A'},
    {"BDFHJLCPRTXVZNYEIWGAKMUSQO", 'W' - 'A'},
    {"ESOVPZJAYQUIRHXLNFTGKDCMWB", 'K' - 'A'},
    {"VZBRGITYUPSDNHLXAWMJQOFECK", 'A' - 'A'},
};

// reflectors, also rotors but don't rotate
const char *reflectors[] = {
    "YRUHQSLDPXNGOKMIEBFZCWVJAT",
    "FVPJIAOYEDRZXWGCTKUQSBNMHL",
    "ENKQAUYWJICOPBLMDXZVFTHRGS",
    "RDOBJNTKVEHMLFCWZAXGYIPSUQ",
};

// holds the configuration of the enigma
typedef struct {
    int rotor_count;
    int **rotors;
    int *rotate;
    int *reflector;
    int *plugboard;
} enigma;

// converts a string into an integer lookup table
void table_from_string(int *dst, const char *src) {
    for (; *src; dst++, src++) {
        char ch = *src;

        // lower to upper case
        if (ch >= 'a' && ch <= 'z') {
            *dst = ch - 'a';
        } else if (ch >= 'A' && ch <= 'Z') {
            *dst = ch - 'A';
        } else {
            // all invalid characters are replaced by 'X'
            *dst = 'X' - 'A';
        }
    }
}

// converts an integer lookup table into a string
void string_from_table(char *dst, const int *src, int size) {
    for (; size; dst++, src++, size--) {
        // to upper case
        *dst = *src + 'A';
    }

    // nul-terminator at the end
    *dst = 0;
}

// sets the rotors of the configuration to any number of rotors indexed by the
// range [1; 5]
void rotor_set(enigma *e, int *rotor_idxs, int rotor_count) {
    e->rotor_count = rotor_count;

    for (int i = 0; i < rotor_count; i++) {
        // e->rotors[0] is expected to be a pointer to 4 * rotor_count * ALSIZE
        // bytes
        e->rotors[i] = *e->rotors + i * ALSIZE;

        int idx = rotor_idxs[i] - 1;
        table_from_string(e->rotors[i], rotors[idx].rotor);
        e->rotate[i] = rotors[idx].rotate;
    }
}

// rotates a cog
void rotate(int *rotor) {
    int tmp = *rotor;

    for (int i = 0; i < ALSIZE - 1; i++) {
        rotor[i] = rotor[i + 1];
    }

    rotor[ALSIZE - 1] = tmp;
}

// rotates all cogs
void rotate_cogs(enigma *e) {
    // rotate right-most (0th) cog
    rotate(e->rotors[0]);
    for (int j = 0; j < e->rotor_count - 1; j++) {
        // rotate all others if the previous cog reached its rotate position
        if (e->rotors[j][0] == e->rotate[j]) {
            rotate(e->rotors[j + 1]);
        }
    }
}

// looks up an integer in the lookup table, possibly inversed
int lookup(int *table, int val, int inverse) {
    if (inverse) {
        for (int i = 0; i < ALSIZE; i++) {
            if (table[i] == val) {
                return i;
            }
        }
        abort(); // unreachable, unless something is terribly messed up
    } else {
        return table[val];
    }
}

// maps a single letter of the alphabet [0; 25] to another letter within the
// alphabet
int map_letter(enigma *e, int l) {
    // input -> plugboard
    int result = lookup(e->plugboard, l, 0);
    
    // plugboard -> rotor[0] -> ... -> rotor[n]
    for (int i = 0; i < e->rotor_count; i++) {
        result = lookup(e->rotors[i], result, 0);
    }

    // rotor[n] -> reflector
    result = lookup(e->reflector, result, 0);
    
    // reflector -> rotor[n]^-1 -> ... -> rotor[0]^-1
    for (int i = e->rotor_count - 1; i >= 0; i--) {
        result = lookup(e->rotors[i], result, 1);
    }

    // rotor[0]^-1 -> plugboard^-1
    // the plugboard doesn't have to be inversed, it's symmetric
    return lookup(e->plugboard, result, 0);
}

// encrypts or decrypts a message encoded as integers in the range [0; 25]
void crypt(enigma *e, int *dst, const int *src, int size) {
    for (; size; dst++, src++, size--) {
        rotate_cogs(e);
        *dst = map_letter(e, *src);
    }
}

int main(int argc, char **argv) {
    int rotor_count = 3;
    int rotors[] = {2, 1, 3};

    enigma e;

    e.rotors = malloc(sizeof(int *) * rotor_count);
    *e.rotors = malloc(sizeof(int) * ALSIZE * rotor_count);
    e.rotate = malloc(sizeof(int) * rotor_count);
    e.reflector = malloc(sizeof(int) * ALSIZE);
    e.plugboard = malloc(sizeof(int) * ALSIZE);

    rotor_set(&e, rotors, rotor_count);

    table_from_string(e.reflector, reflectors[0]);
    //table_from_string(e.plugboard, "QDCBJXGHVEWUTNYSARPMLIKFOZ");
    table_from_string(e.plugboard, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"); // identity mapping

    int *en_in = malloc(BUFFER_SIZE * sizeof(int)); // encoded enigma-ready input
    int *en_out = malloc(BUFFER_SIZE * sizeof(int)); // encoded enigma-generated ouptup
    char *input = malloc(BUFFER_SIZE + 1); // user-readable input
    char *output = malloc(BUFFER_SIZE + 1); // user-readable output
    int total = 0; // total bytes read

    while (!feof(stdin)) {
        // read N bytes
        int size = fread(input, 1, BUFFER_SIZE - 1, stdin);
        // skip the LF at the end
        if (input[size - 1] == '\n') {
            --size;
        }
        // nul-terminator
        input[size] = 0;
        total += size;

        // encode as ints [0; 25]
        table_from_string(en_in, input);

        // encrypt/decrypt
        crypt(&e, en_out, en_in, size);

        // decode
        string_from_table(output, en_out, size);

        // write & repeat
        fwrite(output, size, 1, stdout);
        fflush(stdout);
    }

    if (total) {
        fputc('\n', stdout);
    }

    free(output);
    free(input);
    free(en_out);
    free(en_in);

    free(e.rotors[0]);
    free(e.plugboard);
    free(e.reflector);
    free(e.rotate);
    free(e.rotors);

    return 0;
}
