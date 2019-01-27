#include <cstdint>
#include <vector>
#include <string>

#include "game/search-state/game_state.h"
#include "solver/solver.h"

#include "api.h"

solver::result::type get_moves(std::vector<move>& moves, game_state& gs,
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

    return res.sol_type;
}
