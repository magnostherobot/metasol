#include <cstdint>

#include <vector>
#include <string>

#include <stdio.h>

#include "game/move.h"
#include "game/search-state/game_state.h"
#include "solver/solver.h"

enum finished_state {
    SOLUTION_FOUND, NO_SOLUTION, TIMEOUT, SOLVER_OOM, CANCELLED
};

finished_state get_moves(std::vector<move>& moves, game_state& gs,
        uint64_t cache_capacity, uint64_t timeout);
finished_state get_moves(std::vector<move>& moves, game_state& gs,
        uint64_t cache_capacity, uint64_t timeout) {
    moves.clear();

    solver sol(gs, cache_capacity);
    solver::result res = sol.run(std::chrono::milliseconds(timeout));
    if (res.sol_type == solver::result::type::SOLVED) {
        std::vector<solver::node> nodes = sol.get_frontier();
        for (auto &it : nodes) {
            moves.push_back(it.mv);
        }
    }

    switch (res.sol_type) {
        case solver::result::type::SOLVED:
            return SOLUTION_FOUND;
        case solver::result::type::UNSOLVABLE:
            return NO_SOLUTION;
        case solver::result::type::TIMEOUT:
            return TIMEOUT;
        case solver::result::type::MEM_LIMIT:
            return SOLVER_OOM;
        case solver::result::type::TERMINATED:
            return CANCELLED;
        default:
            assert(false);
            return NO_SOLUTION;
    }
}
