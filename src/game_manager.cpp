#include "game_manager.h"
#include "chess_piece.h"
#include "chess_constants.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <cmath>
#include <algorithm>

using namespace godot;

GameManager *GameManager::singleton = nullptr;

// ─── Binding ──────────────────────────────────────────────────────────
void GameManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("handle_square_click", "col", "row"), &GameManager::handle_square_click);
    ClassDB::bind_method(D_METHOD("deselect"), &GameManager::deselect);

    ClassDB::bind_method(D_METHOD("is_white_turn"), &GameManager::is_white_turn);
    ClassDB::bind_method(D_METHOD("current_turn_color"), &GameManager::current_turn_color);
    ClassDB::bind_method(D_METHOD("is_game_over"), &GameManager::is_game_over);
    ClassDB::bind_method(D_METHOD("get_game_over_reason"), &GameManager::get_game_over_reason);
    ClassDB::bind_method(D_METHOD("is_in_check", "color"), &GameManager::is_in_check);

    ClassDB::bind_method(D_METHOD("promote_pending", "piece_type"), &GameManager::promote_pending);
    ClassDB::bind_method(D_METHOD("complete_promotion", "col", "row", "new_piece"), &GameManager::complete_promotion);

    ClassDB::bind_method(D_METHOD("can_undo"), &GameManager::can_undo);
    ClassDB::bind_method(D_METHOD("undo"), &GameManager::undo);

    ClassDB::bind_method(D_METHOD("get_captured_pieces"), &GameManager::get_captured_pieces);
    ClassDB::bind_method(D_METHOD("_on_piece_tree_exiting", "piece"), &GameManager::_on_piece_tree_exiting);
    ClassDB::bind_method(D_METHOD("set_highlights_enabled", "enabled"), &GameManager::set_highlights_enabled);
    ClassDB::bind_method(D_METHOD("get_highlights_enabled"), &GameManager::get_highlights_enabled);

    // ── Signals consumed by the GDScript UI layer ──
    ADD_SIGNAL(MethodInfo("turn_changed", PropertyInfo(Variant::STRING, "color")));
    ADD_SIGNAL(MethodInfo("piece_captured", PropertyInfo(Variant::STRING, "type"), PropertyInfo(Variant::STRING, "color")));
    ADD_SIGNAL(MethodInfo("capture_occurred", PropertyInfo(Variant::INT, "col"), PropertyInfo(Variant::INT, "row")));
    ADD_SIGNAL(MethodInfo("check_started", PropertyInfo(Variant::STRING, "color_in_check")));
    ADD_SIGNAL(MethodInfo("check_cleared"));
    ADD_SIGNAL(MethodInfo("game_over", PropertyInfo(Variant::STRING, "reason")));
    ADD_SIGNAL(MethodInfo("promotion_needed", PropertyInfo(Variant::INT, "col"), PropertyInfo(Variant::INT, "row"), PropertyInfo(Variant::STRING, "color")));
    ADD_SIGNAL(MethodInfo("move_made", PropertyInfo(Variant::STRING, "notation")));
    ADD_SIGNAL(MethodInfo("undo_performed"));
    ADD_SIGNAL(MethodInfo("selection_changed", PropertyInfo(Variant::INT, "col"), PropertyInfo(Variant::INT, "row"), PropertyInfo(Variant::BOOL, "has_selection")));
}

// ─── Construction ─────────────────────────────────────────────────────
GameManager::GameManager() {
    board.fill(nullptr);
    white_to_move      = true;
    white_king_moved   = false;
    black_king_moved   = false;
    white_rook_a_moved = false;
    white_rook_h_moved = false;
    black_rook_a_moved = false;
    black_rook_h_moved = false;
    ep_col = -1;
    ep_row = -1;
    game_over = false;
    selected_piece = nullptr;
    promotion_pending = false;
    promotion_col = promotion_row = -1;

    singleton = this;
}

GameManager::~GameManager() {
    if (singleton == this) singleton = nullptr;
}

GameManager *GameManager::get_singleton() { return singleton; }

void GameManager::_ready() {
    singleton = this; // re-affirm in case of multiple instantiation order quirks
}

