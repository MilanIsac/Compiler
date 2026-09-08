#include "parser.h"

#include <iostream>
#include <utility>

// ============================================================
// Constructor
// ============================================================

Parser::Parser(std::vector<Token> tokens)
    : tokens(std::move(tokens)),
      pos(0)
{
}

// ============================================================
// Helpers
// ============================================================

Token Parser::peek() const
{
    if (pos >= tokens.size())
    {
        return {TokenType::UNKNOWN, "", -1};
    }
    return tokens[pos];
}

Token Parser::peekNext() const
{
    if (pos + 1 >= tokens.size())
    {
        return {TokenType::UNKNOWN, "", -1};
    }
    return tokens[pos + 1];
}

Token Parser::advance()
{
    if (pos >= tokens.size())
    {
        return {TokenType::UNKNOWN, "", -1};
    }
    return tokens[pos++];
}

bool Parser::check(const std::string& value) const
{
    if (pos >= tokens.size())
        return false;

    return tokens[pos].value == value;
}

bool Parser::checkType(TokenType type) const
{
    if (pos >= tokens.size())
        return false;

    return tokens[pos].type == type;
}

bool Parser::match(const std::string& value)
{
    if (check(value))
    {
        advance();
        return true;
    }
    return false;
}

bool Parser::matchType(TokenType type, Token& outToken)
{
    if (checkType(type))
    {
        outToken = advance();
        return true;
    }
    return false;
}

// ============================================================
// Primary expression
// primary: NUMBER | STRING | IDENTIFIER | '(' expression ')'
// ============================================================

ASTNode* Parser::primary()
{
    Token t = peek();

    // ============================================================
    // NUMBER
    // ============================================================

    if (t.type == TokenType::NUMBER)
    {
        advance();
        return new ASTNode(NodeType::NUMBER, t.value);
    }

    // ============================================================
    // STRING
    // ============================================================

    if (t.type == TokenType::STRING)
    {
        advance();
        return new ASTNode(NodeType::STRING_LITERAL, t.value);
    }

    // ============================================================
    // IDENTIFIER or FUNCTION CALL
    // ============================================================

    if (t.type == TokenType::IDENTIFIER)
    {
        advance();

        std::string name = t.value;

        // --------------------------------------------------------
        // Function call
        //
        // example:
        // add(10, 5)
        // --------------------------------------------------------

        if (match("("))
        {
            ASTNode* call = new ASTNode(NodeType::CALL, name);

            // Arguments
            if (!check(")"))
            {
                while (true)
                {
                    ASTNode* argument = expression();

                    if (!argument)
                    {
                        freeAST(call);
                        return nullptr;
                    }

                    call->children.push_back(argument);

                    if (match(","))
                    {
                        continue;
                    }

                    break;
                }
            }

            if (!match(")"))
            {
                std::cerr
                    << "Parser error [line "
                    << t.line
                    << "]: expected ')' after function arguments\n";

                freeAST(call);
                return nullptr;
            }

            return call;
        }

        // --------------------------------------------------------
        // Postfix ++
        // --------------------------------------------------------

        if (match("++"))
        {
            ASTNode* binOp =
                new ASTNode(NodeType::BINARY_OP, "+");

            binOp->left =
                new ASTNode(NodeType::IDENTIFIER, name);

            binOp->right =
                new ASTNode(NodeType::NUMBER, "1");

            ASTNode* assign =
                new ASTNode(NodeType::ASSIGN, "=");

            assign->left =
                new ASTNode(NodeType::IDENTIFIER, name);

            assign->right = binOp;

            return assign;
        }

        // --------------------------------------------------------
        // Postfix --
        // --------------------------------------------------------

        if (match("--"))
        {
            ASTNode* binOp =
                new ASTNode(NodeType::BINARY_OP, "-");

            binOp->left =
                new ASTNode(NodeType::IDENTIFIER, name);

            binOp->right =
                new ASTNode(NodeType::NUMBER, "1");

            ASTNode* assign =
                new ASTNode(NodeType::ASSIGN, "=");

            assign->left =
                new ASTNode(NodeType::IDENTIFIER, name);

            assign->right = binOp;

            return assign;
        }

        return new ASTNode(NodeType::IDENTIFIER, name);
    }

    // ============================================================
    // Parenthesized expression
    // ============================================================

    if (match("("))
    {
        ASTNode* node = expression();

        if (!match(")"))
        {
            std::cerr
                << "Parser error [line "
                << t.line
                << "]: expected ')' after expression\n";
        }

        return node;
    }

    std::cerr
        << "Parser error [line "
        << t.line
        << "]: unexpected token in primary: '"
        << t.value
        << "'\n";

    advance();

    return nullptr;
}

