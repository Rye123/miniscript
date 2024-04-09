#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
      if (node->tok->type == TOKEN_NL)
	  printf("%s(%s('\\n'))", SymbolTypeString[node->type], TokenTypeString[node->tok->type]);
      else
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

// Removes all *_R nodes from the parse tree to clean it up.
void _astnode_remove_rec(ASTNode *node)
{
    size_t numChildren = node->numChildren;
    ASTNode **children   = malloc(sizeof(ASTNode *) * numChildren);
    memcpy(children, node->children, sizeof(ASTNode *) * numChildren);
    
    node->numChildren = 0;
    node->children = realloc(node->children, 0);
    for (size_t i = 0; i < numChildren; i++) {
	ASTNode *child = children[i];
	// Expand the child prior to expansion
	_astnode_remove_rec(child);
	
	SymbolType childType = children[i]->type;
	switch (child->type) {
	case SYM_EQUALITY_R:
	case SYM_COMPARISON_R:
	case SYM_SUM_R:
	case SYM_TERM_R:
            if (child->numChildren > 0) {
		// Shift children up
		//printf("\tAdd %lu children from %s\n", child->numChildren, SymbolTypeString[childType]);
		for (size_t j = 0; j < child->numChildren; j++)
		    astnode_addChildNode(node, child->children[j]);
            }
	    break;
	    
	default:
	    //printf("\tAdd node %s\n", SymbolTypeString[childType]);
	    astnode_addChildNode(node, child);
	}
    }
    
}

// Rebalances a given node such that its subtree is left-skewed with respect to others of the same type.
void _astnode_left_skew(ASTNode *node, SymbolType targetType)
{
    /*
	      After removing *_Rs, the tree is in the form:
	           (Node)
		 /   |    \
	       ...  SUM1   ...
		  /  |  \
	     TERM1   O1  SUM2
		       /   |  \
		  TERM2    O2  SUM3
		             /  |   \
		        TERM3   O3   SUM4
			              |
			             TERM4
	      This makes it so the right values are evaluated first -- we don't want that!
	      We want to rotate it to:
	                    (Node)
			  /   |    \
	                ...  SUM4   ...
			   /   |  \
			SUM3   O3  TERM4
		      /  |   \
		  SUM2   O2   TERM3
	        /  |  \
	     SUM1  O1  TERM2
	       |
	     TERM1	   
    */
    size_t childIndex = -1;
    ASTNode *curNode = NULL;
    // Identify target node
    for (size_t i = 0; i < node->numChildren; i++) {
	if (node->children[i]->type == targetType) {
	    curNode = node->children[i];
	    childIndex = i;
	    break;
	}
    }

    if (childIndex == -1) {
	printf("_astnode_left_skew Critical Error: Couldn't find targetType %s in node of type %s.", SymbolTypeString[targetType], SymbolTypeString[node->type]);
	exit(1);
    }

    // Invariant: curNode is either a node with ONE child or THREE
    ASTNode *rightChild = NULL;
    ASTNode *curOp = NULL;
    if (curNode->numChildren != 1 && curNode->numChildren != 3) {
	printf("_astnode_left_skew Critical Error: Given invalid number of children for curNode, curNode: %s, targetType: %s\n", SymbolTypeString[curNode->type], SymbolTypeString[targetType]);
	exit(1);
    }
    if (curNode->numChildren == 3) {
	rightChild = curNode->children[2];
	curOp = curNode->children[1];
    }
    
    while (rightChild != NULL) {
	ASTNode *nextRightChild = NULL;
	ASTNode *nextOp = NULL;

	// 0. Get rightChild and op for the NEXT iteration, since this operation would modify the children of rightChild
	// rightChild is either a node of the same type with either ONE child or THREE. If ONE child, then can leave as NULL -- next operation will end
	if (rightChild->numChildren != 1 && rightChild->numChildren != 3 && rightChild->type != targetType) {
	    printf("_astnode_left_skew Critical Error: Given invalid number of children for rightChild of curNode, rightChild: %s, curNode: %s, targetType: %s\n",
		   SymbolTypeString[rightChild->type],
		   SymbolTypeString[curNode->type],
		   SymbolTypeString[targetType]
		);
	    exit(1);
	}
	
        if (rightChild->numChildren == 3) {
	    nextRightChild = rightChild->children[2];
	    nextOp = rightChild->children[1];
        }

	// Invariant: curNode: ..., lChild, op, rChild.
	//                     rChild is same type as curNode
	//             rChild: lChildR, opR, rChildR
	//                               OR
	//                     childR
	//     Note: The "..." is the intermediate result of the PREVIOUS operation. At the start, it's empty
	// We want to rotate the tree, so that curNode becomes the left child of rChild
	// Further, op should become the SECOND child of rChild (after curNode)
	// End State:
	//            curNode: ..., lChild
	//             rChild: curNode, op, lChildR, opR, rChildR
	//      Note: Notice that the "..." is curNode, op from this operation.
	//            In the next operation, rChild effectively becomes:
	//             rChild: curNode, op, lChildR
	//            Which is the CORRECT way to evaluate a left-recursive rule.

        // 1. Remove op, rChild from curNode (invariant: curNode is ..., lChild, op, rChild)
	curNode->numChildren -= 2;
	curNode->children = realloc(curNode->children, sizeof(ASTNode) * curNode->numChildren);

	// 2. Store right child's current children
	size_t rcNumChildren = rightChild->numChildren;
	ASTNode **rcChildren = malloc(sizeof(ASTNode *) * rcNumChildren);
	rcChildren = memcpy(rcChildren, rightChild->children, sizeof(ASTNode *) * rcNumChildren);

	// 3. Reorder right child's children such that:
	//    Original: lChildR, opR, rChildR
	//         New: curNode, op, lChildR, opR, rChildR
	rightChild->numChildren = 0;
	rightChild->children = realloc(rightChild->children, 0);
	astnode_addChildNode(rightChild, curNode);
	astnode_addChildNode(rightChild, curOp);
	for (size_t i = 0; i < rcNumChildren; i++)
	    astnode_addChildNode(rightChild, rcChildren[i]);

	// 4. Cleanup
	free(rcChildren);

	// 5. Ensure invariant
	curNode = rightChild;
	curOp = nextOp;
	rightChild = nextRightChild;
	node->children[childIndex] = curNode;
    }
}

// Recursively applies the left skewing on left-recursive production rules.
void _astnode_left_skew_rec(ASTNode *node)
{
    if (node->type == SYM_TERMINAL)
	return;
    for (size_t i = 0; i < node->numChildren; i++) {
	printf("node %s, numChildren %lu, i %lu\n", SymbolTypeString[node->type], node->numChildren, i);
	_astnode_left_skew_rec(node->children[i]);
    }
    switch (node->type) {
    case SYM_EXPR:
	_astnode_left_skew(node, SYM_EQUALITY);
	break;
    case SYM_EQUALITY:
	_astnode_left_skew(node, SYM_COMPARISON);
	break;
    case SYM_COMPARISON:
	_astnode_left_skew(node, SYM_SUM);
	break;
    case SYM_SUM:
	_astnode_left_skew(node, SYM_TERM);
	break;
    default:
	break;
    }
}

ASTNode *astnode_gen(ASTNode *node)
{
    _astnode_remove_rec(node);
    //astnode_print(node)

    // 1. Recursively rebalance tree so that it's left-skewed.
    _astnode_left_skew_rec(node);

    return node;
}
