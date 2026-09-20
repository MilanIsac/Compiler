#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_set>

enum class TokenType {
    KEYWORD, IDENTIFIER, NUMBER, OPERATOR, DELIMITER, UNKNOWN, STRING
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
    std::string src;
    size_t pos;
    int line;
    std::unordered_set<std::string> keywords;
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
    bool isKeyword(const std::string& str);
};

std::string tokenTypeToStr(TokenType t);

#endif
