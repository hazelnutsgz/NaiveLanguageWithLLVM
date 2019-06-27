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
    TokenExtern = -3,

    // primary
    TokenIdentifier = -4,
    TokenNumber = -5
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
        if (identifierStr == "extern") return TokenExtern;
    
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

std::unique_ptr<ExprAST> ParseExpression();

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
            case TokenExtern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}


