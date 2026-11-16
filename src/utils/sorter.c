#include "src/includes/types.h"


/*Sorts Descrently by probability*/
void DecrescentSortByP(char *symbols, float *probability, int number_s){
    for(int i = 0; i < number_s - 1; i++){
        int max_idx = i;
        for(int j = i + 1; j < number_s; j++){
            if(probability[j] > probability[max_idx]){
                max_idx = j;
            }
        }
        if(max_idx != i){
            // Swap probabilities
            float f_temp = probability[i];
            probability[i] = probability[max_idx];
            probability[max_idx] = f_temp;

            // Swap symbols
            char c_temp = symbols[i];
            symbols[i] = symbols[max_idx];
            symbols[max_idx] = c_temp;
        }
    }
}


/*Sorts Descrently by probability, still need to change this*/
void CrescentSortByP(char* symbols , float* probability, int number_s){
    for(int i = 0; i < number_s - 1; i++){
        int min_idx = i;
        for(int j = i + 1; j < number_s; j++){
            if(probability[j] < probability[min_idx]){
                min_idx = j;
            }
        }
        if(min_idx != i){
            // Swap probabilities
            float f_temp = probability[i];
            probability[i] = probability[min_idx];
            probability[min_idx] = f_temp;

            // Swap symbols
            char c_temp = symbols[i];
            symbols[i] = symbols[min_idx];
            symbols[min_idx] = c_temp;
        }
    }
}


/* Sorts two arrays by the seconds array code length stored in a struct*/
void sortByCodeLength(char* symbs , NewSymbols* code , int N){
    for(int i = 1; i < N; i++){
        NewSymbols c_key = code[i];
        char s_key = symbs[i];
        int j = i - 1;

        // só desloca quando o comprimento é ESTRITAMENTE maior (> e não >=)
        while(j >= 0 && code[j].length > c_key.length){
            code[j+1]  = code[j];
            symbs[j+1] = symbs[j];
            j--;
        }
        code[j+1]  = c_key;
        symbs[j+1] = s_key;
    }
}