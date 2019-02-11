#ifndef METASOL_H
#define METASOL_H

#include <vector>

#include <stdint.h>
#include <string.h>

/*
 * This all looks very ugly, but saves a lot of repetition of enum values.
 * The suite and C macros are inspired by
 * http://www.purl.org/stefan_ram/pub/c_preprocessor_applications_en .
 * Essentially, the suite macros expand to the declarations of an enum and two
 * functions: one string-to-enum, the other enum-to-string. The C macro can be
 * used later with a _gen macro to initialise a static string array filled with
 * the contents of an enum.
 *
 * If this is too messy for a header file to an API, the preprocessor output can
 * be used to get a fairly clean preprocessed version of what's below.
 */
#define _suite(x, _t, _gen, str_, _str, _count) \
    enum x##_t { x##_t##_gen x##_t##_count }; \
    x##_t str_##x(char *); \
    const char *x##_str(x##_t)
#define suite(x) _suite(x, _t, _gen, str_, _str, _count)

#define C(x) x,

#define build_policy_t_gen \
    C(NO_BUILD)\
    C(BUILD_SAME_SUIT)\
    C(BUILD_ALTERNATING)\
    C(BUILD_ANY)
suite(build_policy);

#define spaces_policy_t_gen \
    C(NO_SPACE_FILL)\
    C(KINGS_FILL_SPACE)\
    C(ANY_FILL_SPACE)\
    C(AUTO_RESERVE_THEN_WASTE)\
    C(AUTO_WASTE_THEN_STOCK)
suite(spaces_policy);

#define built_group_t_gen \
    C(CAN_MOVE_BUILT_GROUP)\
    C(CANNOT_MOVE_BUILT_GROUP)\
    C(CAN_MOVE_WHOLE_PILE)\
    C(CAN_MOVE_MAXIMAL_GROUP)
suite(built_group);

enum {
    KINGS_FILL_SPACES = KINGS_FILL_SPACE,
    ANY_FILL_SPACES = ANY_FILL_SPACE
};

#define accordion_policy_t_gen \
    C(ACCORDION_SAME_RANK)\
    C(ACCORDION_SAME_SUIT)\
    C(ACCORDION_ALTERNATE_COLOUR)\
    C(ACCORDION_ANY_SUIT)
suite(accordion_policy);

#define stock_deal_t_gen \
    C(STOCK_TO_WASTE)\
    C(STOCK_TO_TABLEAU)
suite(stock_deal);

#define face_up_policy_t_gen \
    C(ALL_CARDS_FACE_UP)\
    C(TOP_CARDS_FACE_UP)
suite(face_up_policy);

#define direction_t_gen \
    C(LEFT)\
    C(RIGHT)\
    C(BOTH)
suite(direction);

#define foundations_init_t_gen \
    C(NO_FOUNDATION_INIT)\
    C(ONE_FOUNDATION_INIT)\
    C(ALL_FOUNDATIONS_INIT)
suite(foundations_init);

enum {
    NO_FOUNDATIONS_INIT = NO_FOUNDATION_INIT,
};

#undef C

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

ms_card ms_str_card(char *, bool);

typedef struct ms_game_state {
    std::vector<ms_card_pile> foundations;
    ms_card_pile stock;
    ms_card_pile waste;
    std::vector<ms_card_pile> tableau;
    ms_card_pile hole;
    std::vector<ms_card_pile> cells;
    ms_card_pile reserve;
} ms_game_state;

typedef struct ms_move {
    bool stock;
    unsigned to;
    unsigned from;
    unsigned size;
} ms_move;

enum finished_state {
    SOLUTION_FOUND, NO_SOLUTION, TIMEOUT, SOLVER_OOM, CANCELLED
};

typedef struct {
    finished_state result;
    std::vector<ms_move> moves;
} vote;

typedef finished_state thoughtful_run_func_t(ms_game_state *,
        ms_rules *, void *);
typedef finished_state run_func_t(ms_game_state *, ms_rules *,
        std::vector<ms_move> *, unsigned, void *);
typedef bool solved_func_t(ms_game_state *, ms_rules *, void *);

typedef struct ms_settings {
    run_func_t *run_func;
    thoughtful_run_func_t *thoughtful_run_func;
    solved_func_t *solved_func;
    unsigned reserved_move_count;
    void *user_data;
    unsigned max_concurrent_threads;
    unsigned max_votes;
} ms_settings;

unsigned foundation_count(ms_rules *r);
unsigned total_pile_count(ms_rules *r);

ms_card_pile *get_pile_by_index(ms_game_state *gs, uint8_t i);

typedef struct {
    ms_game_state *gs;
    ms_rules *r;
    ms_settings *s;
    std::vector<ms_move> *move_buf;
    finished_state *result;
} thread_info;

ms_rules fetch_default_rules();
ms_settings fetch_default_settings();

ms_game_state random_game_state(long, ms_rules *);

int ms_run(ms_game_state *, ms_rules *, ms_settings *);

#endif /* METASOL_H */
