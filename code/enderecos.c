// enderecos.c
// ficheiro dedicado às funções relativas a validação de endereços (porto e ip)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "enderecos.h"

// Determina se um dado porto é valido e retorna 0 ou 1 conforme
int validate_port(char *port){
    int num;
    num = atoi(port); //convert substring to number

    // O porto deve ser um inteiro e estar entre 1 e 65535 para ser válido
    if (num > 0 && num <= 65535) {
        return 1;
    } 
    else
        return 0;
}

// Determina se um caractér corresponde a um número e retorna 0 ou 1 conforme
// https://www.tutorialspoint.com/c-program-to-validate-an-ip-address
int validate_number(char *str) {
    while (*str) {
        if(!isdigit(*str)){ //if the character is not a number, return false
            return 0;
        }
        str++; //point to next character
    }
    return 1;
}

// Determina se um dado IP é válido e retorna 0 ou 1 conforme
// https://www.tutorialspoint.com/c-program-to-validate-an-ip-address
int validate_ip(char *ip) { //check whether the IP is valid or not
    int num, dots = 0;
    char *ptr;
    if (ip == NULL)
        return 0;
    ptr = strtok(ip, "."); //cut the string using dor delimiter
    if (ptr == NULL)
        return 0;
    while (ptr) {
        if (!validate_number(ptr)) //check whether the sub string is holding only number or not
            return 0;
        num = atoi(ptr); //convert substring to number
        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, "."); //cut the next part of the string
            if (ptr != NULL)
                dots++; //increase the dot count
        } 
        else
            return 0;
    }
    if (dots != 3) //if the number of dots are not 3, return false
        return 0;
    return 1;
}
