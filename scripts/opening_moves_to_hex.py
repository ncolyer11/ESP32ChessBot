import chess

# Assuming this script is run from a context where `chess` is available,
# or you would typically import it.
# from chess import Board, Move

# --- Your existing mapping functions (no changes needed here) ---
file_map = {f: i for i, f in enumerate("abcdefgh")}
rank_map = {r: (int(r) - 1) * 8 for r in "12345678"}

def square_index(sq: str) -> int:
    return file_map[sq[0]] + rank_map[sq[1]]

# --- Removed initial piece squares as we'll use board state ---

ENPASSANT_FLAG = 1
CASTLING_FLAG = 2
PROMOTION_FLAG = 3
DOUBLE_PAWN_FLAG = 7

# --- Modified encode function to use the board state ---
def encode(move_san: str, board: chess.Board) -> int:
    flag = 0
    src_sq = 0
    dst_sq = 0

    if move_san in ("O-O", "O-O-O"):
        flag = CASTLING_FLAG
        # Castling moves can be directly determined
        if board.turn == chess.WHITE:
            src_sq = square_index("e1")
            dst_sq = square_index("g1") if move_san == "O-O" else square_index("c1")
        else:
            src_sq = square_index("e8")
            dst_sq = square_index("g8") if move_san == "O-O" else square_index("c8")
    else:
        # Use python-chess to parse the SAN move and get source/destination squares
        # This is the crucial part that solves the "choose_source" issue
        try:
            # Generate all legal moves from the current board position
            # and find the one that matches the SAN string
            move_obj = board.parse_san(move_san)
            src_sq = move_obj.from_square
            dst_sq = move_obj.to_square

            # Check for special flags
            if board.piece_at(src_sq).piece_type == chess.PAWN:
                if abs(chess.square_rank(src_sq) - chess.square_rank(dst_sq)) == 2:
                    flag = DOUBLE_PAWN_FLAG
                if move_obj.promotion:
                    flag = PROMOTION_FLAG
                # En passant detection is more complex and requires checking board state
                # For simplicity here, we might omit it if not explicitly needed,
                # or handle it based on how the move is parsed if board state implies it.
                # A proper check would involve board.is_en_passant() before making the move.
            
            # This is a basic way to check for en passant if it's implicitly part of the move
            # For a pawn move where the destination is empty, but the move is a capture
            if board.piece_at(src_sq).piece_type == chess.PAWN and \
               board.is_en_passant(move_obj):
                flag = ENPASSANT_FLAG


        except ValueError as e:
            raise ValueError(f"Could not parse SAN move '{move_san}' on board {board.fen()}: {e}")

    return (flag << 12) | (dst_sq << 6) | src_sq

# --- Your opening_moves data (no changes needed here) ---
opening_moves = [
    [  # First Ply
        ["e4", "d4", "Nf3", "c4", "e3", "g3", "b3", "d3", "f4", "Nc3"],  # White
        ["e5", "c5", "d5", "e6", "c6", "d6", "g6", "b6", "Nf6", "Nc6"]   # Black
    ],
    [  # Second Ply
        ["Nf3", "Bc4", "Nc3", "d4", "f4", "d3", "Qh5", "Qf3", "c3", "f3"],  # White      
        ["Nc6", "d6", "Nf6", "Bc5", "Qf6", "d5", "f6", "f5", "Bd6", "c6"]   # Black      
    ],
    [  # Third Ply
        ["Bc4", "Bb5", "d4", "Nc3", "c3", "d3", "Be2", "g3", "a3", "b3"],  # White       
        ["Nf6", "Bc5", "h6", "d6", "Nd4", "Be7", "f5", "f6", "a6", "Qf6"]   # Black      
    ],
    [  # 4th Ply
        ["Ng5", "d3", "Nc3", "O-O", "d4", "c3", "Bxf7+", "Qe2", "Nxe5", "a3"],  # White  
        ["d5", "Bc5", "Nxe4", "Qe7", "h6", "d6", "Be7", "Rg8", "Na5", "Nd4"]   # Black   
    ],
    [  # 5th Ply
        ["exd5", "Bxd5", "Nxf7", "Bb3", "Bb5", "Bd3", "Qf3", "d3", "d4", "Qh5"],  # White
        ["Nxd5", "Na5", "Nd4", "b5", "Ne7", "Nb4", "h6", "Bg4", "Bc5", "Ng4"]   # Black  
    ],
]


