#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

enum Token {
    TokenEof = -1,

    // commands
    TokenDef = -2,

    // primary
    TokenIdentifier = -3,
    TokenNumber = -4
};

std::string identifierStr; // Filled in if tok_identifier
double numVal;

int GetToken() {
    static int lastChar = ' ';
    while (isspace(lastChar)) lastChar = GetChar();

    if (isalpha(LastChar)) { // identifier: [a-zA-Z]*
        identifierStr = lastChar;
        while (isalnum((LastChar = GetChar()))) identifierStr += lastChar;

        if (identifierStr == "def") return TokenDef;
    
        return TokenIdentifier;  
    }  

    if (isdigit(lastChar)) { // Number: [0-9.]+
        std::string numStr;
        do {
            numStr += lastChar;
            lastChar = GetChar();
        } while (isdigit(lastChar));

        numVal = strtod(numStr.c_str(), nullptr);
        return TokenNumber;
    }

    if (lastChar == EOF) return tok_eof;

    int thisChar = lastChar;
    lastChar = GetChar();
    return thisChar;
}

int CurrentToken;

int GetNextToken() { 
    CurrentToken = GetToken(); 
    return CurrentToken;
}

std::map<char, int> BinopPrecedence;

int GetTokenPrecedence() {
    if (!isascii(CurrentToken)) return -1;

    // Make sure it's a declared binop.
    int tokenPrecedence = BinopPrecedence[CurrentToken];
    if (tokenPrecedence <= 0) return -1;
    return tokenPrecedence;
}

std::unique_ptr<ExprAST> ParseBinOpRHS(int expressionPrecedence,
                        std::unique_ptr<ExprAST> LHS) {
    // If this is a binop, find its precedence.
    while (true) {
        int tokenPrecedence = GetTokenPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (tokenPrecedence < expressionPrecedence) return LHS;

        // Okay, we know this is a binop.
        int binaryOperator = CurrentToken;
        GetNextToken(); // eat binop

        // Parse the primary expression after the binary operator.
        auto RHS = ParsePrimary();
        if (!RHS) return nullptr;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int nextPrecedence = GetTokenPrecedence();
        if (tokenPrecedence < nextPrecedence) {
            RHS = ParseBinOpRHS(tokenPrecedence + 1, std::move(RHS));
            if (!RHS) return nullptr;
        }

        // Merge LHS/RHS.
        LHS = llvm::make_unique<BinaryExprAST>(binaryOperator, 
                                                std::move(LHS),
                                                std::move(RHS));
    }
}

std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS) return nullptr;

    return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = llvm::make_unique<NumberExprAST>(numVal);
    GetNextToken(); // consume the number
    return std::move(Result);
}

std::unique_ptr<ExprAST> ParseIdentifierExpression() {
    std::string idName = identifierStr;

    getNextToken(); // eat identifier.

    if (CurTok != '(') return llvm::make_unique<VariableExprAST>(IdName);

    // Call.
    getNextToken(); // eat (
    std::vector<std::unique_ptr<ExprAST>> args;
    if (currentToken != ')') {
        while (true) {
            if (auto arg = ParseExpression()) 
                args.push_back(std::move(arg));
            else
                return nullptr;

            if (currentToken == ')') break;

            if (currentToken != ',') return LogError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    // Eat the ')'.
    getNextToken();

    return llvm::make_unique<CallExprAST>(idName, std::move(args));
}

std::unique_ptr<ExprAST> ParseNumberExpression() {
    auto Result = llvm::make_unique<NumberExprAST>(NumVal);
    getNextToken(); // consume the number
    return std::move(Result);
}

static std::unique_ptr<ExprAST> ParseParenthesesExpression() {
    GetNextToken(); // eat (.
    auto value = ParseExpression();
    if (!value) return nullptr;

    if (CurrentToken != ')')
        return LogError("expected ')'");
    GetNextToken(); // eat ).
    return value;
}

std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurrentToken) {
        case TokenIdentifier:
            return ParseIdentifierExpr();
        case TokenNumber:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        default:
            return LogError("unknown token when expecting an expression");
    }
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E = ParseExpression()) {
        // Make an anonymous proto.
        auto Proto = llvm::make_unique<PrototypeAST>("", std::vector<std::string>());
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (ParseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurrentToken != tokenIdentifier) return LogErrorP("Expected function name in prototype");

    std::string functionName = identifierStr;
    getNextToken();

    if (CurrentToken != '(') return LogErrorP("Expected '(' in prototype");

    std::vector<std::string> argNames;
    while (getNextToken() == tokenIdentifier) 
        argNames.push_back(identifierStr);
    if (CurrentToken != ')')
        return LogErrorP("Expected ')' in prototype");

    // success.
    getNextToken(); // eat ')'.

    return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat def.
    auto Proto = ParsePrototype();
    if (!Proto) return nullptr;

    if (auto E = ParseExpression())
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    return nullptr;
}

void HandleDefinition() {
    if (ParseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurrentToken) {
            case TokenEOF:
                return;
            case ';': // ignore top-level semicolons.
                GetNextToken();
                break;
            case TokenDef:
                HandleDefinition();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}


