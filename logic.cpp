#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include "logic.h"
using namespace std;
Piece::Piece(char t, Color c) : type(t), color(c) {}
bool Piece::isEmpty() const { return type == '.'; }
char Piece::displayChar() const {
    if (isEmpty()) return '.';
    return (color == WHITE) ? type : (char)tolower(type);
}
/* ---------------- Board implementation ---------------- */
Board::Board()
    : board(BOARD_SIZE, vector<Piece>(BOARD_SIZE)),
    turn(Color::WHITE),
    whiteKingMoved(false), blackKingMoved(false),
    whiteKRookMoved(false), whiteQRookMoved(false),
    blackKRookMoved(false), blackQRookMoved(false),
    lastDoublePawnMove({ -1, -1 }), halfmoveClock(0)
{
    initBoard();
}
void Board::initBoard() {
    string pieces = "RNBQKBNR";
    for (int i = 0; i < BOARD_SIZE; ++i) {
        board[6][i] = Piece('P', WHITE);
        board[7][i] = Piece(pieces[i], WHITE);
        board[1][i] = Piece('P', BLACK);
        board[0][i] = Piece(pieces[i], BLACK);
    }
    for (int r = 2; r <= 5; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            board[r][c] = Piece();
    whiteKingMoved = blackKingMoved = false;
    whiteKRookMoved = whiteQRookMoved = false;
    blackKRookMoved = blackQRookMoved = false;
    lastDoublePawnMove = { -1, -1 };
    halfmoveClock = 0;
    positionHistory.clear();
    positionHistory.push_back(positionKey());
    turn = Color::WHITE;
}
void Board::display() const {
    cout << "\n";
    for (int r = BOARD_SIZE - 1; r >= 0; --r) {
        cout << r + 1 << " ";
        for (int c = 0; c < BOARD_SIZE; ++c)
            cout << board[r][c].displayChar() << ' ';
        cout << '\n';
    }
    cout << "  a b c d e f g h\n";
}
bool Board::inBounds(int x, int y) const {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}
bool Board::isSquareAttacked(const pii& sq, Color byColor) const {
    static const vector<pii> bishopDirs = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
    static const vector<pii> rookDirs = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            const Piece& p = board[i][j];
            if (p.isEmpty() || p.color != byColor) continue;
            if (p.type == 'P') {
                int dir = (byColor == WHITE) ? -1 : 1;
                int ax = i + dir;
                if (inBounds(ax, j + 1) && make_pair(ax, j + 1) == sq) return true;
                if (inBounds(ax, j - 1) && make_pair(ax, j - 1) == sq) return true;
            }
            else if (p.type == 'N') {
                static const int dx[] = { 1, 2, 2, 1, -1, -2, -2, -1 };
                static const int dy[] = { 2, 1, -1, -2, -2, -1, 1, 2 };
                for (int k = 0; k < 8; ++k) {
                    int nx = i + dx[k], ny = j + dy[k];
                    if (inBounds(nx, ny) && make_pair(nx, ny) == sq) return true;
                }
            }
            else if (p.type == 'B' || p.type == 'R' || p.type == 'Q') {
                if (p.type == 'B' || p.type == 'Q') {
                    for (auto d : bishopDirs) {
                        int nx = i + d.first, ny = j + d.second;
                        while (inBounds(nx, ny)) {
                            if (make_pair(nx, ny) == sq) return true;
                            if (!board[nx][ny].isEmpty()) break;
                            nx += d.first; ny += d.second;
                        }
                    }
                }
                if (p.type == 'R' || p.type == 'Q') {
                    for (auto d : rookDirs) {
                        int nx = i + d.first, ny = j + d.second;
                        while (inBounds(nx, ny)) {
                            if (make_pair(nx, ny) == sq) return true;
                            if (!board[nx][ny].isEmpty()) break;
                            nx += d.first; ny += d.second;
                        }
                    }
                }
            }
            else if (p.type == 'K') {
                for (int dx = -1; dx <= 1; ++dx)
                    for (int dy = -1; dy <= 1; ++dy)
                        if (!(dx == 0 && dy == 0)) {
                            int nx = i + dx, ny = j + dy;
                            if (inBounds(nx, ny) && make_pair(nx, ny) == sq) return true;
                        }
            }
        }
    }
    return false;
}
void Board::addSlidingMoves(int x, int y, const vector<pii>& dirs, vector<pii>& out) const {
    const Piece& p = board[x][y];
    for (auto d : dirs) {
        int nx = x + d.first, ny = y + d.second;
        while (inBounds(nx, ny)) {
            if (board[nx][ny].isEmpty()) {
                out.push_back({ nx, ny });
            }
            else {
                if (board[nx][ny].color != p.color) out.push_back({ nx, ny });
                break;
            }
            nx += d.first; ny += d.second;
        }
    }
}
vector<pii> Board::getMoves(const pii& pos) const {
    vector<pii> moves;
    int x = pos.first, y = pos.second;
    if (!inBounds(x, y)) return moves;
    const Piece& p = board[x][y];
    if (p.isEmpty()) return moves;
    Color enemy = (p.color == Color::WHITE) ? Color::BLACK : Color::WHITE;
    if (p.type == 'P') {
        int dir = (p.color == WHITE) ? -1 : 1;
        int start_rank = (p.color == WHITE) ? 6 : 1;
        if (inBounds(x + dir, y) && board[x + dir][y].isEmpty()) {
            moves.push_back({ x + dir, y });
            if (x == start_rank && inBounds(x + 2 * dir, y) && board[x + 2 * dir][y].isEmpty())
                moves.push_back({ x + 2 * dir, y });
        }
        if (inBounds(x + dir, y + 1) && !board[x + dir][y + 1].isEmpty() && board[x + dir][y + 1].color == enemy)
            moves.push_back({ x + dir, y + 1 });
        if (inBounds(x + dir, y - 1) && !board[x + dir][y - 1].isEmpty() && board[x + dir][y - 1].color == enemy)
            moves.push_back({ x + dir, y - 1 });
        if (lastDoublePawnMove.first != -1) {
            int mx = lastDoublePawnMove.first, my = lastDoublePawnMove.second;
            if (mx == x && abs(my - y) == 1) {
                int captureX = x + dir;
                int captureY = my;
                if (inBounds(captureX, captureY))
                    moves.push_back({ captureX, captureY });
            }
        }
    }
    else if (p.type == 'N') {
        static const int dx[] = { 1, 2, 2, 1, -1, -2, -2, -1 };
        static const int dy[] = { 2, 1, -1, -2, -2, -1, 1, 2 };
        for (int i = 0; i < 8; ++i) {
            int nx = x + dx[i], ny = y + dy[i];
            if (inBounds(nx, ny) && (board[nx][ny].isEmpty() || board[nx][ny].color != p.color))
                moves.push_back({ nx, ny });
        }
    }
    else if (p.type == 'B' || p.type == 'R' || p.type == 'Q') {
        vector<pii> dirs;
        if (p.type == 'B' || p.type == 'Q') dirs.insert(dirs.end(), { {1,1}, {1,-1}, {-1,1}, {-1,-1} });
        if (p.type == 'R' || p.type == 'Q') dirs.insert(dirs.end(), { {1,0}, {-1,0}, {0,1}, {0,-1} });
        addSlidingMoves(x, y, dirs, moves);
    }
    else if (p.type == 'K') {
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx, ny = y + dy;
                if (inBounds(nx, ny) && (board[nx][ny].isEmpty() || board[nx][ny].color != p.color))
                    moves.push_back({ nx, ny });
            }
    }
    return moves;
}
vector<pii> Board::legalMoves(const pii& pos) {
    vector<pii> out;
    int x = pos.first, y = pos.second;
    if (!inBounds(x, y)) return out;
    const Piece& p = board[x][y];
    if (p.isEmpty()) return out;
    if (p.color != turn) return out;
    vector<pii> cand = getMoves(pos);
    for (auto to : cand) {
        Piece moving = board[x][y];
        Piece captured = board[to.first][to.second];
        pii savedLastDouble = lastDoublePawnMove;
        int savedHalfmove = halfmoveClock;
        bool performedEnPassant = false;
        if (moving.type == 'P' && captured.isEmpty() && to.second != y) {
            if (lastDoublePawnMove.first == x && lastDoublePawnMove.second == to.second) {
                performedEnPassant = true;
            }
        }
        board[to.first][to.second] = moving;
        board[x][y] = Piece();
        Piece capturedEP;
        if (performedEnPassant) {
            capturedEP = board[lastDoublePawnMove.first][lastDoublePawnMove.second];
            board[lastDoublePawnMove.first][lastDoublePawnMove.second] = Piece();
        }
        bool ok = true;
        if (isInCheck(turn)) ok = false;
        board[x][y] = moving;
        board[to.first][to.second] = captured;
        if (performedEnPassant) {
            board[lastDoublePawnMove.first][lastDoublePawnMove.second] = capturedEP;
        }
        lastDoublePawnMove = savedLastDouble;
        halfmoveClock = savedHalfmove;
        if (ok) out.push_back(to);
    }
    return out;
}
pii Board::findKing(Color c) const {
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            if (!board[i][j].isEmpty() && board[i][j].type == 'K' && board[i][j].color == c)
                return { i,j };
    return { -1,-1 };
}
bool Board::isInCheck(Color c) {
    pii kingPos = findKing(c);
    if (kingPos.first == -1) return false;
    Color enemy = (c == Color::WHITE) ? Color::BLACK : Color::WHITE;
    return isSquareAttacked(kingPos, enemy);
}
bool Board::isCheckmate(Color c) {
    if (!isInCheck(c)) return false;
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            if (!board[i][j].isEmpty() && board[i][j].color == c) {
                auto moves = legalMoves({ i,j });
                if (!moves.empty()) return false;
            }
    return true;
}
bool Board::isStalemate(Color c) {
    if (isInCheck(c)) return false;
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            if (!board[i][j].isEmpty() && board[i][j].color == c) {
                auto moves = legalMoves({ i,j });
                if (!moves.empty()) return false;
            }
    return true;
}
string Board::positionKey() const {
    string key;
    for (int i = BOARD_SIZE - 1; i >= 0; --i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            key.push_back(board[i][j].displayChar());
    key.push_back((turn == WHITE) ? 'w' : 'b');
    return key;
}
bool Board::isThreefoldRepetition() const {
    string key = positionKey();
    int count = 0;
    for (auto& k : positionHistory) if (k == key) ++count;
    return count >= 3;
}
bool Board::isFiftyMoveRule() const { return halfmoveClock >= 100; }
bool Board::makeMove(pii from, pii to) {
    if (!inBounds(from.first, from.second) || !inBounds(to.first, to.second)) return false;
    Piece p = board[from.first][from.second];
    if (p.isEmpty() || p.color != turn) return false;
    auto legal = legalMoves(from);
    if (find(legal.begin(), legal.end(), to) == legal.end()) return false;
    Piece target = board[to.first][to.second];
    bool performedEnPassant = false;
    if (p.type == 'P' && lastDoublePawnMove.first != -1) {
        int dir = (p.color == WHITE) ? -1 : 1;
        if (to.first == from.first + dir && to.second == lastDoublePawnMove.second && lastDoublePawnMove.first == from.first) {
            board[lastDoublePawnMove.first][lastDoublePawnMove.second] = Piece();
            performedEnPassant = true;
        }
    }
    board[to.first][to.second] = p;
    board[from.first][from.second] = Piece();
    if (p.type == 'P' && (to.first == 0 || to.first == BOARD_SIZE - 1)) {
        board[to.first][to.second] = Piece('Q', p.color);
    }
    if (p.type == 'P' || !target.isEmpty() || performedEnPassant)
        halfmoveClock = 0;
    else
        ++halfmoveClock;
    if (p.type == 'P' && abs(to.first - from.first) == 2) {
        lastDoublePawnMove = to;
    }
    else {
        lastDoublePawnMove = { -1, -1 };
    }
    turn = (turn == WHITE) ? BLACK : WHITE;
    positionHistory.push_back(positionKey());
    return true;
}
