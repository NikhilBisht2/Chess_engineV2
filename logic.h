#pragma once
#include <string>
#include <utility>
#include <vector>

using namespace std;
using pii = pair<int, int>;

enum Color { NONE, WHITE, BLACK };

struct Piece {
    char type;
    Color color;
    Piece(char t = '.', Color c = Color::NONE);
    bool isEmpty() const;
    char displayChar() const;
};

class Board {
public:
    static const int BOARD_SIZE = 8;
    vector<vector<Piece>> board;
    Color turn;
    bool whiteKingMoved;
    bool blackKingMoved;
    bool whiteKRookMoved;
    bool whiteQRookMoved;
    bool blackKRookMoved;
    bool blackQRookMoved;
    pii lastDoublePawnMove;
    int halfmoveClock;
    vector<string> positionHistory;

    Board();
    void initBoard();
    void display() const;
    bool inBounds(int x, int y) const;
    bool isSquareAttacked(const pii& sq, Color byColor) const;
    vector<pii> getMoves(const pii& pos) const;
    vector<pii> legalMoves(const pii& pos);
    pii findKing(Color c) const;
    bool isInCheck(Color c);
    bool isCheckmate(Color c);
    bool isStalemate(Color c);
    string positionKey() const;
    bool isThreefoldRepetition() const;
    bool isFiftyMoveRule() const;
    bool makeMove(pii from, pii to);
private:
    void addSlidingMoves(int x, int y, const vector<pii>& dirs, vector<pii>& out) const;
};