// ─── Registration ─────────────────────────────────────────────────────
void GameManager::register_piece(ChessPiece *piece, int col, int row) {
    if (!in_bounds(col, row)) return;
    board[idx(col, row)] = piece;
    all_registered.push_back(piece);
    // Ensure GameManager never holds a dangling pointer if this piece node
    // is ever freed directly (e.g. during pawn-promotion node swap).
    // register_piece() runs exactly once per piece (from _ready()), so no
    // duplicate-connection guard is needed.
    piece->connect("tree_exiting", Callable(this, "_on_piece_tree_exiting").bind(piece));
}

void GameManager::place(ChessPiece *piece, int col, int row) {
    board[idx(col, row)] = piece;
}

// ─── Board queries ─────────────────────────────────────────────────────
bool GameManager::is_occupied(int col, int row) const {
    if (!in_bounds(col, row)) return false;
    return board[idx(col, row)] != nullptr;
}

ChessPiece *GameManager::piece_at(int col, int row) const {
    if (!in_bounds(col, row)) return nullptr;
    return board[idx(col, row)];
}

bool GameManager::is_enemy(int col, int row, const String &mover_color) const {
    ChessPiece *p = piece_at(col, row);
    if (!p) return false;
    return p->get_piece_color() != mover_color;
}

bool GameManager::is_friendly(int col, int row, const String &mover_color) const {
    ChessPiece *p = piece_at(col, row);
    if (!p) return false;
    return p->get_piece_color() == mover_color;
}

bool GameManager::path_clear(int from_col, int from_row, int to_col, int to_row) const {
    int dc = to_col - from_col;
    int dr = to_row - from_row;

    int step_c = (dc > 0) - (dc < 0); // -1, 0, or 1
    int step_r = (dr > 0) - (dr < 0);

    // Not a straight line or diagonal — caller shouldn't ask, but be safe.
    if (dc != 0 && dr != 0 && std::abs(dc) != std::abs(dr)) return true;

    int steps = std::max(std::abs(dc), std::abs(dr));
    for (int s = 1; s < steps; s++) {
        int c = from_col + step_c * s;
        int r = from_row + step_r * s;
        if (is_occupied(c, r)) return false;
    }
    return true;
}

// ─── Selection / click routing ──────────────────────────────────────────
void GameManager::handle_square_click(int col, int row) {
    if (game_over) return;
    if (!in_bounds(col, row)) { deselect(); return; }

    ChessPiece *clicked = piece_at(col, row);

    if (!selected_piece) {
        // Nothing selected yet: select only if there's a piece here AND
        // it belongs to whoever's turn it is.
        if (clicked && clicked->get_piece_color() == current_turn_color()) {
            selected_piece = clicked;
            selected_piece->set_selected_visual(true);
            emit_signal("selection_changed", col, row, true);
        }
        return;
    }

    // Something is already selected.
    int from_col = selected_piece->get_col();
    int from_row = selected_piece->get_row();

    if (col == from_col && row == from_row) {
        // Clicked the selected piece again -> deselect.
        deselect();
        return;
    }

    if (clicked && clicked->get_piece_color() == current_turn_color()) {
        // Clicked a different friendly piece -> switch selection.
        selected_piece->set_selected_visual(false);
        selected_piece = clicked;
        selected_piece->set_selected_visual(true);
        emit_signal("selection_changed", col, row, true);
        return;
    }

    // Otherwise: attempt the move (covers empty squares and enemy pieces).
    if (is_legal_move(selected_piece, from_col, from_row, col, row)) {
        execute_move(selected_piece, from_col, from_row, col, row);
    }

    deselect();
}

void GameManager::deselect() {
    if (selected_piece) {
        selected_piece->set_selected_visual(false);
        selected_piece = nullptr;
        emit_signal("selection_changed", -1, -1, false);
    }
}

