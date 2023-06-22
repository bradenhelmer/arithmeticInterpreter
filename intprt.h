#ifndef INTPRT_HH
#define INTPRT_HH

// Buffer
#define MAX_BUFFER_SIZE 64
static char *buffer;
char *alloc_input(void);

// Token Macros
typedef enum {
#define TOKEN(X) X,
#include "tokens.h"
} t_kind;

const char *tokenNames[] = {
#define TOKEN(X) #X,
#include "tokens.h"
};

typedef struct {
  t_kind kind;
  char *start;
  char *end;
  unsigned long length;
} token;

static token *currentToken;

// Lex functions
void getNextToken(void);
char getTokSeq(t_kind kind);
static void lexAndPrintTokens(char *saved);
void scanNumber(token *tok);
void skipWhiteSpace(void);

// Parser
typedef struct Node {
  int isLeaf;
  union {
    t_kind op;
    long value;
  };
  struct Node *left;
  struct Node *right;
} Node;

#define NODE_LIST_SIZE 64
static Node *ast;
Node *nodeList[NODE_LIST_SIZE];
int nodeCount;

Node *createOpNode(t_kind op, Node *left, Node *right);
Node *parseNumber();
Node *parseFactor();
Node *parseExpo();
Node *parseTerm();
Node *parseExpression();
void buildAST();
long executeAST(Node *ast);
void addNode(Node *node);
void extendNodeList();
void freeNodes();

#endif  // INTPRT_HH

#define INTPRT_IMPL
#ifdef INTPRT_IMPL
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

char getTokSeq(t_kind kind) {
  char seq;
  switch (kind) {
    case t_add:
      seq = '+';
      break;
    case t_sub:
      seq = '-';
      break;
    case t_mul:
      seq = '*';
      break;
    case t_div:
      seq = '/';
      break;
    case t_mod:
      seq = '%';
      break;
    case t_expo:
      seq = '^';
      break;
    default:
      seq = 'N';
  }
  return seq;
}

char *alloc_input(void) {
  char *input = (char *)malloc(MAX_BUFFER_SIZE * sizeof(char));
  return input;
}

void getNextToken() {
  if (currentToken == NULL) {
    currentToken = (token *)malloc(sizeof(token));
  }
  skipWhiteSpace();
  switch (*buffer) {
    case '(':
      currentToken->kind = t_o_paren;
      break;
    case ')':
      currentToken->kind = t_c_paren;
      break;
    case '+':
      currentToken->kind = t_add;
      break;
    case '-':
      currentToken->kind = t_sub;
      break;
    case '*':
      currentToken->kind = t_mul;
      break;
    case '/':
      currentToken->kind = t_div;
      break;
    case '%':
      currentToken->kind = t_mod;
      break;
    case '^':
      currentToken->kind = t_expo;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      currentToken->kind = t_num;
      break;
    case '\n':
      currentToken->kind = t_eoe;
      break;
    default:
      currentToken->kind = t_err;
      break;
  }
  if (currentToken->kind == t_err) {
    printf("Error at: %c\n", *buffer);
    exit(1);
  }
  if (currentToken->kind == t_num) {
    scanNumber(currentToken);
  } else {
    currentToken->start = buffer;
    currentToken->end = buffer;
  }
  currentToken->length = (currentToken->end - currentToken->start) + 1;
  buffer++;
}

static void lexAndPrintTokens(char *saved) {
  int tokenCounter = 1;
  getNextToken();
  do {
    if (currentToken->kind == t_num) {
      printf("Token #%d: %s ", tokenCounter,
             tokenNames[currentToken->kind]);
      for (char *curr = currentToken->start; curr <= currentToken->end;
           ++curr) {
        printf("%c", *curr);
        printf("\n");
      }
    } else {
      printf("Token #%d: %s %c\n", tokenCounter,
             tokenNames[currentToken->kind],
             getTokSeq(currentToken->kind));
    }
    getNextToken();
    tokenCounter++;
  } while (currentToken->kind != t_eoe);
  buffer = saved;
}

