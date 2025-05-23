import requests
import chess
import json

def get_popular_moves(fen, limit=10):
    """
    Fetches the most popular moves for a given FEN position from the Lichess Opening Explorer.

    Args:
        fen (str): The FEN string of the current chess position.
        limit (int): The maximum number of moves to return.

    Returns:
        list: A list of dictionaries, where each dictionary contains 'move' (in UCI format)
              and 'popularity' (number of games). Returns an empty list if no moves found.
    """
    url = "https://explorer.lichess.ovh/lichess"
    params = {
        "fen": fen,
        "variant": "standard",
        "moves": limit
    }
    try:
        response = requests.get(url, params=params)
        response.raise_for_status()
        data = response.json()
        
        moves_data = []
        if 'moves' in data:
            for move_info in data['moves']:
                popularity = move_info.get('white', 0) + move_info.get('draws', 0) + move_info.get('black', 0)
                moves_data.append({
                    'uci': move_info['uci'],
                    'san': move_info['san'],
                    'games': popularity
                })
            moves_data.sort(key=lambda x: x['games'], reverse=True)
            return moves_data[:limit]
        else:
            return []
    except requests.exceptions.RequestException as e:
        print(f"Error fetching data: {e}")
        return []

def main():
    board = chess.Board()
    num_plies = 10
    top_n_moves = 10 # Let's use 10 for the example output you provided, but you can change it to 3

    # This will hold the final structured data for all plies
    opening_moves_data = [] 

    for ply in range(1, num_plies + 1):
        white_moves_at_ply = []
        black_moves_at_ply = []

        current_fen = board.fen()
        popular_moves = get_popular_moves(current_fen, limit=top_n_moves)

        if not popular_moves:
            # If no moves are found, we'll append empty lists for this ply
            # and break the loop.
            opening_moves_data.append([white_moves_at_ply, black_moves_at_ply])
            break

        if board.turn == chess.WHITE:
            # For White's turn, the popular_moves are White's options.
            for move_info in popular_moves:
                try:
                    move_san = board.san(chess.Move.from_uci(move_info['uci']))
                    white_moves_at_ply.append(move_san)
                except ValueError:
                    white_moves_at_ply.append(move_info['uci']) # Fallback if SAN conversion fails
            
            # For a White ply, Black's response moves will be filled in the next iteration.
            # We add a placeholder empty list for Black's moves for this ply for now.
            opening_moves_data.append([white_moves_at_ply, black_moves_at_ply])

        else: # board.turn == chess.BLACK
            # For Black's turn, the popular_moves are Black's responses to the previous move.
            for move_info in popular_moves:
                try:
                    move_san = board.san(chess.Move.from_uci(move_info['uci']))
                    black_moves_at_ply.append(move_san)
                except ValueError:
                    black_moves_at_ply.append(move_info['uci']) # Fallback
            
            # These Black moves belong to the *previous* ply's entry.
            # So, we update the second element (Black's moves) of the last ply added.
            if opening_moves_data:
                opening_moves_data[-1][1] = black_moves_at_ply
            else:
                # This case should ideally not happen if ply > 1 and data is consistent,
                # but it's a fallback if something goes wrong.
                opening_moves_data.append([white_moves_at_ply, black_moves_at_ply])

        # Always make the top move to advance the board for the next ply's calculation,
        # unless it's the last ply.
        if popular_moves and ply < num_plies:
            best_move_uci = popular_moves[0]['uci']
            move = chess.Move.from_uci(best_move_uci)
            board.push(move)
        elif not popular_moves and ply < num_plies:
            break # Stop if no more moves found before reaching max plies

    # --- Print the result in the beautifully formatted way ---
    print("opening_moves = [")
    for i, ply_data in enumerate(opening_moves_data):
        ply_number_name = ""
        if i == 0:
            ply_number_name = "First Ply"
        elif i == 1:
            ply_number_name = "Second Ply"
        elif i == 2:
            ply_number_name = "Third Ply"
        else:
            ply_number_name = f"{i+1}th Ply"

        print(f"    [  # {ply_number_name}")
        print(f"        {json.dumps(ply_data[0])},  # White") # Format White's moves as a JSON string
        print(f"        {json.dumps(ply_data[1])}   # Black")  # Format Black's moves as a JSON string
        print("    ],")
    print("]")

if __name__ == "__main__":
    main()