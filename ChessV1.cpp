#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "logic.h"
using namespace std;
const int BASE_WINDOW_SIZE = 800;
struct TextureManager {
    unordered_map<string, SDL_Texture*> textures;
    bool load(SDL_Renderer* renderer, const string& name, const string& path) {
        SDL_Surface* surface = IMG_Load(path.c_str());
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        if (!tex) {
            cerr << "Failed to create texture: " << path << " -> " << SDL_GetError()
                << endl;
            return false;
        }
        textures[name] = tex;
        return true;
    }
    void cleanup() {
        for (auto& p : textures)
            SDL_DestroyTexture(p.second);
        textures.clear();
    }
};
string pieceKeyFor(const Piece& p) {
    if (p.isEmpty())
        return "";
    string key = (p.color == WHITE) ? "white-" : "black-";
    switch (toupper(p.type)) {
    case 'P':
        key += "pawn";
        break;
    case 'R':
        key += "rook";
        break;
    case 'N':
        key += "knight";
        break;
    case 'B':
        key += "bishop";
        break;
    case 'Q':
        key += "queen";
        break;
    case 'K':
        key += "king";
        break;
    default:
        return "";
    }
    return key;
}
int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        cerr << "SDL init failed: " << SDL_GetError() << endl;
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("ChessV1", BASE_WINDOW_SIZE,
        BASE_WINDOW_SIZE, SDL_WINDOW_RESIZABLE);
    if (!window) {
        cerr << "Window creation failed: " << SDL_GetError() << endl;
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        cerr << "Renderer creation failed: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    TextureManager tm;
    vector<string> pieceNames = { "white-pawn",   "white-rook",  "white-knight",
                                 "white-bishop", "white-queen", "white-king",
                                 "black-pawn",   "black-rook",  "black-knight",
                                 "black-bishop", "black-queen", "black-king" };
    for (auto& name : pieceNames) {
        if (!tm.load(renderer, name, "assets/" + name + ".png")) {
            cerr << "Warning: failed to load texture '" << name << "'; continuing.\n";
        }
    }
    Board chess;
    bool running = true;
    bool dragging = false;
    pii drag_from = { -1, -1 };
    float drag_offset_x = 0.0f;
    float drag_offset_y = 0.0f;
    float mouse_x = 0.0f, mouse_y = 0.0f;
    while (running) {
        SDL_Event e;
        int win_w, win_h;
        SDL_GetWindowSize(window, &win_w, &win_h);
        float tile_w = win_w / 8.0f;
        float tile_h = win_h / 8.0f;
        auto screenToBoard = [&](float sx, float sy) -> pii {
            int col = static_cast<int>(sx / tile_w);
            int row = static_cast<int>(sy / tile_h);
            return { 7 - row, col };
            };
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                e.button.button == SDL_BUTTON_LEFT) {
                SDL_GetMouseState(&mouse_x, &mouse_y);
                pii b = screenToBoard(mouse_x, mouse_y);
                int my = b.first;
                int mx = b.second;
                if (!chess.inBounds(my, mx))
                    continue;
                Piece p = chess.board[my][mx];
                if (!p.isEmpty() && p.color == chess.turn) {
                    dragging = true;
                    drag_from = { my, mx };
                    float piece_px = mx * tile_w;
                    float piece_py = (7 - my) * tile_h;
                    drag_offset_x = mouse_x - piece_px;
                    drag_offset_y = mouse_y - piece_py;
                    drag_offset_x = clamp(drag_offset_x, 0.0f, tile_w / 2.0f);
                    drag_offset_y = clamp(drag_offset_y, 0.0f, tile_h / 2.0f);
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_MOTION) {
                SDL_GetMouseState(&mouse_x, &mouse_y);
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP &&
                e.button.button == SDL_BUTTON_LEFT && dragging) {
                SDL_GetMouseState(&mouse_x, &mouse_y);
                pii b = screenToBoard(mouse_x, mouse_y);
                int my = b.first;
                int mx = b.second;
                if (chess.inBounds(my, mx)) {
                    chess.makeMove(drag_from, { my, mx });
                }
                dragging = false;
                drag_from = { -1, -1 };
                drag_offset_x = drag_offset_y = 0.0f;
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                bool light = (i + j) % 2 == 0;
                SDL_SetRenderDrawColor(renderer, light ? 222 : 118, light ? 184 : 84,
                    light ? 135 : 57, 255);
                float drawY = (7 - i) * tile_h;
                SDL_FRect rect = { j * tile_w, drawY, tile_w, tile_h };
                SDL_RenderFillRect(renderer, &rect);
                if (dragging && drag_from.first == i && drag_from.second == j)
                    continue;
                Piece p = chess.board[i][j];
                if (!p.isEmpty()) {
                    string key = pieceKeyFor(p);
                    if (!key.empty() && tm.textures.count(key)) {
                        SDL_FRect dst = { j * tile_w, drawY, tile_w, tile_h };
                        SDL_RenderTexture(renderer, tm.textures[key], nullptr, &dst);
                    }
                }
            }
        }
        if (dragging && drag_from.first != -1) {
            Piece p = chess.board[drag_from.first][drag_from.second];
            string key = pieceKeyFor(p);
            if (!key.empty() && tm.textures.count(key)) {
                float px = mouse_x - drag_offset_x;
                float py = mouse_y - drag_offset_y;
                px = clamp(px, 0.0f, static_cast<float>(win_w) - tile_w);
                py = clamp(py, 0.0f, static_cast<float>(win_h) - tile_h);
                SDL_FRect dst = { px, py, tile_w, tile_h };
                SDL_RenderTexture(renderer, tm.textures[key], nullptr, &dst);
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    tm.cleanup();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
