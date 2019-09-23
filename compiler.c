#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define SAFEALLOC(var,Type) if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

enum { ID, END, CT_INT, CT_REAL, STRING, ADD, SUB, MUL, DIV,
    SEMICOLON, COMMA, LPAR, RPAR, LBRACKET, RBRACKET, LACC, RACC,
    DOT, AND, OR, NOT, NEQUAL, EQUAL, ASSIGN, LESS, LESSEQ,
    GREATER, GREATEREQ, BREAK, CHAR, DOUBLE, ELSE, FOR, IF, INT,
    RETURN, STRUCT, VOID, WHILE, CT_CHAR };

typedef struct _Token{
    int code;
    union {
        char *text;
        long int i;
        double r;
    };
    int line;
    struct _Token *next;
} Token;

typedef struct _Symbol Symbol;
typedef struct{
    Symbol **begin;     
    Symbol **end;       
    Symbol **after;     
} Symbols;
Symbols symbols;

enum{TB_INT,TB_DOUBLE,TB_CHAR,TB_STRUCT,TB_VOID};
typedef struct{
    int typeBase;   
    Symbol *s;      
    int nElements;  
}Type;

enum{CLS_VAR,CLS_FUNC,CLS_EXTFUNC,CLS_STRUCT};
enum{MEM_GLOBAL,MEM_ARG,MEM_LOCAL};
typedef struct _Symbol{
    const char *name;       
    int cls;                
    int mem;                
    Type type;
    int depth;              
    union{
        Symbols args;       
        Symbols members;    
    };
} Symbol;



Token *addTk(int code);
void err(const char *fmt,...);
void tkerr(const Token *tk,const char *fmt,...);
char *createString(const char* start, const char* end);
char escapeCharacter(char ch);
void printTokens();
void generateTokens(char *input);
int consume(int code);
int unit();
int declStruct();
int declVar();
int typeBase();
int arrayDecl1();
int arrayDecl();
int typeName1();
int declFunc();
int funcArg();
int stm();
int stmCompound();
int expr();
int exprAssign();
int exprOr();
void exprOr1();
int exprAnd();
void exprAnd1();
int exprEq();
void exprEq1();
int exprRel();
void exprRel1();
int exprAdd();
void exprAdd1();
int exprMul();
void exprMul1();
int exprCast();
int exprUnary();
int exprPostfix();
void exprPostfix1();
int exprPrimary();

Token *lastToken = NULL, *tokens = NULL;
Token *currentToken, *consumedTk;
int line = 0;
char *tokenNames[]={"ID", "END", "CT_INT", "CT_REAL", "STRING", "ADD", "SUB", "MUL", "DIV",
                 "SEMICOLON", "COMMA", "LPAR", "RPAR", "LBRACKET", "RBRACKET", "LACC", "RACC",
                 "DOT", "AND", "OR", "NOT", "NEQUAL", "EQUAL", "ASSIGN", "LESS", "LESSEQ",
                 "GREATER", "GREATEREQ", "BREAK", "CHAR", "DOUBLE", "ELSE", "FOR", "IF", "INT",
                 "RETURN", "STRUCT", "VOID", "WHILE", "CT_CHAR"};

