#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define MAX 10001
#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

typedef enum { END = 0, ID, CT_INT, CT_REAL, CT_CHAR, CT_STRING,
	       ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ, LESS, LESSEQ, GREATER, GREATEREQ,
               COMMA, SEMICOLON, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC, 
       	       BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT, RETURN, STRUCT, VOID, WHILE
	      } ids;

int getNextToken();
int line = 1;
char *pCrtCh;

void err(const char *fmt, ...)
	{
		va_list va;
		va_start(va,fmt);
		fprintf(stderr,"error: ");
		vfprintf(stderr,fmt,va);
		fputc('\n',stderr);
		va_end(va);
		exit(-1);
	}

typedef struct _Token{
	int code;                      
	union {
		char *text;                 // used for ID, CT_STRING (dynamic allocation)
		long int i;                 // used for CT_INT, CT_CHAR
		double r;                   // used for CT_REAL
		};
	int line;                           // line from entry file
	struct _Token *next;               // next element in chain
	} Token;

Token *lastToken = NULL, *firstToken = NULL;

Token *addTk(int code)
{
	Token *tk;
	SAFEALLOC(tk,Token)
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if(lastToken)
	{
	lastToken->next = tk;
	}else
	{
        firstToken = tk;
	}
	lastToken = tk;
	return tk;
}

char *getTokenCode(int code)
{
	switch(code)
	{
		case END : return "END"; break;
		case ID : return "ID"; break;
		case CT_INT : return "CT_INT"; break;
		case CT_REAL : return "CT_REAL"; break;
		case CT_STRING : return "CT_STRING"; break;
		case CT_CHAR : return "CT_CHAR"; break;
		case ADD : return "ADD"; break;
		case SUB : return "SUB"; break;
		case DIV : return "DIV"; break;
		case MUL : return "MUL"; break;
		case DOT : return "DOT"; break;
		case AND : return "AND"; break;
		case OR : return "OR"; break;
		case NOT : return "NOT"; break;
		case ASSIGN : return "ASSIGN"; break;
		case EQUAL : return "EQUAL"; break;
		case NOTEQ : return "NOTEQ"; break;
		case LESS : return "LESS"; break;
		case LESSEQ : return "LESSEQ"; break;
		case GREATER : return "GREATER"; break;
		case GREATEREQ : return "GREATEREQ"; break;
		case COMMA : return "COMMA"; break;
		case SEMICOLON : return "SEMICOLON"; break;
		case LPAR : return "LPAR"; break;
		case RPAR : return "RPAR"; break;
		case LBRACKET : return "LBRACKET"; break;
		case RBRACKET : return "RBRACKET"; break;
		case LACC : return "LACC"; break;
		case RACC : return "RACC"; break;
		case BREAK : return "BREAK"; break;
		case CHAR : return "CHAR"; break;
		case DOUBLE : return "DOUBLE"; break;
		case ELSE : return "ELSE"; break;
		case FOR : return "FOR"; break;
		case IF : return "IF"; break;
		case INT : return "INT"; break;
		case RETURN : return "RETURN"; break;
		case STRUCT : return "STRUCT"; break;
		case VOID : return "VOID"; break;
		case WHILE : return "WHILE"; break;

		default : return "Invalid code value!"; break;
		
	}
}

void displayTokens()
{
	Token *currentToken;
	for(currentToken = firstToken; currentToken != NULL; currentToken = currentToken->next)
	{
		printf(" %d %s ", currentToken->line, getTokenCode((currentToken->code)));
		switch(currentToken->code)
		{
			case ID: printf(" :  %s", currentToken->text);
			         break;
			case CT_INT: printf(" :  %ld", currentToken->i);
				     break;
			case CT_CHAR: printf(" :  %c", currentToken->i);
				      break;
			case CT_REAL: printf(" :  %g", currentToken->r);
				      break;
			case CT_STRING:	printf(" :  %s", currentToken->text);
					break;

			default : break;
		}
		printf("\n");
	}
}

void tkerr(const Token *tk,const char *fmt, ...)
{
	va_list va;
	va_start(va,fmt);
	fprintf(stderr,"error in line %d: ",tk->line);
	vfprintf(stderr,fmt,va);
	fputc('\n',stderr);
	va_end(va);
	exit(-1);
}

