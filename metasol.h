#ifndef METASOL_H
#define METASOL_H

#include <vector>

#include <stdint.h>

enum build_policy_t {
    NO_BUILD, BUILD_SAME_SUIT, BUILD_ALTERNATING, BUILD_ANY
};

enum spaces_policy_t {
    NO_SPACE_FILL, KINGS_FILL_SPACE, ANY_FILL_SPACE, AUTO_RESERVE_THEN_WASTE,
    AUTO_WASTE_THEN_STOCK
};

enum accordion_policy_t {
    ACCORDION_SAME_RANK, ACCORDION_SAME_SUIT, ACCORDION_ALTERNATE_COLOUR,
    ACCORDION_ANY_SUIT
};

enum stock_deal_t {
    STOCK_TO_WASTE, STOCK_TO_TABLEAU
};

enum face_up_policy_t {
    ALL_CARDS_FACE_UP, TOP_CARDS_FACE_UP
};

enum direction_t {
    LEFT, RIGHT, BOTH
};

enum built_group_t {
    CAN_MOVE_BUILT_GROUP, CANNOT_MOVE_BUILT_GROUP, CAN_MOVE_WHOLE_PILE,
    CAN_MOVE_MAXIMAL_GROUP
};

enum foundations_init_t {
    NO_FOUNDATION_INIT, ONE_FOUNDATION_INIT, ALL_FOUNDATIONS_INIT
};

enum card_suit { HEARTS, SPADES, CLUBS, DIAMONDS };
extern card_suit SUITS[4];

typedef uint8_t card_rank;

typedef struct ms_rules {
    unsigned tableau_size;
    build_policy_t build_policy;
    spaces_policy_t spaces_policy;
    built_group_t move_built_group;
    build_policy_t built_group_policy;
    unsigned deck_count;
    unsigned max_rank;
    bool hole_present;
    bool foundations_present;
    foundations_init_t foundations_init_cards;
    bool specific_foundations_base;
    card_rank foundations_base;
    bool foundations_removable;
    bool foundations_only_comp_piles;
    bool diagonal_deal;
    unsigned cells;
    unsigned cells_pre_filled;
    unsigned stock_size;
    stock_deal_t stock_deal_method;
    unsigned stock_deal_count;
    bool stock_redeal;
    unsigned reserve_size;
    bool reserve_stacked;
    face_up_policy_t face_up_policy;
    unsigned sequence_count;
    direction_t sequence_direction;
    build_policy_t sequence_build_policy;
    bool sequence_fixed_suit;
    unsigned accordion_size;
    std::vector<std::pair<direction_t, int>> accordion_moves;
    std::vector<accordion_policy_t> accordion_policy;
} ms_rules;

typedef struct ms_card {
    card_suit suit;
    card_rank rank;
    bool hidden;
} ms_card;

typedef std::vector<ms_card> ms_card_pile;

typedef struct ms_game_state {
    std::vector<ms_card_pile> foundations;
    ms_card_pile stock;
    ms_card_pile waste;
    std::vector<ms_card_pile> tableau;
    ms_card_pile hole;
    std::vector<ms_card_pile> cells;
} ms_game_state;

typedef struct ms_move {
    bool stock;
    ms_card_pile *to;
    ms_card_pile *from;
} ms_move;

enum finished_state {
    SOLUTION_FOUND, NO_SOLUTION, TIMEOUT, SOLVER_OOM, CANCELLED
};

typedef finished_state thoughtful_run_func_t(ms_game_state *,
        ms_rules *, void *);
typedef finished_state run_func_t(ms_game_state *, ms_rules *, ms_move *,
        unsigned, void *);
typedef bool solved_func_t(ms_game_state *, ms_rules *, void *);

typedef struct ms_settings {
    run_func_t *run_func;
    thoughtful_run_func_t *thoughtful_run_func;
    solved_func_t *solved_func;
    unsigned reserved_move_count;
    void *user_data;
} ms_settings;

unsigned foundation_count(ms_rules *r);
unsigned total_pile_count(ms_rules *r);

ms_card_pile *get_pile_by_index(ms_game_state *gs, uint8_t i);

struct thread_info {
    ms_game_state *gs;
    ms_rules *r;
    ms_settings *s;
    ms_move *move_buf;
    finished_state *result;
};

const int MAX_VOTES = 100;
const int MIN_VOTES = 50;

ms_rules fetch_default_rules();
ms_settings fetch_default_settings();

ms_game_state random_game_state(ms_rules *, ms_settings *);

void ms_run(ms_game_state *, ms_rules *, ms_settings *);

#endif /* METASOL_H */