void err(const char *fmt,...) {
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error: ");
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

void tkerr(const Token *tk,const char *fmt,...) {
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error in line %d: ",tk->line);
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

Token *addTk(int code) {
    Token *tk;
    SAFEALLOC(tk,Token);
    tk->code = code;
    tk->line = line;
    tk->next = NULL;
    if (lastToken) {
        lastToken->next = tk;
    } else {
        tokens = tk;
    }
    lastToken = tk;
    return tk;
}

char* createString(const char* start, const char* end) {
    int len = end - start + 1;
    char *ret = (char*)malloc(sizeof(char) * len);
    snprintf(ret, len, "%s", start);
    return ret;
}

char escapeCharacter(char ch) {
    switch(ch) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case '?': return '\?';
        case '"': return '\"';
        case '0': return '\0';
        case '\'': return '\'';
        case '\\': return '\\';
    }
    return ch;
}

void printTokens() {
    Token *current = tokens;
    while(current != NULL) {
        printf("%s", tokenNames[current->code]);
        switch(current->code) {
            case ID:
            case STRING:
                printf(":%s", current->text);
                break;
            case CT_CHAR:
                printf(":%c", (int)current->i);
                break;
            case CT_INT:
                printf(":%d",current->i);
                break;
            case CT_REAL:
                printf(":%f", current->r);
                break;
        }
        printf(" ");
        current = current->next;
    }
    printf("\nLines of code: %i\n", line);
}


// Lexical Analysis

void generateTokens(char *input) {
    int state = 0;
    char ch;
    char *pStartCh, *pCrtCh = input;
    Token *tk;
    while(1) {
        ch = (*pCrtCh);
        switch(state) {
            case 0:
                pStartCh = pCrtCh;
                if (ch == '\n') {
                    line++;
                    pCrtCh++;
                } else if(ch > '0' && ch <= '9') {
                    state = 1;
                    pCrtCh++;
                } else if(ch == '0') {
                    state = 2;
                    pCrtCh++;
                } else if(ch == ' ' || ch == '\t' || ch == '\r') {
                    pCrtCh++;
                } else if(ch == '/') {
                    pCrtCh++;
                    state = 49;
                } else if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                    pCrtCh++;
                    state = 36;
                } else if(ch == '\'') {
                    pCrtCh++;
                    state = 14;
                } else if(ch == '\"') {
                    pCrtCh++;
                    state = 30;
                } else if(ch == ',') {
                    pCrtCh++;
                    addTk(COMMA);
                } else if(ch == ';') {
                    pCrtCh++;
                    addTk(SEMICOLON);
                } else if(ch == '(') {
                    pCrtCh++;
                    addTk(LPAR);
                } else if(ch == ')') {
                    pCrtCh++;
                    addTk(RPAR);
                } else if(ch == '[') {
                    pCrtCh++;
                    addTk(LBRACKET);
                } else if(ch == ']') {
                    pCrtCh++;
                    addTk(RBRACKET);
                } else if(ch == '{') {
                    pCrtCh++;
                    addTk(LACC);
                } else if(ch == '}') {
                    pCrtCh++;
                    addTk(RACC);
                } else if(ch == '+') {
                    pCrtCh++;
                    addTk(ADD);
                } else if(ch == '-') {
                    pCrtCh++;
                    addTk(SUB);
                } else if(ch == '*') {
                    pCrtCh++;
                    addTk(MUL);
                } else if(ch == '.') {
                    pCrtCh++;
                    addTk(DOT);
                } else if(ch == '&') {
                    pCrtCh++;
                    ch = *pCrtCh;
                    if(ch == '&') {
                        pCrtCh++;
                        addTk(AND);
                    } else {
                        tkerr(tk, "ERROR: Expected binary operator.\n");
                    }
                } else if(ch == '|') {
                    pCrtCh++;
                    ch = *pCrtCh;
                    if(ch == '|'){
                        pCrtCh++;
                        addTk(OR);
                    }
                    else {
                        tkerr(tk, "ERROR: Expected binary operator.\n");
                    }
                } else if(ch == '!') {
                    pCrtCh++;
                    ch = *pCrtCh;
                    if(ch == '=') {
                        pCrtCh++;
                        addTk(NEQUAL);
                    } else {
                        addTk(NOT);
                    }
                } else if(ch == '=') {
                    pCrtCh++;
                    ch = *pCrtCh;
                    if(ch == '=') {
                        pCrtCh++;
                        addTk(EQUAL);
                    } else {
                        addTk(ASSIGN);
                    }
                } else if(ch == '<') {
                    pCrtCh++;
                    ch = *pCrtCh;
                    if(ch == '=') {
                        pCrtCh++;
                        addTk(LESSEQ);
                    }
                    else{
                        addTk(LESS);
                    }
                } else if (ch == '>') {
                    pCrtCh++;
                    ch = *pCrtCh;
                    if(ch == '='){
                        pCrtCh++;
                        addTk(GREATEREQ);
                    }
                    else{
                        addTk(GREATER);
                    }
                } else if(ch == '\0') {
                    addTk(END);
                    return;
                } else {
                    tkerr(tk, "ERROR: Unrecognized character in state %i.", state);
                }
                break;
            case 1:
                if(ch >= '0' && ch <= '9') {
                    pCrtCh++;
                } else if(ch == '.') {
                    pCrtCh++;
                    state = 7;
                } else if(ch == 'e' || ch == 'E') {
                    pCrtCh++;
                    state = 10;
                } else {
                    state = 6;
                }
                break;
            case 2:
                if(ch >= '0' && ch < '8'){
                    state = 3;
                    pCrtCh++;
                }
                else if(ch == 'x' || ch == 'X'){
                    state = 4;
                    pCrtCh++;
                }
                else if(ch == '.'){
                    pCrtCh++;
                    state = 8;
                }
                else if(ch == 'e' || ch =='E'){
                    pCrtCh++;
                    state = 9;
                }
                else {
                    state = 6;
                }
                break;
            case 3:
                if(ch >= '0' && ch < '8') {
                    state = 3;
                    pCrtCh++;
                } else {
                    state = 6;
                }
                break;
            case 4:
                if((ch >= '0' && ch <='9') || (ch>='A' && ch <='F') || (ch>='a' && ch<='f')) {
                    state = 5;
                    pCrtCh++;
                } else {
                    tkerr(tk, "ERROR: Unrecognized character in state %i.", state);
                }
                break;
            case 5:
                if((ch >= '0' && ch <='9') || (ch>='A' && ch <='F') || (ch>='a' && ch<='f')) {
                    pCrtCh++;
                } else {
                    state = 6;
                }
                break;
            case 6:
                tk = addTk(CT_INT);
                tk->i = strtol(pStartCh, NULL, 0);
                state = 0;
                break;
            case 7:
                if(ch >= '0' && ch <= '9') {
                    pCrtCh++;
                    state = 8;
                } else {
                    tkerr(tk, "ERROR: Unrecognized character in state %i. Float expected.", state);
                }
                break;
            case 8:
                if(ch >= '0' && ch <= '9') {
                    pCrtCh++;
                } else {
                    state = 9;
                }
                break;
            case 9:
                if(ch == 'e' || ch == 'E') {
                    pCrtCh++;
                    state = 10;
                } else {
                    state = 13;
                }
                break;
            case 10:
                if(ch == '+' || ch == '-') {
                    pCrtCh++;
                    state = 11;
                } else if(ch >= '0' && ch <= '9') {
                    pCrtCh++;
                    state = 12;
                } else {
                    pCrtCh++;
                    state = 11;
                }
                break;
            case 11:
                if(ch >= '0' && ch <='9') {
                    pCrtCh++;
                    state = 12;
                } else {
                    tkerr(tk, "ERROR: Unrecognized character in state %i. Digit expected.", state);
                }
                break;
            case 12:
                if(ch >= '0' && ch <='9'){
                    pCrtCh++;
                } else {
                    state = 13;
                }
                break;
            case 13:
                tk = addTk(CT_REAL);
                tk->r = strtod(pStartCh, NULL);
                state = 0;
                break;
            case 49:
                if(ch == '*') {
                    pCrtCh++;
                    state = 52;
                } else if(ch == '/') {
                    pCrtCh++;
                    state = 50;
                } else {
                    addTk(DIV);
                    state = 0;
                }
                break;
            case 52:
                if(ch == '*'){
                    pCrtCh++;
                    state = 53;
                } else {
                    pCrtCh++;
                }
                break;
            case 53:
                if(ch == '/') {
                    pCrtCh++;
                    state = 0;
                } else if(ch == '*') {
                    pCrtCh++;
                } else {
                    pCrtCh++;
                    state = 52;
                }
                break;
            case 50:
                if(ch != '\n' && ch != '\r' && ch != '\0'){
                    pCrtCh++;
                } else if(ch == '\n') {
                    pCrtCh++;
                    state = 0;
                    line++;
                } else {
                    pCrtCh++;
                    state = 0;
                }
                break;
            case 36:
                if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_') {
                    if(!memcmp(pStartCh,"break", 5)) {
                        tk = addTk(BREAK);
                        pCrtCh += 3;
                        state = 0;
                    } else if(!memcmp(pStartCh,"char", 4)) {
                        tk = addTk(CHAR);
                        pCrtCh += 2;
                        state = 0;
                    } else if(!memcmp(pStartCh,"double", 6)) {
                        tk = addTk(DOUBLE);
                        pCrtCh += 4;
                        state = 0;
                    } else if(!memcmp(pStartCh,"else", 4)) {
                        tk = addTk(ELSE);
                        pCrtCh += 2;
                        state = 0;
                    } else if(!memcmp(pStartCh,"for", 3)) {
                        tk = addTk(FOR);
                        pCrtCh += 1;
                        state = 0;
                    } else if(!memcmp(pStartCh,"if", 2)) {
                        tk = addTk(IF);
                        state = 0;
                    } else if(!memcmp(pStartCh,"int", 3)) {
                        tk = addTk(INT);
                        pCrtCh += 2;
                        state = 0;
                    } else if(!memcmp(pStartCh,"return", 6)) {
                        tk = addTk(RETURN);
                        pCrtCh += 4;
                        state = 0;
                    } else if(!memcmp(pStartCh,"struct", 6)) {
                        tk = addTk(STRUCT);
                        pCrtCh += 4;
                        state = 0;
                    } else if(!memcmp(pStartCh,"void", 4)) {
                        tk = addTk(VOID);
                        pCrtCh += 2;
                        state = 0;
                    } else if(!memcmp(pStartCh,"while", 5)) {
                        tk = addTk(WHILE);
                        pCrtCh += 3;
                        state = 0;
                    }
                    pCrtCh++;
                } else {
                    tk = addTk(ID);
                    char* str =  createString(pStartCh, pCrtCh);
                    tk->text = str;
                    state = 0;
                }
                break;
            case 14:
                if(ch == '\\') {
                    pCrtCh++;
                    state = 16;
                } else {
                    pCrtCh++;
                    state = 17;
                }
                break;
            case 16:
                if(strchr("abfnrtv'?\"\\0", ch)) {
                    pCrtCh++;
                    state = 17;
                } else {
                    tkerr(tk,"ERROR: Escape character expected\n");
                }
                break;
            case 17:
                if(ch == '\'') {
                    tk = addTk(CT_CHAR);
                    char *str = createString(pStartCh + 1, pCrtCh), c;
                    if(strchr(str, '\\') != NULL) {
                        c = escapeCharacter(*(str + 1));
                    } else {
                        c = *str;
                    }
                    tk->i = c;
                    pCrtCh++;
                    state = 0;
                } else {
                    tkerr(tk, "ERROR: Expected character\n");
                }
                break;
            case 30:
                if(ch == '\\') {
                    pCrtCh++;
                    state = 32;
                } else {
                    state = 33;
                }
                break;
            case 32:
                if(strchr("abfnrtv'?\"\\0", ch)) {
                    pCrtCh++;
                    state = 33;
                } else {
                    tkerr(tk, "ERROR: Escape sequence not recognized\n");
                }
                break;
            case 33:
                if(ch == '\"') {
                    tk = addTk(STRING);
                    char *str = createString(pStartCh + 1, pCrtCh), *p;
                    while((p = strchr(str, '\\')) != NULL) {
                        memmove(p, p + 1, strlen(p));
                        *p = escapeCharacter(*p);
                    }
                    tk->text = str;
                    pCrtCh++;
                    state = 0;
                } else {
                    state = 30;
                    pCrtCh++;
                }
                break;
        }
    }
}


