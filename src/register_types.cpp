#include "register_types.h"
#include "chess_piece.h"
#include "chess_board.h"
#include "chess_king.h"
#include "chess_queen.h"
#include "chess_rook.h"
#include "chess_bishop.h"
#include "chess_knight.h"
#include "chess_pawn.h"
#include "game_manager.h"
#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_chess_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    ClassDB::register_class<ChessPiece>();
    ClassDB::register_class<ChessBoard>();
    ClassDB::register_class<GameManager>();
    ClassDB::register_class<ChessKing>();
    ClassDB::register_class<ChessQueen>();
    ClassDB::register_class<ChessRook>();
    ClassDB::register_class<ChessBishop>();
    ClassDB::register_class<ChessKnight>();
    ClassDB::register_class<ChessPawn>();
}

void uninitialize_chess_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT chess_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_chess_module);
    init_obj.register_terminator(uninitialize_chess_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}