// ─── Legality ───────────────────────────────────────────────────────────
// Full legality pipeline for a single proposed move. Pure function — does
// not mutate board state.
bool GameManager::is_legal_move(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row) const {
    if (!mover) return false;
    if (!in_bounds(from_col, from_row) || !in_bounds(to_col, to_row)) return false;
    if (from_col == to_col && from_row == to_row) return false;

    String mover_color = mover->get_piece_color();

    // Must be this color's turn.
    if (mover_color != current_turn_color()) return false;

    // Destination can't hold a friendly piece.
    if (is_friendly(to_col, to_row, mover_color)) return false;

    bool is_pawn = (mover->get_type() == PieceType::PAWN);
    bool is_king = (mover->get_type() == PieceType::KING);

    // ── Castling special case: king moving 2 squares horizontally ──
    if (is_king && from_row == to_row && std::abs(to_col - from_col) == 2) {
        bool kingside = (to_col - from_col) == 2;
        // Re-validate fully via the dedicated castle checker, but without
        // executing it (peek the conditions). We reuse try_castle_* logic
        // by checking the same prerequisites here.
        bool king_moved   = (mover_color == "white") ? white_king_moved   : black_king_moved;
        bool rook_a_moved = (mover_color == "white") ? white_rook_a_moved : black_rook_a_moved;
        bool rook_h_moved = (mover_color == "white") ? white_rook_h_moved : black_rook_h_moved;
        int back_row = (mover_color == "white") ? 0 : 7;

        if (from_row != back_row || from_col != 4) return false;
        if (king_moved) return false;
        if (is_in_check(mover_color)) return false; // can't castle out of check

        if (kingside) {
            if (rook_h_moved) return false;
            ChessPiece *rook = piece_at(7, back_row);
            if (!rook || rook->get_type() != PieceType::ROOK || rook->get_piece_color() != mover_color) return false;
            if (is_occupied(5, back_row) || is_occupied(6, back_row)) return false;
            String enemy = (mover_color == "white") ? "black" : "white";
            if (is_square_attacked(5, back_row, enemy)) return false;
            if (is_square_attacked(6, back_row, enemy)) return false;
            return true;
        } else {
            if (rook_a_moved) return false;
            ChessPiece *rook = piece_at(0, back_row);
            if (!rook || rook->get_type() != PieceType::ROOK || rook->get_piece_color() != mover_color) return false;
            if (is_occupied(1, back_row) || is_occupied(2, back_row) || is_occupied(3, back_row)) return false;
            String enemy = (mover_color == "white") ? "black" : "white";
            if (is_square_attacked(3, back_row, enemy)) return false;
            if (is_square_attacked(2, back_row, enemy)) return false;
            return true;
        }
    }

    // ── Geometric shape (subclass virtual) ──
    if (!const_cast<ChessPiece*>(mover)->is_valid_move(from_col, from_row, to_col, to_row)) return false;

    // ── Pawn-specific occupancy rules ──
    bool is_ep_capture = false;
    int ep_capture_col = to_col;
    int ep_capture_row = from_row; // the captured pawn sits beside the mover, same row it started on
    if (is_pawn) {
        if (!validate_pawn_capture_move(mover, from_col, from_row, to_col, to_row, is_ep_capture, ep_capture_col, ep_capture_row)) {
            return false;
        }
    } else {
        // ── Sliding pieces: path must be clear between from and to ──
        if (mover->get_type() == PieceType::BISHOP ||
            mover->get_type() == PieceType::ROOK   ||
            mover->get_type() == PieceType::QUEEN) {
            if (!path_clear(from_col, from_row, to_col, to_row)) return false;
        }
        // Knight ignores blocking by definition; king single-step has no
        // intermediate squares to check.
    }

    // ── King-safety: simulate the move and ensure mover's own king is
    //    not left in (or placed into) check. ──
    if (!simulate_and_check_safe(mover, from_col, from_row, to_col, to_row, is_ep_capture, ep_capture_col, ep_capture_row)) {
        return false;
    }

    return true;
}

bool GameManager::validate_pawn_capture_move(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row, bool &is_ep_capture, int &ep_capture_col, int &ep_capture_row) const {
    int dc = to_col - from_col;
    if (dc == 0) {
        // Straight move: destination must be empty; 2-square move must also
        // have an empty intermediate square.
        if (is_occupied(to_col, to_row)) return false;
        int mid_row = (from_row + to_row) / 2;
        if (std::abs(to_row - from_row) == 2 && is_occupied(from_col, mid_row)) return false;
        return true;
    }

    // Diagonal move: capture an enemy piece. En-passant is disabled to avoid confusion.
    if (is_occupied(to_col, to_row)) {
        return is_enemy(to_col, to_row, mover->get_piece_color());
    }

    return false;
}