// ============================================================
// Unary expression
// unary: ('+' | '-' | '!' | '~' | '++' | '--') unary | primary
// ============================================================

ASTNode* Parser::unary()
{
    Token t = peek();

    if (match("+"))
    {
        return unary();
    }

    if (match("-"))
    {
        ASTNode* right = unary();
        ASTNode* neg = new ASTNode(NodeType::BINARY_OP, "-");
        neg->left = new ASTNode(NodeType::NUMBER, "0");
        neg->right = right;
        return neg;
    }

    if (match("!"))
    {
        ASTNode* right = unary();
        ASTNode* node = new ASTNode(NodeType::LOGICAL_NOT, "!");
        node->left = right;
        return node;
    }

    if (match("++"))
    {
        ASTNode* right = unary();
        if (right && right->type == NodeType::IDENTIFIER)
        {
            ASTNode* binOp = new ASTNode(NodeType::BINARY_OP, "+");
            binOp->left = new ASTNode(NodeType::IDENTIFIER, right->value);
            binOp->right = new ASTNode(NodeType::NUMBER, "1");

            ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
            assign->left = right;
            assign->right = binOp;
            return assign;
        }
        return right;
    }

    if (match("--"))
    {
        ASTNode* right = unary();
        if (right && right->type == NodeType::IDENTIFIER)
        {
            ASTNode* binOp = new ASTNode(NodeType::BINARY_OP, "-");
            binOp->left = new ASTNode(NodeType::IDENTIFIER, right->value);
            binOp->right = new ASTNode(NodeType::NUMBER, "1");

            ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
            assign->left = right;
            assign->right = binOp;
            return assign;
        }
        return right;
    }

    return primary();
}

// ============================================================
// Multiplicative
// multiplicative: unary (('*' | '/' | '%') unary)*
// ============================================================

