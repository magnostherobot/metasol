#include <cstdint>
#include <vector>
#include <string>
#include <list>

#include "game/card.h"
#include "game/move.h"
#include "game/sol_rules.h"

#ifndef SOLVITAIRE_SOLVER_H
namespace solver {
    namespace result {
        enum class type {
            TIMEOUT, SOLVED, UNSOLVABLE, MEM_LIMIT, TERMINATED
        };
    }
}
#endif

#ifndef SOLVITAIRE_GAME_STATE_H
class game_state {
    friend class hasher;
    friend class global_cache;
    friend class cached_game_state;
    friend class deal_parser;
    friend class state_printer;
    friend class test_helper;
    friend class json_helper;
public:
    enum class streamliner_options {NONE, AUTO_FOUNDATIONS, SUIT_SYMMETRY, BOTH};

    /* Constructors */

    // Creates a game state representation from a JSON doc
    game_state(const sol_rules&, const rapidjson::Document&, streamliner_options);
    // Does the same from a seed
    game_state(const sol_rules&, int seed, streamliner_options);
    // Does the same but with an initialiser list (useful for testing)
    game_state(const sol_rules&, std::initializer_list<std::initializer_list<std::string>>);
    // Does the same but with vectors
    game_state(const sol_rules&, std::vector<std::vector<std::string>>);
    // Does the same but with lots of vectors
    game_state(const sol_rules&, std::vector<std::vector<card::suit_t>>,
               std::vector<std::vector<card::rank_t>>,
               std::vector<std::vector<bool>>);

    /* Altering state */

    void make_move(move);
    void undo_move(move);
    void place_card(pile::ref, card);
    card take_card(pile::ref);

    /* Legal move generation */

    std::vector<move> get_legal_moves(move = move(move::mtype::regular));
    boost::optional<move> get_dominance_move() const;

    /* State inspection */

    bool is_solved() const;
    const std::vector<pile>& get_data() const;

    /* Printing */

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
#endif

typedef std::vector<std::vector<std::string>> piles;
typedef std::vector<move> movelist;

/**
 * Populate a move list with a bunch of moves that lead to a solution.
 *
 * Clears the list, then, if a solution is found, populates it with the moves
 * required to realise the solution.
 *
 * Returns true if a solution was found; false otherwise.
 */
solver::result::type get_moves(movelist& moves, game_state& gs,
        uint64_t cache_capacity, uint64_t timeout);