// Scans numeric literal, finds start and end addresses.
void scanNumber(token *tok) {
  tok->start = buffer;
  while (isdigit(*(buffer + 1))) {
    buffer++;
  }
  tok->end = buffer;
}

// Skips whitespace in buffer
void skipWhiteSpace() {
  while (isspace(*buffer) && *buffer != '\n') buffer++;
}

Node *createOpNode(t_kind op, Node *left, Node *right) {
  Node *node = (Node *)malloc(sizeof(Node));
  node->op = op;
  node->left = left;
  node->right = right;
  node->isLeaf = 0;
  addNode(node);
  return node;
}

Node *parseNumber() {
  assert(currentToken->kind == t_num);
  char *strNum = (char *)malloc(currentToken->length * sizeof(char));
  assert(strNum != NULL);
  memcpy(strNum, currentToken->start, currentToken->length);
  char *end;
  long value = strtol(strNum, &end, 10);
  Node *left = (Node *)malloc(sizeof(Node));
  addNode(left);
  assert(left != NULL);
  left->isLeaf = 1;
  left->value = value;
  free(strNum);
  return left;
}

Node *parseFactor() {
  Node *left;
  if (currentToken->kind == t_num) {
    left = parseNumber();
    getNextToken();
  } else if (currentToken->kind == t_o_paren) {
    getNextToken();
    left = parseExpression();
    if (currentToken->kind != t_c_paren) {
      printf("Missing Closing Paren!\n");
      exit(1);
    } else {
      getNextToken();
    }
  } else {
    printf("Error parsing\n");
    exit(1);
  }
  return left;
}

Node *parseExpo() {
  Node *left;
  Node *right;
  left = parseFactor();
  while (currentToken->kind == t_expo) {
    t_kind op = currentToken->kind;
    getNextToken();
    right = parseExpo();
    left = createOpNode(op, left, right);
  }
  return left;
}

Node *parseTerm() {
  Node *left;
  Node *right;
  left = parseExpo();
  while (currentToken->kind == t_mul || currentToken->kind == t_div ||
         currentToken->kind == t_mod) {
    t_kind op = currentToken->kind;
    getNextToken();
    right = parseExpo();
    left = createOpNode(op, left, right);
  }
  return left;
}

Node *parseExpression() {
  Node *left;
  Node *right;
  left = parseTerm();
  while (currentToken->kind == t_add || currentToken->kind == t_sub) {
    t_kind op = currentToken->kind;
    getNextToken();
    right = parseTerm();
    left = createOpNode(op, left, right);
  }
  return left;
}

void buildAST() {
  getNextToken();
  ast = parseExpression();
}

void addNode(Node *node) {
  if (nodeList[0] == NULL) {
    nodeCount = 0;
  }
  nodeList[nodeCount] = node;
  nodeCount++;
}

void freeNodes() {
  for (int i = 0; i < nodeCount; i++) {
    free(nodeList[i]);
    nodeList[i] = NULL;
  }
  nodeCount = 0;
  ast = NULL;
}

long executeAST(Node *node) {
  if (node->isLeaf) {
    return node->value;
  }
  switch (node->op) {
    long right;
    case t_add:
      return executeAST(node->left) + executeAST(node->right);
    case t_sub:
      return executeAST(node->left) - executeAST(node->right);
    case t_mul:
      return executeAST(node->left) * executeAST(node->right);
    case t_div:
      right = executeAST(node->right);
      if (right == 0) {
        printf("Cannot divide by 0! Exiting\n");
        exit(1);
      }
      return executeAST(node->left) / right;
    case t_mod:
      right = executeAST(node->right);
      if (right == 0) {
        printf("Cannot mod by 0! Exiting\n");
        exit(1);
      }
      return executeAST(node->left) % right;
    case t_expo:
      return powl(executeAST(node->left), executeAST(node->right));
    default:
      printf("Error executing expression\n");
      return 0;
  }
}

#undef INTPRT_IMPL
#endif  // INTPRT_IMPL