// Syntactic Analysis

int consume(int code) {
    if(currentToken->code == code) {
        consumedTk = currentToken;
        currentToken = currentToken->next;
        return 1;
    }
    return 0;
}

// unit: ( declStruct | declFunc | declVar )* END
int unit() {
    currentToken = tokens;

    while(1) {
        if(declStruct()) {}
        else if(declFunc()) {}
        else if(declVar()) {}
        else break;
    }
    if(!consume(END)) tkerr(currentToken,"missing END token");

    return 1;
}


// declStruct: STRUCT ID LACC declVar* RACC SEMICOLON
int declStruct() {
    Token *startTk = currentToken;
    if(!consume(STRUCT)) return 0;
    if(!consume(ID)) tkerr(currentToken,"ID expected after struct");
    if(!consume(LACC)){
       currentToken = startTk;
        return 0;
    }
    while(1) {
        if(declVar()) {}
        else break;
    }
    if(!consume(RACC)) tkerr(currentToken,"Missing } in struct declaration");
    if(!consume(SEMICOLON)) tkerr(currentToken,"Missing ; in struct declaration");
    return 1;
}

// declVar:  typeBase ID arrayDecl? ( COMMA ID arrayDecl? )* SEMICOLON
int declVar() {
    //Token *startTk = currentToken;
    if(!typeBase()) return 0;
    if(!consume(ID)) tkerr(currentToken, "ID expected after type base");
    if(!arrayDecl()) { }
    while(1) {

        if(!consume(COMMA)) break;
        if(!consume(ID)) tkerr(currentToken, "ID expected");
        if(!arrayDecl()) {}
    }
    if(!consume(SEMICOLON)) {
        return 0;
    }
    return 1;
}

