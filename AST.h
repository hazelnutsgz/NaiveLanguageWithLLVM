class ExprAST {
public:
    virtual ~ExprAST() {}
};

class NumberExprAST : public ExprAST {
    double val;

public:
    NumberExprAST(double val) : val(val) {}
};

class VariableExprAST : ExprAST {
    std::string name;

public:
    VariableExprAST(const std::string& name) {} 
};

class BinaryExprAST : public ExprAST {
    char op;
    std::unique_ptr<ExprAST> lhs;
    std::unique_ptr<ExprAST> rhs;
public:
    BinaryExprAST(char op,
                  std::unique_ptr<ExprAST> lhs,
                  std::unique_ptr<ExprAST> rhs
    ) : op(op), LHS(std::move(lhs)), RHS(std::move(rhs)) {}
}

class CallExprAST : public ExprAST {
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(const std::string &callee,
                std::vector<std::unique_ptr<ExprAST>> args)
    : callee(callee), args(std::move(args)) {}
};

class PrototypeAST {
    std::string name;
    std::vector<std::string> args;

public:
    PrototypeAST(const std::string &name, 
            std::vector<std::string> args)
    : name(name), args(std::move(args)) {}

    const std::string &getName() const { return name; }
};

class FunctionAST {
    std::unique_ptr<PrototypeAST> proto;
    std::unique_ptr<ExprAST> body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> proto,
                std::unique_ptr<ExprAST> body)
    : proto(std::move(proto)), body(std::move(body)) {}
};