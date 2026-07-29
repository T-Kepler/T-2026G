#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int split_string(const char* str, const char* delim, char*** tokens);
void process_tokens(char** tokens, int len);
unsigned int string_to_uint(const char* str);

int main()
{
    char str[] = "a11,b12,c13,d14";
    char** tokens = NULL;
    int count;
    int flag_succful = 0;
    count = split_string(str, ",", &tokens);
    flag_succful = process_tokens(tokens, count);
    if (flag_succful == 0)
        return 0;
    for (int i = 0; i < count; i++) {
        printf("Token %d: %s\n", i + 1, tokens[i]);
    }

    for (int i = 0; i < count; i++) {
        free(tokens[i]);
    }
    free(tokens);

    return 0;
}

int split_string(const char* str, const char* delim, char*** tokens)
{
    char* strCopy = strdup(str);
    if (!strCopy) {
        perror("strdup failed");
        return 0;
    }
    char* token;
    char* last = NULL;
    int tokenCount = 0;

    token = strtok_r(strCopy, delim, &last);
    while (token != NULL) {
        tokenCount++;
        *tokens = realloc(*tokens, sizeof(char*) * tokenCount);
        if (*tokens == NULL) {
            break;
        }
        (*tokens)[tokenCount - 1] = strdup(token);
        token = strtok_r(NULL, delim, &last);
    }

    free(strCopy);
    return tokenCount;
}

void process_tokens(char** tokens, int len)
{
    for (int i = 0; i < len; i++) {
        char firstChar = tokens[i][0];
        char tempArray[256];
        char* tempStr = tokens[i] + 1;
        strcpy(tempArray, tempStr);
        unsigned int temp_uint = string_to_uint(tempArray);
        if (temp_uint == 0)
            return;
        switch (firstChar) {
        case 'a':
            printf("it is temp a %u\n", temp_uint);
            break;
        case 'b':
            printf("it is temp b %u\n", temp_uint);
            break;
        case 'c':
            printf("it is temp c %u\n", temp_uint);
            break;
        case 'd':
            printf("it is temp d %u\n", temp_uint);
            break;
        default:
            printf("Token %d starts with an unrecognized character: %s\n", i + 1, tokens[i]);
            break;
        }
    }
}

unsigned int string_to_uint(const char* str)
{
    if (str == NULL) {
        return 0;
    }

    char* endPtr;
    unsigned int number = strtoul(str, &endPtr, 10);

    if (*endPtr != '\0') {
        return 0;
    }

    return number;
}