std::vector<std::pair<int,int>> GameManager::get_fully_legal_moves(ChessPiece *mover, int col, int row) const {
    std::vector<std::pair<int,int>> result;
    if (!mover) return result;

    auto candidates = mover->get_valid_moves(col, row);

    // Board-level pawn capture validation: diagonal squares are only legal
    // if there is an enemy piece to capture.
    if (mover->get_type() == PieceType::PAWN) {
        std::vector<std::pair<int,int>> filtered;
        for (auto &mv : candidates) {
            int dc = mv.first - col;
            int dr = mv.second - row;
            if (std::abs(dc) == 1 && std::abs(dr) == 1) {
                if (!is_occupied(mv.first, mv.second)) {
                    continue;
                }
            }
            filtered.push_back(mv);
        }
        candidates = std::move(filtered);
    }

    // Castling candidates aren't produced by get_valid_moves (king geometry
    // is single-step only) — add them explicitly so highlights show them.
    if (mover->get_type() == PieceType::KING) {
        candidates.push_back({col + 2, row});
        candidates.push_back({col - 2, row});
    }

    for (auto &mv : candidates) {
        if (is_legal_move(mover, col, row, mv.first, mv.second)) {
            result.push_back(mv);
        }
    }
    return result;
}

// ─── Check / attack detection ───────────────────────────────────────────
std::pair<int,int> GameManager::find_king(const String &color) const {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            ChessPiece *p = piece_at(c, r);
            if (p && p->get_type() == PieceType::KING && p->get_piece_color() == color) {
                return {c, r};
            }
        }
    }
    return {-1, -1};
}

bool GameManager::is_square_attacked(int col, int row, const String &by_attacking_color) const {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            ChessPiece *p = piece_at(c, r);
            if (!p || p->get_piece_color() != by_attacking_color) continue;

            PieceType t = p->get_type();

            if (t == PieceType::PAWN) {
                // Pawns attack diagonally only (never straight ahead).
                int direction = (by_attacking_color == "white") ? 1 : -1;
                if (r + direction == row && std::abs(c - col) == 1) return true;
                continue;
            }

            if (t == PieceType::KING) {
                // Avoid infinite recursion / castling special-case noise —
                // king attacks are just adjacency.
                int dc = std::abs(c - col), dr = std::abs(r - row);
                if (dc <= 1 && dr <= 1 && !(dc == 0 && dr == 0)) return true;
                continue;
            }

            // Knight: pure geometry, no blocking.
            if (t == PieceType::KNIGHT) {
                if (const_cast<ChessPiece*>(p)->is_valid_move(c, r, col, row)) return true;
                continue;
            }

            // Sliding pieces: geometry + clear path.
            if (t == PieceType::BISHOP || t == PieceType::ROOK || t == PieceType::QUEEN) {
                if (const_cast<ChessPiece*>(p)->is_valid_move(c, r, col, row) && path_clear(c, r, col, row)) {
                    return true;
                }
                continue;
            }
        }
    }
    return false;
}

bool GameManager::is_in_check(const String &color) const {
    auto [kc, kr] = find_king(color);
    if (kc < 0) return false; // king missing (shouldn't happen) — treat as not-in-check
    String enemy = (color == "white") ? "black" : "white";
    return is_square_attacked(kc, kr, enemy);
}

// Simulates a move on the logical board only (no visuals, no piece state
// changes), checks whether the mover's own king ends up safe, then
// restores the board exactly as it was. Marked const via internal
// const_cast since the mutation is fully reversed before returning.
bool GameManager::simulate_and_check_safe(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row,
                                           bool is_ep_capture, int ep_capture_col, int ep_capture_row) const {
    GameManager *self = const_cast<GameManager*>(this);

    ChessPiece *captured_normal = self->board[idx(to_col, to_row)];
    ChessPiece *captured_ep = nullptr;

    // Apply
    self->board[idx(from_col, from_row)] = nullptr;
    self->board[idx(to_col, to_row)] = mover;

    if (is_ep_capture) {
        captured_ep = self->board[idx(ep_capture_col, ep_capture_row)];
        self->board[idx(ep_capture_col, ep_capture_row)] = nullptr;
    }

    bool safe = !is_in_check(mover->get_piece_color());

    // Revert
    self->board[idx(from_col, from_row)] = mover;
    self->board[idx(to_col, to_row)] = captured_normal;
    if (is_ep_capture) {
        self->board[idx(ep_capture_col, ep_capture_row)] = captured_ep;
    }

    return safe;
}

