#include "lexer.h"

#include <cctype>
#include <unordered_set>
#include <utility>

// ============================================================
// Keyword check
// ============================================================

bool Lexer::isKeyword(const std::string& str)
{
    static const std::unordered_set<std::string> keywords = {
        "int",
        "return",
        "if",
        "else",
        "while",
        "for",
        "char",
        "string",
        "float",
        "double",
        "void"
    };

    return keywords.find(str) != keywords.end();
}

// ============================================================
// Constructor
// ============================================================

Lexer::Lexer(std::string input)
    : src(std::move(input)),
      pos(0),
      line(1)
{
}

// ============================================================
// End
// ============================================================

bool Lexer::isAtEnd() const
{
    return pos >= src.size();
}

// ============================================================
// Peek
// ============================================================

char Lexer::peek() const
{
    if (isAtEnd())
        return '\0';

    return src[pos];
}

// ============================================================
// Peek next
// ============================================================

char Lexer::peekNext() const
{
    if (pos + 1 >= src.size())
        return '\0';

    return src[pos + 1];
}

// ============================================================
// Advance
// ============================================================

char Lexer::advance()
{
    if (isAtEnd())
        return '\0';

    char ch = src[pos++];

    if (ch == '\n')
        ++line;

    return ch;
}

// ============================================================
// Whitespace
// ============================================================

void Lexer::skipWhiteSpace()
{
    while (!isAtEnd() && isspace(static_cast<unsigned char>(peek())))
    {
        advance();
    }
}

// ============================================================
// Comments
// ============================================================

void Lexer::skipComments()
{
    bool foundComment = true;

    while (foundComment && !isAtEnd())
    {
        foundComment = false;

        // Single-line comment: //
        if (peek() == '/' && peekNext() == '/')
        {
            foundComment = true;
            advance();
            advance();

            while (!isAtEnd() && peek() != '\n')
            {
                advance();
            }
        }
        // Multi-line comment: /* ... */
        else if (peek() == '/' && peekNext() == '*')
        {
            foundComment = true;
            advance();
            advance();

            while (!isAtEnd() && !(peek() == '*' && peekNext() == '/'))
            {
                advance();
            }

            if (!isAtEnd())
            {
                advance(); // consume '*'
                advance(); // consume '/'
            }
        }

        skipWhiteSpace();
    }
}

// ============================================================
// Identifier / keyword
// ============================================================

Token Lexer::identifier()
{
    std::string value;
    int startLine = line;

    while (!isAtEnd() && (isalnum(static_cast<unsigned char>(peek())) || peek() == '_'))
    {
        value.push_back(advance());
    }

    if (isKeyword(value))
    {
        return {TokenType::KEYWORD, value, startLine};
    }

    return {TokenType::IDENTIFIER, value, startLine};
}

// ============================================================
// Number
// ============================================================

Token Lexer::number()
{
    std::string value;
    int startLine = line;

    // Integer portion
    while (!isAtEnd() && isdigit(static_cast<unsigned char>(peek())))
    {
        value.push_back(advance());
    }

    // Decimal portion
    if (peek() == '.' && isdigit(static_cast<unsigned char>(peekNext())))
    {
        value.push_back(advance());
        while (!isAtEnd() && isdigit(static_cast<unsigned char>(peek())))
        {
            value.push_back(advance());
        }
    }

    // Exponent
    if (peek() == 'e' || peek() == 'E')
    {
        value.push_back(advance());

        if (peek() == '+' || peek() == '-')
        {
            value.push_back(advance());
        }

        while (!isAtEnd() && isdigit(static_cast<unsigned char>(peek())))
        {
            value.push_back(advance());
        }
    }

    return {TokenType::NUMBER, value, startLine};
}

// ============================================================
// String literal
// ============================================================

Token Lexer::stringLiteral()
{
    std::string value;
    int startLine = line;

    // Consume opening quote
    advance();

    while (!isAtEnd() && peek() != '"')
    {
        if (peek() == '\\')
        {
            advance();
            if (!isAtEnd())
            {
                char esc = advance();
                switch (esc)
                {
                    case 'n':  value.push_back('\n'); break;
                    case 't':  value.push_back('\t'); break;
                    case 'r':  value.push_back('\r'); break;
                    case '\\': value.push_back('\\'); break;
                    case '"':  value.push_back('"');  break;
                    case '0':  value.push_back('\0'); break;
                    default:   value.push_back(esc);  break;
                }
            }
        }
        else
        {
            value.push_back(advance());
        }
    }

    // Consume closing quote
    if (peek() == '"')
    {
        advance();
    }

    return {TokenType::STRING, value, startLine};
}

// ============================================================
// Next token
// ============================================================

Token Lexer::nextToken()
{
    skipWhiteSpace();
    skipComments();
    skipWhiteSpace();

    if (isAtEnd())
    {
        return {TokenType::UNKNOWN, "", line};
    }

    char ch = peek();

    // Identifier or keyword
    if (isalpha(static_cast<unsigned char>(ch)) || ch == '_')
    {
        return identifier();
    }

    // Number
    if (isdigit(static_cast<unsigned char>(ch)))
    {
        return number();
    }

    // String
    if (ch == '"')
    {
        return stringLiteral();
    }

    // Check two-character operators
    char next = peekNext();
    if (next != '\0')
    {
        std::string twoChar;
        twoChar.push_back(ch);
        twoChar.push_back(next);

        if (twoChar == "==" || twoChar == "!=" || twoChar == "<=" || twoChar == ">=" ||
            twoChar == "&&" || twoChar == "||" || twoChar == "++" || twoChar == "--" ||
            twoChar == "+=" || twoChar == "-=" || twoChar == "*=" || twoChar == "/=" ||
            twoChar == "%=" || twoChar == "<<" || twoChar == ">>" || twoChar == "&=" ||
            twoChar == "|=" || twoChar == "^=")
        {
            advance();
            advance();
            return {TokenType::OPERATOR, twoChar, line};
        }
    }

    // Single-character operators
    if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' ||
        ch == '=' || ch == '<' || ch == '>' || ch == '!' || ch == '&' ||
        ch == '|' || ch == '^' || ch == '~')
    {
        advance();
        return {TokenType::OPERATOR, std::string(1, ch), line};
    }

    // Delimiters
    if (ch == ';' || ch == ',' || ch == '(' || ch == ')' ||
        ch == '{' || ch == '}' || ch == '[' || ch == ']')
    {
        advance();
        return {TokenType::DELIMITER, std::string(1, ch), line};
    }

    // Unknown token
    advance();
    return {TokenType::UNKNOWN, std::string(1, ch), line};
}

// ============================================================
// Tokenize
// ============================================================

std::vector<Token> Lexer::tokenize()
{
    std::vector<Token> tokens;

    while (!isAtEnd())
    {
        Token token = nextToken();

        if (token.type != TokenType::UNKNOWN || !token.value.empty())
        {
            tokens.push_back(token);
        }
    }

    return tokens;
}

// ============================================================
// Token type string
// ============================================================

std::string tokenTypeToStr(TokenType t)
{
    switch (t)
    {
        case TokenType::KEYWORD:    return "KEYWORD";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER:     return "NUMBER";
        case TokenType::OPERATOR:   return "OPERATOR";
        case TokenType::DELIMITER:  return "DELIMITER";
        case TokenType::STRING:     return "STRING";
        default:                    return "UNKNOWN";
    }
}