// typeBase: INT | DOUBLE | CHAR | STRUCT ID
int typeBase() {
    if(consume(INT)) {}
    else if(consume(DOUBLE)) {}
    else if(consume(CHAR)) {}
    else if(consume(STRUCT)) {
        if(!consume(ID)) tkerr(currentToken, "ID expected after struct");
    }
    else return 0;
    return 1;
}
//arrayDecl: LBRACKET expr? RBRACKET ;
int arrayDecl() {
    if(!consume(LBRACKET)) return 0;
    expr();
    if(!consume(RBRACKET)) tkerr(currentToken, "missing ] from array declaration");
    return 1;
}

// typeName: typeBase arrayDecl?
int typeName1() {
    if(!typeBase()) return 0;
    if(!arrayDecl()) {}
    return 1;
}

// declFunc: ( typeBase MUL? | VOID ) ID
//                         LPAR ( funcArg ( COMMA funcArg )* )? RPAR
//                         stmCompound
int declFunc() {
   Token *back = currentToken;
    if(typeBase()) {
        if(consume(MUL)) {}
    } else if (consume(VOID)) {}
    else return 0;
    if(!consume(ID)) {
        currentToken = back;
        return 0;
    }
    if(!consume(LPAR)) {
        currentToken = back;
        return 0;
    }

    if(funcArg()) {
        while(1) {
            if(consume(COMMA)){
                if(!funcArg()) tkerr(currentToken, "missing func arg in stm");
            }
            else
                break;
        }
    }
    if(!consume(RPAR)) tkerr(currentToken, "missing ) in func declaration");

    if(!stmCompound()) tkerr(currentToken, "compound statement expected");
    return 1;
}