bool GameManager::color_has_any_legal_move(const String &color) const {
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            ChessPiece *p = piece_at(c, r);
            if (!p || p->get_piece_color() != color) continue;
            if (!get_fully_legal_moves(p, c, r).empty()) return true;
        }
    }
    return false;
}

// ─── Move execution ─────────────────────────────────────────────────────
bool GameManager::execute_move(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row) {
    if (!mover) return false;
    String mover_color = mover->get_piece_color();
    PieceType type = mover->get_type();

    bool is_pawn = (type == PieceType::PAWN);
    bool is_king = (type == PieceType::KING);

    // ── Detect castling (king double-step) before mutating anything ──
    if (is_king && from_row == to_row && std::abs(to_col - from_col) == 2) {
        bool kingside = (to_col - from_col) == 2;
        if (kingside) return try_castle_kingside(mover_color);
        else          return try_castle_queenside(mover_color);
    }

    // ── Determine en-passant capture before we overwrite ep_col/ep_row ──
    bool is_ep_capture = false;
    int ep_capture_col = to_col, ep_capture_row = from_row;
    if (is_pawn && (to_col != from_col) && !is_occupied(to_col, to_row)) {
        if (to_col == ep_col && to_row == ep_row) {
            is_ep_capture = true;
        }
    }

    ChessPiece *captured = piece_at(to_col, to_row);
    ChessPiece *ep_captured = nullptr;
    if (is_ep_capture) {
        ep_captured = piece_at(ep_capture_col, ep_capture_row);
    }

    std::string notation;
    char file_to = 'a' + to_col;
    char rank_to = '1' + to_row;
    bool was_capture = (captured != nullptr) || is_ep_capture;

    // Build simple algebraic-ish notation: piece letter (blank for pawn) +
    // optional 'x' + destination square. Good enough for a move-history UI.
    if (type != PieceType::PAWN) notation += mover->get_type_char();
    if (was_capture) notation += 'x';
    notation += file_to;
    notation += rank_to;

    // ── Snapshot BEFORE any mutation (Memento captures the state to RETURN TO) ──
    // This must come before board[], captured_pieces, ep_col/row, castling
    // flags, and has_moved are changed — otherwise undo would restore the
    // post-move state instead of the pre-move state.
    BoardMemento snap = capture_snapshot(notation);
    history.push_back(snap);

    // ── Mutate logical board ──
    board[idx(from_col, from_row)] = nullptr;
    board[idx(to_col, to_row)] = mover;

    // ── Park captured piece(s) off-board (kept alive for undo) ──
    if (captured) {
        char ccolor = (captured->get_piece_color() == "white") ? 'w' : 'b';
        captured_pieces.push_back({captured->get_type_char(), ccolor});
        int slot = 0;
        for (auto &cp : captured_pieces) if (cp.second == ccolor) slot++;
        captured->set_captured_visual(true, slot);
        emit_signal("piece_captured", captured->get_type_name(), captured->get_piece_color());
        emit_signal("capture_occurred", to_col, to_row);
    }
    if (ep_captured) {
        board[idx(ep_capture_col, ep_capture_row)] = nullptr;
        char ccolor = (ep_captured->get_piece_color() == "white") ? 'w' : 'b';
        captured_pieces.push_back({ep_captured->get_type_char(), ccolor});
        int slot = 0;
        for (auto &cp : captured_pieces) if (cp.second == ccolor) slot++;
        ep_captured->set_captured_visual(true, slot);
        emit_signal("piece_captured", ep_captured->get_type_name(), ep_captured->get_piece_color());
        emit_signal("capture_occurred", ep_capture_col, ep_capture_row);
    }

    // ── Move the piece visually + update its own col/row ──
    mover->set_grid_position(to_col, to_row);
    mover->set_has_moved(true);

    // ── Track moved-state for castling rights ──
    if (is_king) {
        if (mover_color == "white") white_king_moved = true; else black_king_moved = true;
    }
    if (type == PieceType::ROOK) {
        if (mover_color == "white") {
            if (from_col == 0 && from_row == 0) white_rook_a_moved = true;
            if (from_col == 7 && from_row == 0) white_rook_h_moved = true;
        } else {
            if (from_col == 0 && from_row == 7) black_rook_a_moved = true;
            if (from_col == 7 && from_row == 7) black_rook_h_moved = true;
        }
    }
    // A captured rook also permanently forfeits that side's castling right.
    if (captured && captured->get_type() == PieceType::ROOK) {
        String cc = captured->get_piece_color();
        if (cc == "white") {
            if (to_col == 0 && to_row == 0) white_rook_a_moved = true;
            if (to_col == 7 && to_row == 0) white_rook_h_moved = true;
        } else {
            if (to_col == 0 && to_row == 7) black_rook_a_moved = true;
            if (to_col == 7 && to_row == 7) black_rook_h_moved = true;
        }
    }

    // ── New en-passant target: set only on a pawn's first 2-square move ──
    ep_col = -1; ep_row = -1;
    if (is_pawn && std::abs(to_row - from_row) == 2) {
        ep_col = to_col;
        ep_row = (from_row + to_row) / 2;
    }

    // ── Promotion check ──
    bool triggers_promotion = false;
    if (is_pawn) {
        int last_row = (mover_color == "white") ? 7 : 0;
        if (to_row == last_row) triggers_promotion = true;
    }

    if (triggers_promotion) {
        promotion_pending = true;
        promotion_col = to_col;
        promotion_row = to_row;
        promotion_color = mover_color;
        emit_signal("promotion_needed", to_col, to_row, mover_color);
        // Turn does NOT switch yet — finalized in promote_pending().
        emit_signal("move_made", String(notation.c_str()));
        return true;
    }

    // ── Switch turn & refresh check/mate state ──
    white_to_move = !white_to_move;
    emit_signal("move_made", String(notation.c_str()));
    emit_signal("turn_changed", current_turn_color());
    refresh_game_over_state();

    return true;
}

