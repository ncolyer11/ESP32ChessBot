# encode_opening_moves.py

file_map = {f: i for i, f in enumerate("abcdefgh")}
rank_map = {r: (int(r) - 1) * 8 for r in "12345678"}

def square_index(sq: str) -> int:
    return file_map[sq[0]] + rank_map[sq[1]]

# initial piece squares by color (0=white,1=black)
initial = {
    0: {
        'P': [f + '2' for f in "abcdefgh"],
        'N': ["b1", "g1"],
        'B': ["c1", "f1"],
        'R': ["a1", "h1"],
        'Q': ["d1"],
        'K': ["e1"],
    },
    1: {
        'P': [f + '7' for f in "abcdefgh"],
        'N': ["b8", "g8"],
        'B': ["c8", "f8"],
        'R': ["a8", "h8"],
        'Q': ["d8"],
        'K': ["e8"],
    }
}

ENPASSANT_FLAG = 1
CASTLING_FLAG = 2
PROMOTION_FLAG = 3
DOUBLE_PAWN_FLAG = 7

def is_knight_move(src: int, dst: int) -> bool:
    dx = abs((src & 7) - (dst & 7))
    dy = abs((src >> 3) - (dst >> 3))
    return sorted((dx, dy)) == [1, 2]

def is_straight_move(src: int, dst: int) -> bool:
    return (src & 7) == (dst & 7) or (src >> 3) == (dst >> 3)

def is_diagonal_move(src: int, dst: int) -> bool:
    dx = abs((src & 7) - (dst & 7))
    dy = abs((src >> 3) - (dst >> 3))
    return dx == dy

def choose_source(piece: str, dst_sq: int, color: int) -> int:
    for src in initial[color][piece]:
        idx = square_index(src)
        if piece == 'N' and is_knight_move(idx, dst_sq): return idx
        if piece == 'B' and is_diagonal_move(idx, dst_sq): return idx
        if piece == 'R' and is_straight_move(idx, dst_sq): return idx
        if piece in ('Q', 'K') and (is_straight_move(idx, dst_sq) or is_diagonal_move(idx, dst_sq)): return idx
    raise ValueError(f"no valid {piece} from for {dst_sq}")

def encode(move: str, color: int) -> int:
    flag = 0
    if move in ("O-O", "O-O-O"):
        flag = CASTLING_FLAG
        if color == 0:
            src_sq = square_index("e1")
            dst_sq = square_index("g1") if move == "O-O" else square_index("c1")
        else:
            src_sq = square_index("e8")
            dst_sq = square_index("g8") if move == "O-O" else square_index("c8")
    else:
        m = move.replace("x", "")
        promo = None
        if "=" in m:
            m, promo = m.split("=")
        dst = m[-2:]
        dst_sq = square_index(dst)
        p = m[0] if m[0].isupper() else 'P'
        if p == 'P':
            file = m[0] if m[0].isalpha() and len(m) == 3 else dst[0]
            src = file + ('2' if color == 0 else '7')
            src_sq = square_index(src)
            if abs((dst_sq >> 3) - (src_sq >> 3)) == 2:
                flag = DOUBLE_PAWN_FLAG
        else:
            src_sq = choose_source(p, dst_sq, color)
        if promo:
            flag = PROMOTION_FLAG
    return (flag << 12) | (dst_sq << 6) | src_sq

opening_moves = [
    [  # First Ply
        ["e4", "d4", "Nf3", "c4", "g3", "b3", "f4", "Nc3", "b4", "g4"],  # White
        ["c5", "e5", "e6", "c6", "Nf6", "d5", "g6", "d6", "b6", "f5"]   # Black
    ],
    [  # Second Ply
        ["Nf3", "Nc3", "Bc4", "Bb5", "d3", "g3", "h3", "c3", "a3", "f4"],  # White
        ["Nc6", "Nf6", "d6", "g6", "e6", "Bb4", "Be7", "a6", "h6", "d5"]   # Black
    ],
    [  # Third Ply
        ["Bb5", "Bc4", "c3", "d3", "g3", "Nc3", "O-O", "Re1", "a3", "h3"],  # White
        ["a6", "Be7", "Nc6", "g6", "d5", "Bb4", "O-O", "Re8", "cxd4", "h6"]  # Black
    ]
]


phase_names = ["First", "Second", "Third"]
color_names = ["WHITE", "BLACK"]

print("static constexpr Move opening_moves_encoded[3][2][10] = {")
for i, phase in enumerate(opening_moves):
    if i < len(opening_moves):
        print(f"    // {phase_names[i]} move options")
    print("    {")
    for j, side in enumerate(phase):
        print("        {", end="")
        vals = []
        for mv in side:
            code = encode(mv, j)
            vals.append(f"0x{code:04X}")
        print(", ".join(vals), "},")
        
        # Print formatted comment with moves aligned
        print("        //", end="")
        for k, mv in enumerate(side):
            if k == 0:
                print(f'  "{mv}"', end="")
            else:
                print(f',   "{mv}"', end="")
        print(f"     // {color_names[j]}")
        
        if j == 0:  # Add blank line between WHITE and BLACK
            print()
    print("    },")
print("};")