// funcArg: typeBase ID arrayDecl?
int funcArg() {
    if(!typeBase()) return 0;
    if(!consume(ID)) tkerr(currentToken, "ID missing in function declaration");
    if(!arrayDecl()) {}
    return 1;
}

// stm: stmCompound
//            | IF LPAR expr RPAR stm ( ELSE stm )?
//            | WHILE LPAR expr RPAR stm
//            | FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR stm
//            | BREAK SEMICOLON
//            | RETURN expr? SEMICOLON
//            | expr? SEMICOLON
int stm() {
    if(stmCompound()) {}
    else if(consume(IF)) {
        if(!consume(LPAR)) tkerr(currentToken, "missing ( after if") ;
        if(!expr()) tkerr(currentToken, "Expected expression after ( ");
        if(!consume(RPAR)) tkerr(currentToken, "missing ) after if") ;
        if(!stm()) tkerr(currentToken, "Expected statement after if ") ;
        if(consume(ELSE)) {
            if(!stm()) tkerr(currentToken, "Expected statement after else ") ;
        }
    }
    else if(consume(WHILE)) {
        if(!consume(LPAR)) tkerr(currentToken, "missing ( after while") ;
        if(!expr()) tkerr(currentToken, "Expected expression after ( ") ;
        if(!consume(RPAR)) tkerr(currentToken, "missing ) after while") ;
        if(!stm()) tkerr(currentToken, "Expected statement after while ") ;
    }
    else if(consume(FOR)) {
        if(!consume(LPAR)) tkerr(currentToken, "missing ( after for") ;
        expr();
        if(!consume(SEMICOLON)) tkerr(currentToken, "missing ; in for") ;
        expr();
        if(!consume(SEMICOLON)) tkerr(currentToken, "missing ; in for") ;
        expr();
        if(!consume(RPAR)) tkerr(currentToken, "missing ) after for") ;
        if(!stm()) tkerr(currentToken, "Expected statement after for ") ;
    }
    else if(consume(BREAK)) {
        if(!consume(SEMICOLON)) tkerr(currentToken, "missing ; after break") ;
    }
    else if(consume(RETURN)) {
        expr();
        if(!consume(SEMICOLON)) tkerr(currentToken, "missing ; after return") ;
    }
    else if(expr()) {
        if(!consume(SEMICOLON)) tkerr(currentToken,"missing ; after expression in statement");
    }
    else if(consume(SEMICOLON)) {}
    else return 0;
    return 1;
}