// ─── Castling execution ─────────────────────────────────────────────────
bool GameManager::try_castle_kingside(const String &color) {
    int back_row = (color == "white") ? 0 : 7;
    ChessPiece *king = piece_at(4, back_row);
    ChessPiece *rook = piece_at(7, back_row);
    if (!king || king->get_type() != PieceType::KING || king->get_piece_color() != color) return false;
    if (!rook || rook->get_type() != PieceType::ROOK || rook->get_piece_color() != color) return false;

    bool king_moved = (color == "white") ? white_king_moved : black_king_moved;
    bool rook_moved  = (color == "white") ? white_rook_h_moved : black_rook_h_moved;
    if (king_moved || rook_moved) return false;
    if (is_occupied(5, back_row) || is_occupied(6, back_row)) return false;

    String enemy = (color == "white") ? "black" : "white";
    if (is_in_check(color)) return false;
    if (is_square_attacked(5, back_row, enemy)) return false;
    if (is_square_attacked(6, back_row, enemy)) return false;

    // Snapshot BEFORE mutating board
    std::string notation = "O-O";
    BoardMemento snap = capture_snapshot(notation);
    history.push_back(snap);

    // Mutate board
    board[idx(4, back_row)] = nullptr;
    board[idx(7, back_row)] = nullptr;
    board[idx(6, back_row)] = king;
    board[idx(5, back_row)] = rook;

    king->set_grid_position(6, back_row);
    king->set_has_moved(true);
    rook->set_grid_position(5, back_row);
    rook->set_has_moved(true);

    if (color == "white") { white_king_moved = true; white_rook_h_moved = true; }
    else                  { black_king_moved = true; black_rook_h_moved = true; }

    ep_col = -1; ep_row = -1;

    white_to_move = !white_to_move;
    emit_signal("move_made", String("O-O"));
    emit_signal("turn_changed", current_turn_color());
    refresh_game_over_state();
    return true;
}