phase_names = ["First", "Second", "Third", "Fourth", "Fifth", "Sixth"] # Extend for 6 plies
color_names = ["WHITE", "BLACK"]

print("static constexpr Move opening_moves_encoded[3][2][10] = {") # Assuming 3 phases, 2 colors, 10 moves
# Create a board instance to track the game state
board = chess.Board()

for i, phase in enumerate(opening_moves):
    # Reset board for each "phase" (ply group) if you want to test independently
    # However, for a sequence of moves, we must push moves on the same board
    
    if i < len(phase_names):
        print(f"    // {phase_names[i]} move options")
    print("    {")
    
    # We need to process moves one by one to update the board accurately
    # This means we'll run `encode` for White's moves, then update the board,
    # then run `encode` for Black's moves, then update the board.
    
    current_ply_encoded_moves = [[], []] # [White's encoded, Black's encoded]

    # Process White's moves for this ply
    white_san_moves = phase[0]
    print("        {", end="")
    temp_white_vals = []
    for mv in white_san_moves:
        # Before encoding White's move, the board is ready for White's turn
        code = encode(mv, board) # Pass the current board state
        temp_white_vals.append(f"0x{code:04X}")
    
    print(", ".join(temp_white_vals), "},")
    current_ply_encoded_moves[0] = temp_white_vals

    # Print White's SAN comment
    print("        //", end="")
    for k, mv in enumerate(white_san_moves):
        if k == 0:
            print(f'  "{mv}"', end="")
        else:
            print(f',   "{mv}"', end="")
    print(f"\n\t// {color_names[0]}")
    print() # Blank line after White's moves


    # Now, push the *first* (most popular) White move to update the board for Black's response
    # This assumes you want to calculate Black's options *after* White has made a move.
    # If your `opening_moves` data represents ALL possible moves from the initial position for each ply,
    # then this push logic might need re-evaluation based on your exact encoding strategy.
    # For now, let's assume `opening_moves` gives top choices per board state.
    try:
        if white_san_moves:
            board.push_san(white_san_moves[0])
    except ValueError as e:
        print(f"Error pushing White's move '{white_san_moves[0]}' at ply {i+1}: {e}")
        break # Cannot continue if the board state cannot be advanced
    
    # Process Black's moves for this ply (after White's hypothetical move)
    black_san_moves = phase[1]
    print("        {", end="")
    temp_black_vals = []
    for mv in black_san_moves:
        # Before encoding Black's move, the board is ready for Black's turn (after White's move)
        code = encode(mv, board) # Pass the current board state
        temp_black_vals.append(f"0x{code:04X}")

    print(", ".join(temp_black_vals), "},")
    current_ply_encoded_moves[1] = temp_black_vals

    # Print Black's SAN comment
    print("        //", end="")
    for k, mv in enumerate(black_san_moves):
        if k == 0:
            print(f'  "{mv}"', end="")
        else:
            print(f',   "{mv}"', end="")
    print(f"\n\t// {color_names[1]}")
    
    # Now, push the *first* (most popular) Black move to update the board for the next White move
    try:
        if black_san_moves:
            board.push_san(black_san_moves[0])
    except ValueError as e:
        print(f"Error pushing Black's move '{black_san_moves[0]}' at ply {i+1}: {e}")
        break # Cannot continue if the board state cannot be advanced
    
    print("    },") # Closing brace for the current phase (ply group)

print("};")