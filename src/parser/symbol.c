#include <stdio.h>
#include <stdlib.h>
#include "../lexer/token.h"
#include "symbol.h"

ASTNode *astnode_new(SymbolType type, Token *tok)
{
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = type;
    node->tok = tok;
    node->parent = NULL;
    node->children = malloc(sizeof(ASTNode *) * 0);
    node->numChildren = 0;
    return node;
}

void astnode_free(ASTNode *node)
{
    // Loop through children and free
    for (size_t i = 0; i < node->numChildren; i++) {
	ASTNode *child = node->children[i];
        astnode_free(child);
    }

    // Free self
    free(node->children);
    free(node);
}

void astnode_print(ASTNode *node)
{
  if (node->type == SYM_TERMINAL) {
      printf("%s(%s('%s'))", SymbolTypeString[node->type], TokenTypeString[node->tok->type], node->tok->lexeme);
      return;
  } else
      printf("%s", SymbolTypeString[node->type]);

  if (node->numChildren > 0) {
      printf("(");
      for (size_t i = 0; i < (node->numChildren) - 1; i++) {
	  astnode_print(node->children[i]);
	  printf(" ");
      }
      astnode_print(node->children[(node->numChildren) - 1]);
      printf(")");	
  }
}

int astnode_isExpanded(ASTNode *node)
{
    if (node->type == SYM_TERMINAL)
	return node->tok != NULL;

    // We expect a nonterminal to have children
    if (node->numChildren == 0)
	return 0;

    // Otherwise, loop through children
    for (size_t i = 0; i < node->numChildren; i++) {
	ASTNode *child = node->children[i];
	if (!astnode_isExpanded(child))
	    return 0;
    }
    return 1;
}

void astnode_addChildNode(ASTNode *parent, ASTNode *child)
{
    parent->numChildren++;
    parent->children = realloc(parent->children, sizeof(ASTNode *) * parent->numChildren);
    parent->children[parent->numChildren - 1] = child;
    child->parent = parent;
}

void astnode_addChild(ASTNode *node, const SymbolType type, Token *tok)
{
    node->numChildren++;
    node->children = realloc(node->children, sizeof(ASTNode *) * node->numChildren);
    node->children[node->numChildren - 1] = astnode_new(type, tok);
}

void astnode_addChildExp(ASTNode *node, const SymbolType expectedType) { astnode_addChild(node, expectedType, NULL); }

void astnode_eval(ASTNode *node)
{
    
}