bool GameManager::try_castle_queenside(const String &color) {
    int back_row = (color == "white") ? 0 : 7;
    ChessPiece *king = piece_at(4, back_row);
    ChessPiece *rook = piece_at(0, back_row);
    if (!king || king->get_type() != PieceType::KING || king->get_piece_color() != color) return false;
    if (!rook || rook->get_type() != PieceType::ROOK || rook->get_piece_color() != color) return false;

    bool king_moved = (color == "white") ? white_king_moved : black_king_moved;
    bool rook_moved  = (color == "white") ? white_rook_a_moved : black_rook_a_moved;
    if (king_moved || rook_moved) return false;
    if (is_occupied(1, back_row) || is_occupied(2, back_row) || is_occupied(3, back_row)) return false;

    String enemy = (color == "white") ? "black" : "white";
    if (is_in_check(color)) return false;
    if (is_square_attacked(3, back_row, enemy)) return false;
    if (is_square_attacked(2, back_row, enemy)) return false;

    // Snapshot BEFORE mutating board
    std::string notation = "O-O-O";
    BoardMemento snap = capture_snapshot(notation);
    history.push_back(snap);

    board[idx(4, back_row)] = nullptr;
    board[idx(0, back_row)] = nullptr;
    board[idx(2, back_row)] = king;
    board[idx(3, back_row)] = rook;

    king->set_grid_position(2, back_row);
    king->set_has_moved(true);
    rook->set_grid_position(3, back_row);
    rook->set_has_moved(true);

    if (color == "white") { white_king_moved = true; white_rook_a_moved = true; }
    else                  { black_king_moved = true; black_rook_a_moved = true; }

    ep_col = -1; ep_row = -1;

    white_to_move = !white_to_move;
    emit_signal("move_made", String("O-O-O"));
    emit_signal("turn_changed", current_turn_color());
    refresh_game_over_state();
    return true;
}

// ─── Promotion ───────────────────────────────────────────────────────────
// GameManager cannot spawn new piece nodes itself (model loading lives in
// GDScript / main.gd). The contract is:
//   1. execute_move() detects a pawn reaching the last rank and emits
//      "promotion_needed(col, row, color)" WITHOUT switching the turn.
//   2. GDScript shows a promotion picker UI, then:
//        a. frees the pawn node
//        b. spawns the chosen piece (queen/rook/bishop/knight) at (col,row)
//        c. calls GameManager.complete_promotion(col, row, new_piece)
//   3. complete_promotion() finishes the bookkeeping: registers the new
//      piece into the board array, switches the turn, and refreshes
//      check/mate state.
void GameManager::promote_pending(const String &piece_type) {
    (void)piece_type; // unused: this stub exists for API compatibility only
    // Kept for API compatibility / simple cases; prefer complete_promotion
    // from GDScript since it has the freshly-spawned node reference.
    if (!promotion_pending) return;
    promotion_pending = false;
    white_to_move = !white_to_move;
    emit_signal("turn_changed", current_turn_color());
    refresh_game_over_state();
}

void GameManager::complete_promotion(int col, int row, ChessPiece *new_piece) {
    if (!promotion_pending) return;
    if (!in_bounds(col, row) || !new_piece) { promotion_pending = false; return; }

    board[idx(col, row)] = new_piece;
    new_piece->set_has_moved(true);
    all_registered.push_back(new_piece);

    promotion_pending = false;
    promotion_col = promotion_row = -1;

    white_to_move = !white_to_move;
    emit_signal("turn_changed", current_turn_color());
    refresh_game_over_state();
}

// ─── Snapshot / restore (Memento) ────────────────────────────────────────
BoardMemento GameManager::capture_snapshot(const std::string &notation) const {
    BoardMemento snap;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            ChessPiece *p = piece_at(c, r);
            int i = idx(c, r);
            if (p) {
                snap.type[i] = p->get_type_char();
                snap.color[i] = (p->get_piece_color() == "white") ? 'w' : 'b';
                snap.has_moved[i] = p->get_has_moved();
            } else {
                snap.type[i] = ' ';
                snap.color[i] = ' ';
                snap.has_moved[i] = false;
            }
        }
    }
    snap.white_to_move = white_to_move;
    snap.white_king_moved = white_king_moved;
    snap.black_king_moved = black_king_moved;
    snap.white_rook_a_moved = white_rook_a_moved;
    snap.white_rook_h_moved = white_rook_h_moved;
    snap.black_rook_a_moved = black_rook_a_moved;
    snap.black_rook_h_moved = black_rook_h_moved;
    snap.ep_col = ep_col;
    snap.ep_row = ep_row;
    snap.captured = captured_pieces;
    snap.notation = notation;
    return snap;
}

// ─── Undo ────────────────────────────────────────────────────────────────
bool GameManager::can_undo() const {
    return !history.empty();
}

void GameManager::undo() {
    if (history.empty()) return;

    // Cancel any pending promotion / selection before rewinding.
    promotion_pending = false;
    promotion_col = promotion_row = -1;
    deselect();

    BoardMemento snap = history.back();
    history.pop_back();

    restore_snapshot(snap);

    emit_signal("undo_performed");
    emit_signal("turn_changed", current_turn_color());
    refresh_game_over_state();
}

