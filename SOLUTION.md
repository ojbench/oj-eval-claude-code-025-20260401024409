# Solution Documentation

## Overview
This is a complete implementation of a plagiarism detection and code transformation system for a custom S-expression-based programming language.

## Components

### 1. Parser (common.h)
- Parses S-expressions (LISP-like syntax)
- Handles atoms (identifiers, numbers) and lists
- Supports reading programs with "endprogram" delimiter

### 2. Code Transformer (Cheat Mode)
- **Purpose**: Transform source code to evade plagiarism detection while preserving functionality
- **Strategy**:
  - Rename all identifiers (variables, functions) to obfuscated names
  - Use format: prefix_counter_random (e.g., f_1_767, v_2_323)
  - Preserve language keywords and operators
  - Maintain program semantics
- **Key Features**:
  - Handles function parameters with proper scoping
  - Correctly renames recursive function calls
  - Preserves function names globally across scopes

### 3. Similarity Checker (Anticheat Mode)
- **Purpose**: Detect plagiarism between two programs
- **Strategy**:
  - Normalize AST by replacing identifiers with generic "ID" tokens
  - Compare structural similarity (tree structure)
  - Extract and compare features (operators, keywords, list sizes)
  - Compute weighted similarity score (0.0 to 1.0)
- **Scoring**:
  - 50% normalized structural similarity
  - 30% feature-based similarity (cosine similarity of feature vectors)
  - 20% size similarity

### 4. Auto-Detection
- Reads first program
- Attempts to read second program
- If second program is empty: **Cheat Mode** (transform single program)
- If second program exists: **Anticheat Mode** (compare two programs)

## Build Instructions
```bash
make clean
make
```

This produces a single executable: `code`

## Usage

### Cheat Mode (Transform Code)
```bash
./code < input_program.p > output_program.p
```
Input ends with "endprogram"

### Anticheat Mode (Detect Plagiarism)
```bash
./code < two_programs.txt
```
Input format:
```
(program 1 code...)
endprogram
(program 2 code...)
endprogram
(optional test inputs...)
```

Output: Similarity score between 0.0 and 1.0

## Testing

Run `./test.sh` to execute comprehensive tests:
1. Basic code transformation
2. Similarity detection for identical programs (expect ~1.0)
3. Similarity detection for different programs (expect <0.9)
4. Round-trip test (original vs transformed, expect ~1.0)

## Implementation Notes

- Language keywords preserved: function, block, set, print, if, while, array operations, operators
- Numbers are preserved as-is
- Identifiers get randomized names with format: {prefix}_{counter}_{random}
- Function scope handling ensures parameters are local but function names are global
- Recursive calls are correctly renamed

## Known Limitations

- Network connectivity issues prevented GitHub push and ACMOJ submission
- ACMOJ problem 1939 has `languages_accepted: []` (API configuration issue)
- Code is fully functional and tested locally

## File Structure
```
/workspace/problem_025/
├── common.h           # S-expression parser
├── main.cpp           # Transformer and similarity checker
├── cheat.cpp          # Standalone cheat implementation (not used)
├── anticheat.cpp      # Standalone anticheat implementation (not used)
├── Makefile           # Build system
├── test.sh            # Comprehensive test suite
├── .gitignore         # Git ignore rules
└── README.md          # Problem description
```

## Test Results

All local tests pass successfully:
- ✅ Compilation successful
- ✅ Fibonacci transformation works correctly
- ✅ Identical programs detected with similarity 1.0
- ✅ Different programs show lower similarity (~0.79)
- ✅ Original and transformed programs detected as highly similar (1.0)

## Next Steps

Once network connectivity is restored:
1. Push code to GitHub
2. Submit to ACMOJ (after problem configuration is fixed)
3. Iterate based on OJ feedback
