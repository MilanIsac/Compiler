#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

enum class TokenType
{
    KEYWORD,
    IDENTIFIER,
    NUMBER,
    OPERATOR,
    DELIMITER,
    STRING,
    UNKNOWN
};

struct Token
{
    TokenType type;
    std::string value;
    int line;
};

class Lexer
{
private:
    std::string src;
    size_t pos;
    int line;

    static bool isKeyword(const std::string& str);

public:
    explicit Lexer(std::string input);

    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();

    void skipWhiteSpace();
    void skipComments();

    Token identifier();
    Token number();
    Token stringLiteral();
    Token nextToken();

    std::vector<Token> tokenize();
};

std::string tokenTypeToStr(TokenType t);

#endif