ASTNode* Parser::multiplicative()
{
    ASTNode* node = unary();

    while (check("*") || check("/") || check("%"))
    {
        std::string op = advance().value;
        ASTNode* right = unary();

        ASTNode* parent = new ASTNode(NodeType::BINARY_OP, op);
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Additive
// additive: multiplicative (('+' | '-') multiplicative)*
// ============================================================

ASTNode* Parser::additive()
{
    ASTNode* node = multiplicative();

    while (check("+") || check("-"))
    {
        std::string op = advance().value;
        ASTNode* right = multiplicative();

        ASTNode* parent = new ASTNode(NodeType::BINARY_OP, op);
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Shift
// shift: additive (('<<' | '>>') additive)*
// ============================================================

ASTNode* Parser::shift()
{
    ASTNode* node = additive();

    while (check("<<") || check(">>"))
    {
        std::string op = advance().value;
        ASTNode* right = additive();

        ASTNode* parent = new ASTNode(NodeType::BINARY_OP, op);
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Comparison
// comparison: shift (('<' | '<=' | '>' | '>=') shift)*
// ============================================================

ASTNode* Parser::comparison()
{
    ASTNode* node = shift();

    while (check("<") || check("<=") || check(">") || check(">="))
    {
        std::string op = advance().value;
        ASTNode* right = shift();

        ASTNode* parent = new ASTNode(NodeType::COMPARISON, op);
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Equality
// equality: comparison (('==' | '!=') comparison)*
// ============================================================

ASTNode* Parser::equality()
{
    ASTNode* node = comparison();

    while (check("==") || check("!="))
    {
        std::string op = advance().value;
        ASTNode* right = comparison();

        ASTNode* parent = new ASTNode(NodeType::COMPARISON, op);
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Bitwise AND
// bitwiseAnd: equality ('&' equality)*
// ============================================================

ASTNode* Parser::bitwiseAnd()
{
    ASTNode* node = equality();

    while (check("&"))
    {
        advance();
        ASTNode* right = equality();

        ASTNode* parent = new ASTNode(NodeType::BINARY_OP, "&");
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Bitwise XOR
// bitwiseXor: bitwiseAnd ('^' bitwiseAnd)*
// ============================================================

ASTNode* Parser::bitwiseXor()
{
    ASTNode* node = bitwiseAnd();

    while (check("^"))
    {
        advance();
        ASTNode* right = bitwiseAnd();

        ASTNode* parent = new ASTNode(NodeType::BINARY_OP, "^");
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Bitwise OR
// bitwiseOr: bitwiseXor ('|' bitwiseXor)*
// ============================================================

ASTNode* Parser::bitwiseOr()
{
    ASTNode* node = bitwiseXor();

    while (check("|"))
    {
        advance();
        ASTNode* right = bitwiseXor();

        ASTNode* parent = new ASTNode(NodeType::BINARY_OP, "|");
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Logical AND
// logicalAnd: bitwiseOr ('&&' bitwiseOr)*
// ============================================================

ASTNode* Parser::logicalAnd()
{
    ASTNode* node = bitwiseOr();

    while (check("&&"))
    {
        advance();
        ASTNode* right = bitwiseOr();

        ASTNode* parent = new ASTNode(NodeType::LOGICAL_AND, "&&");
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Logical OR
// logicalOr: logicalAnd ('||' logicalAnd)*
// ============================================================

ASTNode* Parser::logicalOr()
{
    ASTNode* node = logicalAnd();

    while (check("||"))
    {
        advance();
        ASTNode* right = logicalAnd();

        ASTNode* parent = new ASTNode(NodeType::LOGICAL_OR, "||");
        parent->left = node;
        parent->right = right;
        node = parent;
    }

    return node;
}

// ============================================================
// Expression
// ============================================================

ASTNode* Parser::expression()
{
    return logicalOr();
}

// ============================================================
// Variable declaration
// varDeclaration: type IDENTIFIER ('=' expression)? ';'
// ============================================================

ASTNode* Parser::varDeclaration()
{
    Token typeTok = advance(); // Consume type keyword (int, float, char, etc.)

    Token idTok = peek();
    if (idTok.type != TokenType::IDENTIFIER)
    {
        std::cerr << "Parser error [line " << idTok.line << "]: expected identifier after type '" << typeTok.value << "'\n";
        return nullptr;
    }
    advance();

    ASTNode* rhs = nullptr;
    if (match("="))
    {
        rhs = expression();
    }
    else
    {
        // Default initialize to 0
        rhs = new ASTNode(NodeType::NUMBER, "0");
    }

    match(";");

    ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
    assign->left = new ASTNode(NodeType::IDENTIFIER, idTok.value);
    assign->right = rhs;
    return assign;
}

// ============================================================
// Assignment / Expression statement
// ============================================================

ASTNode* Parser::assignmentOrExpr()
{
    Token id = peek();

    // Check if this is an assignment: id = expr, id += expr, etc.
    if (id.type == TokenType::IDENTIFIER)
    {
        Token next = peekNext();

        if (next.value == "=")
        {
            advance(); // id
            advance(); // =
            ASTNode* rhs = expression();
            match(";");

            ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
            assign->left = new ASTNode(NodeType::IDENTIFIER, id.value);
            assign->right = rhs;
            return assign;
        }

        // Compound assignment: +=, -=, *=, /=, %=
        if (next.value == "+=" || next.value == "-=" || next.value == "*=" ||
            next.value == "/=" || next.value == "%=")
        {
            advance(); // id
            std::string opStr = advance().value; // +=, etc.
            char mathOp = opStr[0];

            ASTNode* rhs = expression();
            match(";");

            ASTNode* binOp = new ASTNode(NodeType::BINARY_OP, std::string(1, mathOp));
            binOp->left = new ASTNode(NodeType::IDENTIFIER, id.value);
            binOp->right = rhs;

            ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
            assign->left = new ASTNode(NodeType::IDENTIFIER, id.value);
            assign->right = binOp;
            return assign;
        }

        // Post-increment / decrement: id++; id--;
        if (next.value == "++" || next.value == "--")
        {
            advance(); // id
            std::string incOp = advance().value;
            match(";");

            std::string mathOp = (incOp == "++") ? "+" : "-";
            ASTNode* binOp = new ASTNode(NodeType::BINARY_OP, mathOp);
            binOp->left = new ASTNode(NodeType::IDENTIFIER, id.value);
            binOp->right = new ASTNode(NodeType::NUMBER, "1");

            ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
            assign->left = new ASTNode(NodeType::IDENTIFIER, id.value);
            assign->right = binOp;
            return assign;
        }
    }

    // Otherwise, evaluate as expression
    ASTNode* expr = expression();
    match(";");
    return expr;
}

// ============================================================
// Return statement
// ============================================================

ASTNode* Parser::parseReturn()
{
    Token retTok = advance(); // Consume 'return'

    ASTNode* retVal = nullptr;
    if (!check(";") && !check("}"))
    {
        retVal = expression();
    }
    else
    {
        retVal = new ASTNode(NodeType::NUMBER, "0");
    }

    match(";");

    ASTNode* node = new ASTNode(NodeType::RETURN, "RETURN");
    node->left = retVal;
    return node;
}

// ============================================================
// Block
// ============================================================

ASTNode* Parser::parseBlock()
{
    if (!match("{"))
    {
        std::cerr << "Parser error: expected '{'\n";
        return nullptr;
    }

    ASTNode* block = new ASTNode(NodeType::BLOCK, "BLOCK");

    while (pos < tokens.size() && !check("}"))
    {
        ASTNode* node = statement();
        if (node)
        {
            block->children.push_back(node);
        }
    }

    if (!match("}"))
    {
        std::cerr << "Parser error: expected '}'\n";
    }

    return block;
}

// ============================================================
// If statement
// ============================================================

ASTNode* Parser::parseIf()
{
    advance(); // Consume 'if'

    if (!match("("))
    {
        std::cerr << "Parser error: expected '(' after if\n";
        return nullptr;
    }

    ASTNode* condition = expression();

    if (!match(")"))
    {
        std::cerr << "Parser error: expected ')' after if condition\n";
    }

    ASTNode* thenBranch = nullptr;
    if (check("{"))
    {
        thenBranch = parseBlock();
    }
    else
    {
        thenBranch = statement();
    }

    ASTNode* elseBranch = nullptr;
    if (check("else"))
    {
        advance(); // Consume 'else'

        if (check("{"))
        {
            elseBranch = parseBlock();
        }
        else
        {
            elseBranch = statement();
        }
    }

    ASTNode* node = new ASTNode(NodeType::IF, "IF");
    node->left = condition;
    node->right = thenBranch;
    node->third = elseBranch;
    return node;
}

// ============================================================
// While statement
// ============================================================

ASTNode* Parser::parseWhile()
{
    advance(); // Consume 'while'

    if (!match("("))
    {
        std::cerr << "Parser error: expected '(' after while\n";
        return nullptr;
    }

    ASTNode* condition = expression();

    if (!match(")"))
    {
        std::cerr << "Parser error: expected ')' after while condition\n";
    }

    ASTNode* body = nullptr;
    if (check("{"))
    {
        body = parseBlock();
    }
    else
    {
        body = statement();
    }

    ASTNode* node = new ASTNode(NodeType::WHILE, "WHILE");
    node->left = condition;
    node->right = body;
    return node;
}

// ============================================================
// For statement
// ============================================================

ASTNode* Parser::parseFor()
{
    advance(); // Consume 'for'

    if (!match("("))
    {
        std::cerr << "Parser error: expected '(' after for\n";
        return nullptr;
    }

    // Init
    ASTNode* init = nullptr;
    if (!check(";"))
    {
        Token t = peek();
        if (t.type == TokenType::KEYWORD && (t.value == "int" || t.value == "float" ||
            t.value == "char" || t.value == "double" || t.value == "string" || t.value == "void"))
        {
            init = varDeclaration(); // varDeclaration already consumes the ';'
        }
        else
        {
            init = assignmentOrExpr();
            match(";");
        }
    }
    else
    {
        match(";");
    }

    // Condition
    ASTNode* cond = nullptr;
    if (!check(";"))
    {
        cond = expression();
    }
    else
    {
        cond = new ASTNode(NodeType::NUMBER, "1");
    }
    match(";");

    // Update
    ASTNode* update = nullptr;
    if (!check(")"))
    {
        Token id = peek();
        Token next = peekNext();
        if (id.type == TokenType::IDENTIFIER &&
            (next.value == "=" || next.value == "+=" || next.value == "-=" ||
             next.value == "*=" || next.value == "/=" || next.value == "%="))
        {
            advance(); // id
            std::string opStr = advance().value;
            ASTNode* rhs = expression();
            if (opStr == "=")
            {
                ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
                assign->left = new ASTNode(NodeType::IDENTIFIER, id.value);
                assign->right = rhs;
                update = assign;
            }
            else
            {
                ASTNode* binOp = new ASTNode(NodeType::BINARY_OP, std::string(1, opStr[0]));
                binOp->left = new ASTNode(NodeType::IDENTIFIER, id.value);
                binOp->right = rhs;
                ASTNode* assign = new ASTNode(NodeType::ASSIGN, "=");
                assign->left = new ASTNode(NodeType::IDENTIFIER, id.value);
                assign->right = binOp;
                update = assign;
            }
        }
        else
        {
            update = expression();
        }
    }
    if (!match(")"))
    {
        std::cerr << "Parser error: expected ')' in for loop\n";
    }

    // Body
    ASTNode* body = nullptr;
    if (check("{"))
    {
        body = parseBlock();
    }
    else
    {
        body = statement();
    }

    ASTNode* node = new ASTNode(NodeType::FOR, "FOR");
    node->left = init;
    node->right = cond;
    node->third = update;
    node->fourth = body;
    return node;
}

// ============================================================
// Statement
// ============================================================

ASTNode* Parser::statement()
{
    while (match(";"))
    {
    }

    Token t = peek();

    if (t.type == TokenType::UNKNOWN && t.value.empty())
    {
        return nullptr;
    }

    if (t.type == TokenType::KEYWORD)
    {
        if (t.value == "if")
        {
            return parseIf();
        }
        if (t.value == "while")
        {
            return parseWhile();
        }
        if (t.value == "for")
        {
            return parseFor();
        }
        if (t.value == "return")
        {
            return parseReturn();
        }
        if (t.value == "else")
        {
            std::cerr << "Parser error [line " << t.line << "]: unexpected 'else'\n";
            advance();
            return nullptr;
        }
                if (isTypeKeyword(t))
        {
            // If:
            //
            // int name (
            //
            // then this is a function definition.

            if (peekNext().type == TokenType::IDENTIFIER)
            {
                size_t savedPos = pos;

                advance(); // type
                advance(); // identifier

                bool isFunction = check("(");

                pos = savedPos;

                if (isFunction)
                {
                    return parseFunction();
                }
            }

            return varDeclaration();
        }
    }

    if (check("{"))
    {
        return parseBlock();
    }

    if (t.type == TokenType::IDENTIFIER || t.type == TokenType::NUMBER ||
        t.type == TokenType::STRING || check("(") || check("!") || check("-") || check("+"))
    {
        return assignmentOrExpr();
    }

    std::cerr << "Parser error [line " << t.line << "]: unexpected token '" << t.value << "'\n";
    advance();
    return nullptr;
}

// ============================================================
// Check whether token is a type keyword
// ============================================================

bool Parser::isTypeKeyword(const Token& token) const
{
    if (token.type != TokenType::KEYWORD)
        return false;

    return token.value == "int" ||
           token.value == "float" ||
           token.value == "double" ||
           token.value == "char" ||
           token.value == "string" ||
           token.value == "void";
}


// ============================================================
// Function definition
//
// function:
//     type IDENTIFIER '(' parameters ')' block
//
// Example:
//
// int add(int a, int b) {
//     return a + b
// }
// ============================================================

ASTNode* Parser::parseFunction()
{
    Token returnType = advance();

    Token name = peek();

    if (name.type != TokenType::IDENTIFIER)
    {
        std::cerr
            << "Parser error [line "
            << name.line
            << "]: expected function name after return type\n";

        return nullptr;
    }

    advance();

    if (!match("("))
    {
        std::cerr
            << "Parser error [line "
            << name.line
            << "]: expected '(' after function name\n";

        return nullptr;
    }

    ASTNode* function =
        new ASTNode(NodeType::FUNCTION, name.value);

    // Store return type in fourth field.
    function->fourth =
        new ASTNode(NodeType::IDENTIFIER, returnType.value);

    // ========================================================
    // Parameters
    // ========================================================

    if (!check(")"))
    {
        while (true)
        {
            Token paramType = peek();

            if (!isTypeKeyword(paramType))
            {
                std::cerr
                    << "Parser error [line "
                    << paramType.line
                    << "]: expected parameter type\n";

                freeAST(function);
                return nullptr;
            }

            advance();

            Token paramName = peek();

            if (paramName.type != TokenType::IDENTIFIER)
            {
                std::cerr
                    << "Parser error [line "
                    << paramName.line
                    << "]: expected parameter name\n";

                freeAST(function);
                return nullptr;
            }

            advance();

            ASTNode* parameter =
                new ASTNode(NodeType::IDENTIFIER, paramName.value);

            // Parameter type is stored in the parameter's
            // fourth field.
            parameter->fourth =
                new ASTNode(NodeType::IDENTIFIER, paramType.value);

            function->children.push_back(parameter);

            if (match(","))
            {
                continue;
            }

            break;
        }
    }

    if (!match(")"))
    {
        std::cerr
            << "Parser error [line "
            << name.line
            << "]: expected ')' after parameters\n";

        freeAST(function);
        return nullptr;
    }

    // ========================================================
    // Function body
    // ========================================================

    if (!check("{"))
    {
        std::cerr
            << "Parser error [line "
            << name.line
            << "]: expected '{' before function body\n";

        freeAST(function);
        return nullptr;
    }

    function->left = parseBlock();

    if (!function->left)
    {
        freeAST(function);
        return nullptr;
    }

    return function;
}

// ============================================================
// Parse Program
// ============================================================

std::vector<ASTNode*> Parser::parseProgram()
{
    std::vector<ASTNode*> statements;

    while (pos < tokens.size())
    {
        ASTNode* node = statement();

        if (node)
        {
            statements.push_back(node);
        }
        else
        {
            if (pos < tokens.size())
            {
                advance();
            }
        }
    }

    return statements;
}