char *createString(char *startCh, char *endCh)
{
    char *result = (char*)malloc((endCh-startCh)*sizeof(char));
    int index = 0;
    char aux;

    while((endCh-startCh) > 0)
    {
        aux = startCh[1];
		if((*startCh) == '\\')
		{
			if(aux == 'a') result[index] = '\a';
			if(aux == 'b') result[index] = '\b';
			if(aux == 'f') result[index] = '\f';
			if(aux == 'n') result[index] = '\n';
			if(aux == 'r') result[index] = '\r';
			if(aux == 't') result[index] = '\t';
			if(aux == 'v') result[index] = '\v';
			if(aux == 'r') result[index] = '\r';
			if(aux == '\'') result[index] = '\'';
			if(aux == '?') result[index] = '\?';
			if(aux == '"') result[index] = '\"';
			if(aux == '\\') result[index] = '\\';
			
			if(!isalpha(aux)) result[index] = aux;
			
			startCh++;
		}
		else result[index] = *startCh;
        index++;
        startCh++;
    }
    result[index] = '\0';

    return result;
}

int getNextToken()
{
	char ch, *ptrStart, prevChar;
	int state = 0;
	int lenChar;
	int n;
	int isHexFlag = 0;
	int isOctFlag = 0;

	Token *tk;

	for(;;)
	{
		ch = *pCrtCh;

		switch(state)
		{
			case 0: if(isalpha(ch) || ch == '_')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 1;
					}
					else if(ch == '\'')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 13;
					}
					else if(ch == '\"')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 16;
					}
					else if(ch == '[')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 28;
					}
					else if(ch == ']')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 29;
					}
					else if(ch >= '1' && ch <= '9')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 2;
					}
					else if(ch == '0')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 5;
						isOctFlag = 1;
					}
					else if(ch == '!')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 10;
					}
					else if(ch == '/')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 18;
					}
					else if(ch == ' ' || ch == '\r' || ch == '\t') pCrtCh++;
					else if(ch == '\n')
					{
						pCrtCh++;
						line++;
					}
					else if(ch == '}')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 41;
					}
					else if(ch == '{')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 42;
					}
					else if(ch == '<')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 22;
					}
					else if(ch == '>')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 23;
					}
					else if(ch == '=')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 24;
					}
					else if(ch == '+')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 45;
					}
					else if(ch == '-')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 46;
					}
					else if(ch == '*')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 47;
					}
					else if(ch == ',')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 35;
					}
					else if(ch == ';')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 36;
					}
					else if(ch == '.')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 48;
					}
					else if(ch == '&')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 25;
					}
					else if(ch == '|')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 26;
					}
					else if(ch == '(')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 51;
					}
					else if(ch == ')')
					{
						ptrStart = pCrtCh;
						pCrtCh++;
						state = 52;
					}
					else if(ch == '\0') return END;
					else {
						  perror("\nINVALID CHARACTER!\n");
						  exit(-1);
						 }
					break;

			case 1: if(ch == '_' || isalnum(ch)) pCrtCh++;
					else state = 27;
					break;

			case 2: if(ch >= '0' && ch <= '9') pCrtCh++;
					else if(ch == '.')
					{
						pCrtCh++;
						state = 3;
					}
					else if(ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 6;
					}
					else state = 30;
					break;

			case 3: if(isdigit(ch))
					{
						pCrtCh++;
						state = 4;
					}
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;

			case 4: if(isdigit(ch)) pCrtCh++;
					else if(ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 6;
					}
					else state = 33;
					break;

			case 5: if(ch >= '0' && ch <= '7')
					{
						pCrtCh++;
						state = 7;
					}
					else if(ch == '.')
					{
                        pCrtCh++;
						state = 3;
					}
					else if(ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 6;
					}
					else if(ch == 'x')
					{
						isHexFlag = 1;
						pCrtCh++;
						state = 11;
					}
					else if(ch == '8' || ch == '9')
					{
						pCrtCh++;
						state = 55;
					}
					else state = 30;
					break;

			case 6: if(isdigit(ch))
					{
						pCrtCh++;
						state = 8;
					}
					else if(ch == '-' || ch == '+')
					{
						pCrtCh++;
						state = 9;
					}
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;

			case 7: if(ch >= '0' && ch <= '7') pCrtCh++;
					else if(ch == '8' || ch == '9')
					{
						pCrtCh++;
						state = 55;
					}
					else
					{
						pCrtCh++;
						state = 30;
					}
					break;

			case 8: if(isdigit(ch)) pCrtCh++;
					else state = 33;
					break;

			case 9: if(isdigit(ch))
					{
						pCrtCh++;
						state = 8;
					}
					else
					{
						perror("\nINVALID CHARACTER AFTER +/- !\n");
						exit(-1);
					}
					break;

			case 10: if(ch == '=')
					{
						pCrtCh++;
						state = 32;
					}
					else
					{
						pCrtCh++;
						state = 31;
					}
					break;

			case 11: if((isdigit(ch)) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))
					{
						pCrtCh++;
						state = 12;
					}
					else
					{
						perror("\nINVALID CHARACTER IN BASE 16! CH BETWEEN 0-F!\n");
						exit(-1);
					}
					break;

			case 12: if((isdigit(ch)) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) pCrtCh++;
					else state = 30;
					break;

			case 13: if(ch == '\\')
					{
						pCrtCh++;
						state = 14;
					}
					else if(ch == '\'')
					{
					 	perror("\nYOU CAN'T HAVE AN EMPTY CHARACTER\n");
						exit(-1);
					}
					else
					{
						prevChar = pCrtCh[0];
						 pCrtCh++;
						state = 15;
					}
					break;

			case 14: switch(ch)
					 {
					case 'a': prevChar = '\a'; break;
					case 'b': prevChar = '\b'; break;
					case 'f': prevChar = '\f'; break;
					case 'n': prevChar = '\n'; break;
					case 'r': prevChar = '\r'; break;
					case 't': prevChar = '\t'; break;
					case 'v': prevChar = '\v'; break;
					case '\\': prevChar = '\\'; break;
					case '"': prevChar = '\"'; break;
					case '\'': prevChar = '\''; break;
					case '?': prevChar = '\?'; break;
					case '0': prevChar = '\0'; break;

					default:prevChar = ch;
					}

					pCrtCh++;
					state = 15;
					break;

			case 15: if(ch == '\'')
					{
						pCrtCh++;
						state = 34;
					}
					else
					{
						perror("\nToo many characters!\n");
						exit(-1);
					}
					break;

			case 16: if(ch == '\\')
					{
						pCrtCh++;
						state = 17;
					}
					else if(ch == '\"') state = 37;
					else pCrtCh++;
					break;

			case 17: if(ch == 'a' || ch == 'n' || ch == 'f' || ch == 'r' || ch == 't' ||
					    ch == 'v' || ch == '\'' || ch == '\"' || ch == '?' || ch == '\\' || ch == '0')
					{
						pCrtCh++;
						state = 16;
					}
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;

			case 18: if(ch == '*')
					{   line++;
						pCrtCh++;
						state = 20;
					}
					else if(ch == '/')
					{
						line++;
						pCrtCh++;
						state = 19;
					}
					else state = 38;
					break;

			case 19: if(ch != '\n' && ch != '\r' && ch != '\0') pCrtCh++;
					else
					{
						pCrtCh++;
						state = 0;
					}
					break;

			case 20: if(ch != '*') pCrtCh++;
					else if(ch == '*')
					{
						pCrtCh++;
						state = 21;
					}
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;

			case 21: if(ch == '*') pCrtCh++;
					else if(ch != '/')
					{
						pCrtCh++;
						state = 20;
					}
					else if(ch == '/')
					{
						pCrtCh++;
						state = 0;
						line++;
					}
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;
	
			case 22: if(ch == '=')
					{
						pCrtCh++;
						state = 40;
					}
					else state = 39;
					break;

			case 23: if(ch == '=')
					{
						pCrtCh++;
						state = 43;
					}
					else state = 54;
					break;

			case 24: if(ch == '=')
					{
						pCrtCh++;
						state = 44;
					}
					else state = 53;
					break;

			case 25: if(ch == '&')
					{
						pCrtCh++;
						state = 49;
					}
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;

			case 26: if(ch == '|')
					{
						pCrtCh++;
						state = 50;
					}
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;

			case 27: n = pCrtCh- ptrStart;
					if(n == 5 && (!memcmp(ptrStart,"break",5))) tk = addTk(BREAK);
					else if (n == 5 && (!memcmp(ptrStart,"while",5))) tk = addTk(WHILE);
					else if (n == 4 && (!memcmp(ptrStart,"char",4))) tk = addTk(CHAR);
					else if (n == 6 && (!memcmp(ptrStart,"double",n))) tk = addTk(DOUBLE);
					else if (n == 4 && (!memcmp(ptrStart,"else",n))) tk = addTk(ELSE);
					else if (n == 3 && (!memcmp(ptrStart,"for",n))) tk = addTk(FOR);
					else if (n == 2 && (!memcmp(ptrStart,"if",n))) tk = addTk(IF);
					else if (n == 3 && (!memcmp(ptrStart,"int",n))) tk = addTk(INT);
					else if (n == 6 && (!memcmp(ptrStart,"return",n))) tk = addTk(RETURN);
					else if (n == 6 && (!memcmp(ptrStart,"struct",n))) tk = addTk(STRUCT);
					else if (n == 4 && (!memcmp(ptrStart,"void",n))) tk = addTk(VOID);
					else if (n == 5 && (!memcmp(ptrStart,"while",n))) tk = addTk(WHILE);
					else
					 {
						tk = addTk(ID);
						tk->text = (char*)malloc(pCrtCh - ptrStart + 1);
						strcpy(tk->text,createString(ptrStart, pCrtCh));
					 }
					return ID;
					break;

			case 28: tk = addTk(LBRACKET);
					return LBRACKET;
					break;
			case 29: addTk(RBRACKET);
					return RBRACKET;
					break;
			case 30: tk = addTk(CT_INT);
					if(isHexFlag == 1) tk->i=(int)strtol(createString(ptrStart+2, pCrtCh) , NULL, 16);	//converts to a long int according to the base value
					else if(isOctFlag == 1) tk->i=(int)strtol(createString(ptrStart+1,pCrtCh), NULL, 8);
					else tk->i = atoi(createString(ptrStart, pCrtCh));
					return CT_INT;
					break; 

			case 31: addTk(NOT);
					return NOT;
					break;

			case 32: addTk(NOTEQ);
					return NOTEQ;
					break;

			case 33: tk = addTk(CT_REAL);
					tk->r = atof(createString(ptrStart, pCrtCh));
					return CT_REAL;
					break;

			case 34: tk = addTk(CT_CHAR);
					tk->i = prevChar;
					return CT_CHAR;
					break;

			case 35: addTk(COMMA);
					return COMMA;
					break;

			case 36: addTk(SEMICOLON);
					return SEMICOLON;
					break;

			case 37: tk = addTk(CT_STRING);
					tk->text = (char*)malloc(pCrtCh - ptrStart + 1);
					strcpy(tk->text,createString(ptrStart+1, pCrtCh));
					pCrtCh++;
					return CT_STRING;
					break;

			case 38: addTk(DIV);
					return DIV;
					break;

			case 39: addTk(LESS);
					return LESS;
					break;

			case 40: addTk(LESSEQ);
					return LESSEQ;
					break;

			case 41: addTk(RACC);
					return RACC;
					break;

			case 42: addTk(LACC);
					return LACC;
					break;

			case 43: addTk(GREATEREQ);
					return GREATEREQ;
					break;

			case 44: addTk(EQUAL);
					return EQUAL;
					break;

			case 45: addTk(ADD);
					return ADD;
					break;

			case 46: addTk(SUB);
					return SUB;
					break;

			case 47: addTk(MUL);
					return MUL;
					break;

			case 48: addTk(DOT);
					return DOT;
					break;

			case 49: addTk(AND);
					return AND;
					break;

			case 50: addTk(OR);
					return OR;
					break;

			case 51: addTk(LPAR);
					return LPAR;
					break;

			case 52: addTk(RPAR);
					return RPAR;
					break;

			case 53: addTk(ASSIGN);
					return ASSIGN;
					break;

			case 54: addTk(GREATER);
					return GREATER;
					break;

			case 55: if(ch == '.')
					{
						pCrtCh++;
						state = 3;
					}
					else if (ch == 'e' || ch == 'E')
					{
						pCrtCh++;
						state = 6;
					}
					else if (isdigit(ch)) pCrtCh++;
					else
					{
						perror("\nINVALID CHARACTER!\n");
						exit(-1);
					}
					break;

			case 56: addTk(END);
					return END;
					break;

		}
	}
}

int main(int argc, char **argv)
{
	char *text = (char*)malloc(MAX);
	int n;

	//int fd = open("tests\\9.c", O_RDONLY);
	int fd = open(argv[1],O_RDONLY);		// choose one of those test files as first argument
	
	if( (n = read(fd, text, MAX-1)) == -1)
	{
		perror("\n Could not read file! \n");
		exit(-2);
	}

   	text[n] = '\0';
	printf("%s\n", text);
	
	pCrtCh = text;

	while(getNextToken() != END);

	close(fd);
	displayTokens();

	return 0;
}
