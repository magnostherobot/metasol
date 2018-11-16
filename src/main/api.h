#include <cstdint>
#include <vector>
#include <string>
#include <list>

#include "game/move.h"
#include "game/sol_rules.h"

// just for now, until I construct my own rules stuff:
#include "input-output/input/json-parsing/rules_parser.h"

typedef std::vector<std::vector<std::string>> piles;
typedef std::vector<move> movelist;

class game_state {
    public:
    game_state(const sol_rules&, piles);
    enum class streamliner_options {NONE, AUTO_FOUNDATIONS, SUIT_SYMMETRY, BOTH};
    void make_move(move);

    friend std::ostream& operator<< (std::ostream&, const game_state&);

    private:
    /* Game rules */

    const sol_rules rules;
    streamliner_options stream_opts;
    card::rank_t foundations_base;

    /* Pile references */

    std::list<pile::ref> tableau_piles;
    std::list<pile::ref> cells;
    pile::ref stock;
    pile::ref waste;
    std::list<pile::ref> reserve;
    std::vector<pile::ref> foundations;
    std::vector<pile::ref> sequences;
    std::list<pile::ref> accordion;
    pile::ref hole;

    /* Pile references of starting/original layout */

    std::vector<pile::ref> original_tableau_piles;
    std::vector<pile::ref> original_cells;
    std::vector<pile::ref> original_reserve;

    /* Auto-foundation moves state */

    std::vector<bool> auto_foundation_moves;

    /* Core piles */

    std::vector<pile> piles;
};

/**
 * Populate a move list with a bunch of moves that lead to a solution.
 *
 * Clears the list, then, if a solution is found, populates it with the moves
 * required to realise the solution.
 *
 * Returns true if a solution was found; false otherwise.
 */
bool get_moves(movelist& moves, game_state& gs, uint64_t cache_capacity,
        uint64_t timeout);