void GameManager::restore_snapshot(const BoardMemento &snap) {
    // First, figure out which pieces SHOULD be on the board afterward vs.
    // which should be parked (captured), by matching type+color+square
    // against currently-known nodes. We rebuild board[] from scratch.

    std::array<ChessPiece*, 64> new_board;
    new_board.fill(nullptr);
    std::vector<bool> used(all_registered.size(), false);

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int i = idx(c, r);
            if (snap.type[i] == ' ') continue;

            char want_type = snap.type[i];
            char want_color = snap.color[i];

            // Find an unused registered piece matching type+color. Prefer
            // one already at (c,r) if present (minimizes pointless moves),
            // otherwise take any unused match (e.g. a piece coming back
            // from the captured-parking area).
            int found = -1;
            for (size_t k = 0; k < all_registered.size(); k++) {
                if (used[k]) continue;
                ChessPiece *p = all_registered[k];
                if (!p) continue;
                char pt = p->get_type_char();
                char pc = (p->get_piece_color() == "white") ? 'w' : 'b';
                if (pt == want_type && pc == want_color) {
                    if (p->get_col() == c && p->get_row() == r) { found = (int)k; break; }
                    if (found == -1) found = (int)k;
                }
            }

            if (found >= 0) {
                used[found] = true;
                ChessPiece *p = all_registered[found];
                new_board[i] = p;
                p->set_captured_visual(false);
                p->set_grid_position_silent(c, r);
                p->set_has_moved(snap.has_moved[i]);
            }
        }
    }

    // Any registered piece NOT used in the rebuilt board is captured in
    // this snapshot — park it.
    int white_slot = 0, black_slot = 0;
    for (size_t k = 0; k < all_registered.size(); k++) {
        if (used[k]) continue;
        ChessPiece *p = all_registered[k];
        if (!p) continue;
        bool is_white = (p->get_piece_color() == "white");
        int slot = is_white ? ++white_slot : ++black_slot;
        p->set_captured_visual(true, slot);
    }

    board = new_board;

    white_to_move = snap.white_to_move;
    white_king_moved = snap.white_king_moved;
    black_king_moved = snap.black_king_moved;
    white_rook_a_moved = snap.white_rook_a_moved;
    white_rook_h_moved = snap.white_rook_h_moved;
    black_rook_a_moved = snap.black_rook_a_moved;
    black_rook_h_moved = snap.black_rook_h_moved;
    ep_col = snap.ep_col;
    ep_row = snap.ep_row;
    captured_pieces = snap.captured;

    game_over = false;
    game_over_reason = "";
}

// ─── Game-over detection ────────────────────────────────────────────────
void GameManager::refresh_game_over_state() {
    String color = current_turn_color();
    bool in_check = is_in_check(color);
    bool has_move = color_has_any_legal_move(color);

    if (in_check) {
        emit_signal("check_started", color);
    } else {
        emit_signal("check_cleared");
    }

    if (!has_move) {
        game_over = true;
        if (in_check) {
            String winner = (color == "white") ? "Black" : "White";
            game_over_reason = winner + " wins by checkmate";
        } else {
            game_over_reason = "Draw by stalemate";
        }
        emit_signal("game_over", game_over_reason);
    } else {
        game_over = false;
        game_over_reason = "";
    }
}

// ─── UI helpers ──────────────────────────────────────────────────────────
Array GameManager::get_captured_pieces() const {
    Array result;
    for (auto &cp : captured_pieces) {
        Dictionary d;
        d["type"] = String::chr(cp.first);
        d["color"] = (cp.second == 'w') ? String("white") : String("black");
        result.push_back(d);
    }
    return result;
}

void GameManager::notify_piece_freed(ChessPiece *piece) {
    auto it = std::find(all_registered.begin(), all_registered.end(), piece);
    if (it != all_registered.end()) all_registered.erase(it);
    for (auto &slot : board) {
        if (slot == piece) slot = nullptr;
    }
    if (selected_piece == piece) selected_piece = nullptr;
}

void GameManager::_on_piece_tree_exiting(ChessPiece *piece) {
    notify_piece_freed(piece);
}