// stmCompound: LACC ( declVar | stm )* RACC
int stmCompound() {
    if(!consume(LACC)) return 0;
    while(1) {
        if(declVar()) {}
        else if(stm()) {}
        else break;
    }
    if(!consume(RACC)) tkerr(currentToken, "Expected } in compound statement");
    return 1;
}

// expr: exprAssign
int expr() {
    if(!exprAssign()) return 0;
    return 1;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
int exprAssign() {
    Token *startTk = currentToken;
    if(exprUnary()) {
        if(consume(ASSIGN)) {
            if(!exprAssign()) tkerr(currentToken, "Expected assign in expression");
            return 1;
        }
      currentToken = startTk;
    }
    if(exprOr()) {}
    else return 0;
    return 1;
}

// exprOr: exprOr OR exprAnd | exprAnd
// Remove left recursion:
//     exprOr: exprAnd exprOr1
//     exprOr1: OR exprAnd exprOr1
int exprOr() {
    if(!exprAnd()) return 0;
    exprOr1();
    return 1;
}

void exprOr1() {
    if(consume(OR)) {
        if(!exprAnd()) tkerr(currentToken,"missing expression after OR");
        exprOr1();
    }
}

// exprAnd: exprAnd AND exprEq | exprEq
// Remove left recursion:
//     exprAnd: exprEq exprAnd1
//     exprAnd1: AND exprEq exprAnd1
int exprAnd() {
    if(!exprEq()) return 0;
    exprAnd1();
    return 1;
}

void exprAnd1() {
    if(consume(AND)) {
        if(!exprEq()) tkerr(currentToken,"missing expression after AND");
        exprAnd1();
    }
}

// exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
// Remove left recursion:
//     exprEq: exprRel exprEq1
//     exprEq1: ( EQUAL | NOTEQ ) exprRel exprEq1
int exprEq() {
    if(!exprRel()) return 0;
    exprEq1();
    return 1;
}

void exprEq1() {
    if(consume(EQUAL)) {}
    else if(consume(NEQUAL)) {}
    else return;
    if(!exprRel()) tkerr(currentToken,"missing expressiong after =");
    exprEq1();
}

// exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
// Remove left recursion:
//     exprRel: exprAdd exprRel1
//     exprRel1: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRel1
int exprRel() {
    if(!exprAdd()) return 0;
    exprRel1();
    return 1;
}

void exprRel1() {
    if(consume(LESS)) {}
    else if(consume(LESSEQ)) {}
    else if(consume(GREATER)) {}
    else if(consume(GREATEREQ)) {}
    else return;
    if(!exprAdd()) tkerr(currentToken,"missing expression after relationship");
    exprRel1();
}

// exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
// Remove left recursion:
//     exprAdd: exprMul exprAdd1
//     exprAdd1: ( ADD | SUB ) exprMul exprAdd1
int exprAdd() {
    if(!exprMul()) return 0;
    exprAdd1();
    return 1;
}

void exprAdd1() {
    if(consume(ADD)) {}
    else if(consume(SUB)) {}
    else return;
    if(!exprMul()) tkerr(currentToken,"missing expressiong after + or -");
    exprAdd1();
}

// exprMul: exprMul ( MUL | DIV ) exprCast | exprCast
// Remove left recursion:
//     exprMul: exprCast exprMul1
//     exprMul1: ( MUL | DIV ) exprCast exprMul1
int exprMul() {
    if(!exprCast()) return 0;
    exprMul1();
    return 1;
}

void exprMul1() {
    if(consume(MUL)) {}
    else if(consume(DIV)) {}
    else return;
    if(!exprCast()) tkerr(currentToken,"missing expressiong after * or /");
    exprMul1();
}

// exprCast: LPAR typeName RPAR exprCast | exprUnary
int exprCast() {
    Token *startTk = currentToken;
    if(consume(LPAR)) {
        if(typeName1(NULL)) {
            if(consume(RPAR)) {
                if(exprCast()) { return 1; }
            }
        }
        currentToken = startTk;
    }
    if(exprUnary()) {}
    else return 0;
    return 1;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
int exprUnary() {
    if(consume(SUB)) {
        if(!exprUnary()) tkerr(currentToken,"missing unary expression after -");
    }
    else if(consume(NOT)) {
        if(!exprUnary()) tkerr(currentToken,"missing unary expression after !");
    }
    else if(exprPostfix()) {}
    else return 0;
    return 1;
}

// exprPostfix: exprPostfix LBRACKET expr RBRACKET
//            | exprPostfix DOT ID
//            | exprPrimary
// Remove left recursion:
//     exprPostfix: exprPrimary exprPostfix1
//     exprPostfix1: ( LBRACKET expr RBRACKET | DOT ID ) exprPostfix1
int exprPostfix() {
    if(!exprPrimary()) return 0;
    exprPostfix1();
    return 1;
}

void exprPostfix1() {
    if(consume(LBRACKET)) {
        if(!expr()) tkerr(currentToken,"missing expression after (");
        if(!consume(RBRACKET)) tkerr(currentToken,"missing ) after expression");
    } else if(consume(DOT)) {
        if(!consume(ID)) tkerr(currentToken,"error");
    } else return;
    exprPostfix1();
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//            | CT_INT
//            | CT_REAL
//            | CT_CHAR
//            | CT_STRING
//            | LPAR expr RPAR
int exprPrimary() {
    Token *startTk = currentToken;
    if(consume(ID)) {
        if(consume(LPAR)) {
            if(expr()) {
                while(1) {
                    if(!consume(COMMA)) break;
                    if(!expr()) tkerr(currentToken,"missing expression after , in primary expression");
                }
            }
            if(!consume(RPAR)) tkerr(currentToken,"missing )");
        }
    }
    else if(consume(CT_INT)) {}
    else if(consume(CT_REAL)) {}
    else if(consume(CT_CHAR)) {}
    else if(consume(STRING)) {}
    else if(consume(LPAR)) {
        if(!expr()) {
            currentToken = startTk;
            return 0;
        }
        if(!consume(RPAR)) tkerr(currentToken,"missing ) after expression");
    }
    else return 0;
    return 1;
}

int main(int argc, char **argv) {
    
    char *file_path = "tests/9.c";
    struct stat l_stat;
    int size;
    FILE *file;

    if(stat(file_path, &l_stat) == 0) {
        size = l_stat.st_size;
    } else {
        printf("ERROR: Could not open file for stats.\n");
        return -1;
    }

    if((file = fopen(file_path, "r")) == NULL) {
        printf("We cannot open this file.\n");
        return -1;
    }

    char buffer[size*sizeof(char)];
    int ret;
    if((ret = (fread(buffer, sizeof(char), size*sizeof(char), file))) <= 0) {
        printf("ERROR: Cannot read from file.\n");
        return -1;
    }

    if(ret < size) {
        buffer[ret] = '\0' ;
    } else {
        buffer[size] = '\0';
    }

    generateTokens(buffer);
    printf("\n");
    if (unit()) {
        printf("Syntax is correct.\n");
    }

    return 0;

}

