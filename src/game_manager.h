#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <array>
#include <vector>
#include <utility>
#include <string>

namespace godot {

class ChessPiece;

// ─── One immutable snapshot of the whole game — the "Memento" ─────────
// Pushed onto the history stack after every successful move, so Undo can
// restore everything: board layout, whose turn it is, castling rights,
// en-passant target, and captured-piece list.
struct BoardMemento {
    std::array<char, 64> type{};       // 'P','N','B','R','Q','K', or ' ' (empty)
    std::array<char, 64> color{};      // 'w','b', or ' '
    std::array<bool, 64> has_moved{};

    bool white_to_move = true;

    bool white_king_moved = false;
    bool black_king_moved = false;
    bool white_rook_a_moved = false;
    bool white_rook_h_moved = false;
    bool black_rook_a_moved = false;
    bool black_rook_h_moved = false;

    int ep_col = -1;
    int ep_row = -1;

    std::vector<std::pair<char,char>> captured; // (type, color) in capture order
    std::string notation;                        // e.g. "e2-e4", "Nb1xc3", "O-O"
};

class GameManager : public Node {
    GDCLASS(GameManager, Node)

private:
    static GameManager *singleton;

    // 8x8 logical board: index = row * 8 + col. nullptr == empty.
    std::array<ChessPiece*, 64> board;

    bool white_to_move;

    bool white_king_moved;
    bool black_king_moved;
    bool white_rook_a_moved;
    bool white_rook_h_moved;
    bool black_rook_a_moved;
    bool black_rook_h_moved;

    int ep_col; // en-passant target square (-1,-1 if none active this turn)
    int ep_row;

    bool game_over;
    String game_over_reason;

    ChessPiece *selected_piece;

    std::vector<BoardMemento> history;
    std::vector<std::pair<char,char>> captured_pieces;

    int idx(int col, int row) const { return row * 8 + col; }

    // Internal helpers (no Godot binding needed)
    bool path_clear(int from_col, int from_row, int to_col, int to_row) const;
    bool validate_pawn_capture_move(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row, bool &is_ep_capture, int &ep_capture_col, int &ep_capture_row) const;
    bool simulate_and_check_safe(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row, bool is_ep_capture, int ep_capture_col, int ep_capture_row) const;
    void place(ChessPiece *piece, int col, int row); // updates board[] only
    BoardMemento capture_snapshot(const std::string &notation) const;
    void restore_snapshot(const BoardMemento &snap);
    std::vector<ChessPiece*> all_registered; // every piece ever registered (for undo re-insertion of captures)

protected:
    static void _bind_methods();

public:
    GameManager();
    ~GameManager();

    static GameManager *get_singleton();

    void _ready() override;

    // ── Registration ──────────────────────────────────────────────
    void register_piece(ChessPiece *piece, int col, int row);

    // ── Board queries ─────────────────────────────────────────────
    bool is_occupied(int col, int row) const;
    ChessPiece *piece_at(int col, int row) const;
    bool is_enemy(int col, int row, const String &mover_color) const;
    bool is_friendly(int col, int row, const String &mover_color) const;

    // ── Turn / state ───────────────────────────────────────────────
    bool is_white_turn() const { return white_to_move; }
    String current_turn_color() const { return white_to_move ? "white" : "black"; }
    bool is_game_over() const { return game_over; }
    String get_game_over_reason() const { return game_over_reason; }

    int get_ep_col() const { return ep_col; }
    int get_ep_row() const { return ep_row; }

    // ── Selection / click routing (single source of truth) ─────────
    // Called by ChessPiece::on_click (with that piece's own square) and by
    // ChessBoard::on_board_click (with the clicked empty/occupied square).
    void handle_square_click(int col, int row);
    void deselect();
    ChessPiece *get_selected_piece() const { return selected_piece; }

    // ── Legality ─────────────────────────────────────────────────
    // Full legality: subclass geometric shape + occupancy/blocking +
    // friendly-fire + pawn capture-only-diagonal/straight-no-capture +
    // en passant + king-safety (does not leave mover's own king in check).
    bool is_legal_move(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row) const;
    std::vector<std::pair<int,int>> get_fully_legal_moves(ChessPiece *mover, int col, int row) const;

    bool is_in_check(const String &color) const;
    bool is_square_attacked(int col, int row, const String &by_attacking_color) const;
    std::pair<int,int> find_king(const String &color) const;
    bool color_has_any_legal_move(const String &color) const;

    // ── Move execution ──────────────────────────────────────────────
    // Executes a fully-legal move (assumes is_legal_move already passed).
    // Handles capture, en passant capture, castling rook hop, promotion
    // trigger, turn switch, history push, and check/mate refresh.
    bool execute_move(ChessPiece *mover, int from_col, int from_row, int to_col, int to_row);

    // ── Castling (invoked internally by execute_move when a king makes a
    //    2-square horizontal move; exposed too in case GDScript wants to
    //    offer a direct "castle" button) ──
    bool try_castle_kingside(const String &color);
    bool try_castle_queenside(const String &color);

    // ── Promotion ─────────────────────────────────────────────────
    bool promotion_pending;
    int promotion_col, promotion_row;
    String promotion_color;
    void promote_pending(const String &piece_type);
    // Called by GDScript after it has freed the pawn and spawned the
    // replacement piece node. Registers the new piece into the logical
    // board, removes any stale undo ambiguity, switches the turn, and
    // refreshes check/mate state.
    void complete_promotion(int col, int row, ChessPiece *new_piece);

    // ── Undo ──────────────────────────────────────────────────────
    bool can_undo() const;
    void undo();

    // ── UI helpers ────────────────────────────────────────────────
    Array get_captured_pieces() const;
    void refresh_game_over_state();

    // ── Restart ───────────────────────────────────────────────────
    void notify_piece_freed(ChessPiece *piece); // called from piece destructor path if needed

    // Internal signal handler — bound so it can be used as a Callable target.
    void _on_piece_tree_exiting(ChessPiece *piece);

    // ── Mode control ──────────────────────────────────────────────────
    // Set false in Advanced mode so no highlights are drawn.
    bool highlights_enabled = true;
    void set_highlights_enabled(bool enabled) { highlights_enabled = enabled; }
    bool get_highlights_enabled() const { return highlights_enabled; }
};

}

#endif // GAME_MANAGER_H
