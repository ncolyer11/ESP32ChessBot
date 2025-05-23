# PowerShell script to generate ChessEngine project structure

$root = "ChessEngine"
$files = @(
    "ChessEngine.ino",
    "board.h", "board.cpp",
    "movegen.h", "movegen.cpp",
    "search.h", "search.cpp",
    "eval.h", "eval.cpp",
    "opening.h", "opening.cpp",
    "transposition.h", "transposition.cpp",
    "util.h", "util.cpp"
)

# Create project directory
New-Item -ItemType Directory -Path $root -Force | Out-Null

# Create each file
foreach ($file in $files) {
    $path = Join-Path $root $file
    New-Item -ItemType File -Path $path -Force | Out-Null
}
