#include <iostream>
#include <map>
#include <algorithm>

#include <err.h>

#include "solvitaire/headers/api.h"
#include "docopt/docopt.h"
#include "jsmn/jsmn.h"

#include "metasol.hpp"
#include "debug.h"

typedef struct {
    unsigned run_cache_size;
    uint64_t run_timeout;
    uint64_t run_node_limit;
    const char *deal_seed_file;

    sol_rules::build_policy build_policy;
    sol_rules::spaces_policy spaces_policy;
    sol_rules::built_group_type move_built_group;
    sol_rules::build_policy built_group_policy;
    bool hole_build_loops;
    bool foundations_removable;
    bool foundations_only_comp_piles;
    sol_rules::direction sequence_direction;
    sol_rules::build_policy sequence_build_policy;
    bool sequence_fixed_suit;
    std::vector<std::pair<sol_rules::direction, uint8_t>> accordion_moves;
    std::vector<sol_rules::accordion_policy> accordion_policy;
} user_data;

static const char USAGE[] =
R"(centre - a metasol instance.

Usage: centre <rules_file> [options]
       centre --help|-h

Options:
  --dealseed SEED          Specify a seed to use when randomly generating a game
                           (set to 0 to use an unspecified seed).
  --dealseedfile FILE      Specify a list of seeds to use when randomly
                           generating games.
  -h --help                Display this help.
  -f --forever             Run forever, on as many games as possible.
  -g FILE --game FILE      Specify a file describing a game-state.
  --limitnodes NODES       Specify a limit to the number of nodes traversible by
                           a single run of Solvitaire (supercedes --limittime).
  --limittime MSECONDS     Specify a time limit used for each individual run of
                           Solvitaire, in milliseconds (superceded by
                           --limitnodes).
  --solveseed SEED         Specify a seed for the solver to use (set to 0 to use
                           an unspecified seed).
  -s FILE --settings FILE  Specify a settings file.
  -v --version             Display version.)";

static const char VERSION_STR[] = "Solver in-development";

inline void lower(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

struct dictcmp {
    bool operator()(const char *a, const char *b) const {
        return strcmp(a, b) < 0;
    }
};

#define kv_dict(t) std::map<const char *, t, dictcmp>

sol_rules::build_policy str_build_policy(char *str) {
    kv_dict(sol_rules::build_policy) dict = {
        { "no build", sol_rules::build_policy::NO_BUILD },
        { "no-build", sol_rules::build_policy::NO_BUILD },
        { "none", sol_rules::build_policy::NO_BUILD },

        { "same suit", sol_rules::build_policy::SAME_SUIT },
        { "same-suit", sol_rules::build_policy::SAME_SUIT },
        { "suits", sol_rules::build_policy::SAME_SUIT },

        { "alternating", sol_rules::build_policy::RED_BLACK },
        { "red-black", sol_rules::build_policy::RED_BLACK },

        { "any", sol_rules::build_policy::ANY_SUIT },
        { "any-suit", sol_rules::build_policy::ANY_SUIT },
        { "all", sol_rules::build_policy::ANY_SUIT }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

sol_rules::stock_deal_type convert_stock_deal(stock_deal_t sd) {
    switch (sd) {
        case STOCK_TO_WASTE:   return sol_rules::stock_deal_type::WASTE;
        case STOCK_TO_TABLEAU: return sol_rules::stock_deal_type::TABLEAU_PILES;
        case STOCK_TO_HOLE:    return sol_rules::stock_deal_type::HOLE;
        default: assert(false);
    }
}

sol_rules::face_up_policy convert_face_up_policy(face_up_policy_t fup) {
    switch (fup) {
        case ALL_CARDS_FACE_UP: return sol_rules::face_up_policy::ALL;
        case TOP_CARDS_FACE_UP: return sol_rules::face_up_policy::TOP_CARDS;
        default: assert(false);
    }
}

sol_rules::foundations_init_type convert_foundations_init(
        foundations_init_t fi) {
    switch (fi) {
        case NO_FOUNDATION_INIT:
            return sol_rules::foundations_init_type::NONE;
        case ONE_FOUNDATION_INIT:
            return sol_rules::foundations_init_type::ONE;
        case ALL_FOUNDATIONS_INIT:
            return sol_rules::foundations_init_type::ALL;
        default: assert(false);
    }
}

sol_rules::spaces_policy str_spaces_policy(char *str) {
    kv_dict(sol_rules::spaces_policy) dict = {
        { "none", sol_rules::spaces_policy::NO_BUILD },
        { "no fill", sol_rules::spaces_policy::NO_BUILD },
        { "no-fill", sol_rules::spaces_policy::NO_BUILD },
        { "no_fill", sol_rules::spaces_policy::NO_BUILD },

        { "kings only", sol_rules::spaces_policy::KINGS },
        { "kings-only", sol_rules::spaces_policy::KINGS },
        { "kings_only", sol_rules::spaces_policy::KINGS },
        { "kings", sol_rules::spaces_policy::KINGS },

        { "any", sol_rules::spaces_policy::ANY },
        { "all", sol_rules::spaces_policy::ANY },

        { "reserve first", sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE },
        { "reserve-first", sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE },
        { "reserve_first", sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE },
        { "reserve then waste",
            sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE },
        { "reserve-then-waste",
            sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE },
        { "reserve_then_waste",
            sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE },

        { "waste first", sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK },
        { "waste-first", sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK },
        { "waste_first", sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK },
        { "waste then stock", sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK },
        { "waste-then-stock", sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK },
        { "waste_then_stock", sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

sol_rules::direction str_direction(char *str) {
    kv_dict(sol_rules::direction) dict = {
        { "left", sol_rules::direction::LEFT },

        { "right", sol_rules::direction::RIGHT },

        { "both", sol_rules::direction::BOTH },
        { "lr", sol_rules::direction::BOTH }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

sol_rules::built_group_type str_built_group(char *str) {
    kv_dict(sol_rules::built_group_type) dict = {
        { "yes", sol_rules::built_group_type::YES },

        { "no", sol_rules::built_group_type::NO },
        { "none", sol_rules::built_group_type::NO },

        { "whole pile only", sol_rules::built_group_type::WHOLE_PILE },
        { "whole pile", sol_rules::built_group_type::WHOLE_PILE },

        { "maximal group only", sol_rules::built_group_type::MAXIMAL_GROUP },
        { "maximal", sol_rules::built_group_type::MAXIMAL_GROUP }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

sol_rules::accordion_policy str_accordion_policy(char *str) {
    kv_dict(sol_rules::accordion_policy) dict = {
        { "same rank", sol_rules::accordion_policy::SAME_RANK },
        { "same-rank", sol_rules::accordion_policy::SAME_RANK },
        { "same_rank", sol_rules::accordion_policy::SAME_RANK },
        { "rank", sol_rules::accordion_policy::SAME_RANK },

        { "same suit", sol_rules::accordion_policy::SAME_SUIT },
        { "same-suit", sol_rules::accordion_policy::SAME_SUIT },
        { "same_suit", sol_rules::accordion_policy::SAME_SUIT },
        { "suit", sol_rules::accordion_policy::SAME_SUIT },

        { "alternating colour",
            sol_rules::accordion_policy::RED_BLACK },
        { "alternating-colour",
            sol_rules::accordion_policy::RED_BLACK },
        { "alternating_colour",
            sol_rules::accordion_policy::RED_BLACK },
        { "red-black", sol_rules::accordion_policy::RED_BLACK
        },

        { "any", sol_rules::accordion_policy::ANY_SUIT }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

sol_rules convert_rules(ms_rules *sr, user_data *d) {
    sol_rules r;

    r.tableau_pile_count = sr->tableau_size;
    r.build_pol = d->build_policy;
    r.spaces_pol = d->spaces_policy;
    r.move_built_group = d->move_built_group;
    r.built_group_pol = d->built_group_policy;
    r.max_rank = sr->max_rank;
    r.hole = sr->hole_present;
    r.hole_build_loops = d->hole_build_loops;
    r.foundations_present = sr->foundations_present;
    r.foundations_init_cards =
        convert_foundations_init(sr->foundations_init_cards);
    r.foundations_removable = d->foundations_removable;
    r.foundations_only_comp_piles = d->foundations_only_comp_piles;
    r.diagonal_deal = sr->diagonal_deal;
    r.cells = sr->cells;
    r.cells_pre_filled = sr->cells_pre_filled;
    r.stock_size = sr->stock_size;
    r.stock_deal_t = convert_stock_deal(sr->stock_deal_method);
    r.stock_deal_count = sr->stock_deal_count;
    r.stock_redeal = sr->stock_redeal;
    r.reserve_size = sr->reserve_size;
    r.reserve_stacked = sr->reserve_stacked;
    r.face_up = convert_face_up_policy(sr->face_up_policy);
    r.sequence_count = sr->sequence_count;
    r.sequence_direction = d->sequence_direction;
    r.sequence_build_pol = d->sequence_build_policy;
    r.sequence_fixed_suit = d->sequence_fixed_suit;
    r.accordion_size = sr->accordion_size;

    assert(sr->deck_count <= 2);
    r.two_decks = sr->deck_count == 2;

    if (sr->specific_foundations_base) {
        r.foundations_base = sr->foundations_base;
    } else {
        r.foundations_base = boost::none;
    }

    for (auto &p : d->accordion_moves) {
        r.accordion_moves.push_back(std::pair<sol_rules::direction, uint8_t>
                (p.first, p.second));
    }

    for (auto &x : d->accordion_policy) {
        r.accordion_pol.push_back(x);
    }

    return r;
}

int split(ms_card_pile &in, std::vector<ms_card_pile> &out,
        std::vector<unsigned> numbers) {
    out.clear();

    unsigned acc = 0;
    for (auto &n : numbers) {
        ms_card_pile cp(in.begin() + acc, in.begin() + acc + n);
        out.push_back(cp);
        acc += n;
    }

    assert(acc == in.size());
    return 0;
}

int split_even(ms_card_pile &in, std::vector<ms_card_pile> &out,
        unsigned n_piles) {
    std::vector<unsigned> nums;

    auto t = in.size();
    for (int i = 0; i < (int)in.size(); ++i) {
        auto w = t / (n_piles - i);
        nums.push_back(w);
        t -= w;
    }

    return split(in, out, nums);
}

char const *mtype_str(move::mtype t) {
    switch (t) {
        case move::mtype::regular:              return "regular";
        case move::mtype::built_group:          return "built group";
        case move::mtype::stock_k_plus:         return "stock k+";
        case move::mtype::stock_to_all_tableau: return "stock-tableau";
        case move::mtype::sequence:             return "sequence";
        case move::mtype::accordion:            return "accordion";
        case move::mtype::null:                 return "null(!)";
        default: assert(false);
    }
}

card::suit_t convert_suit(card_suit s) {
    switch (s) {
        case HEARTS:   return card::suit::Hearts;
        case SPADES:   return card::suit::Spades;
        case CLUBS:    return card::suit::Clubs;
        case DIAMONDS: return card::suit::Diamonds;
        default: assert(false);
    }
}

card::rank_t convert_rank(card_rank r) {
    return r;
}

void convert_game_state_pile(ms_card_pile *p,
        std::vector<std::vector<card::suit_t>> *suits,
        std::vector<std::vector<card::rank_t>> *ranks,
        std::vector<std::vector<bool>> *face_down,
        bool force_face_up = false) {
    suits->push_back({});
    ranks->push_back({});
    face_down->push_back({});

    for (auto &c : *p) {
        suits->back().push_back(convert_suit(c.suit));
        ranks->back().push_back(convert_rank(c.rank));
        face_down->back().push_back(force_face_up ? false : c.hidden);
    }
}

void convert_game_state_pile_set(std::vector<ms_card_pile> *set,
        std::vector<std::vector<card::suit_t>> *suits,
        std::vector<std::vector<card::rank_t>> *ranks,
        std::vector<std::vector<bool>> *face_down,
        bool force_face_up = false) {
    for (auto &p : *set) {
        convert_game_state_pile(&p, suits, ranks, face_down, force_face_up);
    }
}

/*
 * game_state pile creation order:
 * 1. hole
 * 2. 4 foundations (8 for two decks)
 * 3. cell piles
 * 4. stock
 * 5. waste
 * 6. reserve (maybe multiple?)
 * 7. accordion piles
 * 8. tableau piles
 * 9. sequence piles
 */

game_state convert_game_state(ms_game_state *sgs, ms_rules *r, user_data *d) {
    std::vector<std::vector<card::suit_t>> suits;
    std::vector<std::vector<card::rank_t>> ranks;
    std::vector<std::vector<bool>> face_down;

    if (r->hole_present) {
        convert_game_state_pile(&sgs->hole, &suits, &ranks, &face_down);
    }

    convert_game_state_pile_set(&sgs->foundations, &suits, &ranks, &face_down);
    convert_game_state_pile_set(&sgs->cells, &suits, &ranks, &face_down);
    if (r->stock_size) {
        // solvitaire requires that the stock be entirely face-up
        convert_game_state_pile(&sgs->stock, &suits, &ranks, &face_down, true);
        if (r->stock_deal_method == STOCK_TO_WASTE) {
            convert_game_state_pile(&sgs->waste, &suits, &ranks, &face_down);
        }
    }
    if (r->reserve_size) {
        convert_game_state_pile(&sgs->reserve, &suits, &ranks, &face_down);
    }
    // TODO: how does the accordion work? leaving empty for now
    convert_game_state_pile_set(&sgs->tableau, &suits, &ranks, &face_down);
    // TODO: how does the sequence work? leaving empty for now

    game_state gs(convert_rules(r, d), suits, ranks, face_down);
    return gs;
}

void print_sgs(ms_game_state *gs, ms_rules *r, user_data *d) {
    game_state solv_gs = convert_game_state(gs, r, d);
    std::cerr << solv_gs << std::endl;
}

bool is_dud_move(move *m) {
    if (m->to == 255u && m->from == 255u) return true;
    return false;
}

bool goes_through_stock(move *a) {
    return a->type == move::mtype::stock_k_plus
        && a->count != 0;
}

unsigned get_first_tableau_pile_index(ms_rules *r) {
    assert(r->tableau_size > 0);

    unsigned i = 0;
    if (r->hole_present) {
        ++i;
    }

    if (r->foundations_present) {
        i += 4 * r->deck_count;
    }

    i += r->cells;

    if (r->stock_size > 0) {
        i += 1;

        if (r->stock_deal_method == STOCK_TO_WASTE) {
            i += 1;
        }
    }

    return i;
}

unsigned get_stock_index(ms_rules *r) {
    assert(r->stock_size > 0);

    unsigned i = 0;
    if (r->hole_present) {
        ++i;
    }

    if (r->foundations_present) {
        i += 4 * r->deck_count;
    }

    i += r->cells;

    return i;
}

unsigned get_waste_index(ms_rules *r) {
    assert(r->stock_deal_method == STOCK_TO_WASTE);
    return get_stock_index(r) + 1;
}

int convert_move(ms_game_state *gs, ms_rules *r, move *m, ms_move *sm) {
    sm->stock = goes_through_stock(m);

    if (m->type == move::mtype::stock_k_plus) {
        sm->from = get_waste_index(r);

        // if the move is a k+ move, then this value only matters if it doesn't
        // go through the stock, in which case `m->count == 0` but we want it to
        // move one card from the waste.
        sm->size = 1;
    } else {
        sm->from = m->from;
        sm->size = m->count;
    }

    sm->to = m->to;

    return 0;
}

finished_state convert_result(solver::result::type r) {
    switch (r) {
        case solver::result::type::SOLVED:
            return SOLUTION_FOUND;
        case solver::result::type::UNSOLVABLE:
            return NO_SOLUTION;
        case solver::result::type::TIMEOUT:
        case solver::result::type::MEM_LIMIT:
        case solver::result::type::TERMINATED:
            return CANCELLED;
        default:
            assert(false);
    }
}

unsigned calculate_stock_moves(int n, unsigned *stock, unsigned *waste,
        unsigned deal) {
    unsigned stock_waste = *stock + *waste;

    unsigned moves = 0;
    while (n != 0) {
        if (*stock == 0) {
            *stock = stock_waste;
            *waste = 0;
        }

        if (n < 0) {
            n += stock_waste;
            continue;
        }

        unsigned m = std::min(deal, *stock);
        *stock -= m;
        *waste += m;
        n -= m;
        ++moves;
    }

    assert(*waste > 0);
    *waste -= 1u;

    return moves;
}

/**
 * The function that is called by every voter, to get its vote.
 */
finished_state run_single(ms_game_state *gs, ms_rules *r,
        std::vector<ms_move> *moves, unsigned move_count, void *data) {

    user_data *d = (user_data *) data;

    game_state solv_gs = convert_game_state(gs, r, d);

    movelist ml;

    finished_state result;
    if (d->run_node_limit) {
        result = convert_result(get_moves_node_limit(ml, solv_gs,
                    d->run_cache_size, d->run_node_limit));
    } else {
        result = convert_result(get_moves(ml, solv_gs, d->run_cache_size,
                    d->run_timeout));
    }

    if (result == SOLUTION_FOUND) {
        auto n = ml.size();
        if (move_count) {
            n = std::min((std::vector<move>::size_type) move_count, ml.size());
        }

        moves->clear();

        unsigned stock = gs->stock.size();
        unsigned waste = gs->waste.size();

        /*
         * FIXME: This section counts the number of converted moves rather than
         * the number of moves produced during converting. This shouldn't be a
         * problem (for now) but should be changed at some point.
         */
        for (unsigned i = 0; i < n; ++i) {
            move *m = &ml[i];
            if (is_dud_move(m)) {
                continue;
            }

            switch (m->type) {
                case move::mtype::regular:
                case move::mtype::built_group: {
                    ms_move rm;
                    rm.from = m->from;
                    rm.to = m->to;
                    rm.size = m->count;
                    rm.stock = false;

                    moves->push_back(rm);
                    break;
                } case move::mtype::stock_k_plus: {
                    int cards_moved = m->count;
                    unsigned stock_moves = calculate_stock_moves(cards_moved,
                            &stock, &waste, r->stock_deal_count);

                    for (unsigned j = 0; j < stock_moves; ++j) {
                        ms_move sm;
                        sm.stock = true;

                        moves->push_back(sm);
                    }

                    ms_move kpm;
                    kpm.from = get_waste_index(r);
                    kpm.to = m->to;
                    kpm.size = 1u;
                    kpm.stock = false;

                    moves->push_back(kpm);
                    break;
                } case move::mtype::stock_to_all_tableau: {
                    ms_move stm;
                    stm.stock = true;

                    moves->push_back(stm);
                    break;
                } default: {
                    errx(EXIT_FAILURE, "unimplmented move type %s",
                            mtype_str(m->type));
                }
            }
        }
    }

    std::reverse(moves->begin(), moves->end());

    return result;
}

finished_state thoughtful_run(ms_game_state *gs, ms_rules *r, void *data) {
    user_data *d = (user_data *) data;

    game_state solv_gs = convert_game_state(gs, r, d);
    movelist ml;
    finished_state result;
    if (d->run_node_limit) {
        result = convert_result(get_moves_node_limit(ml, solv_gs,
                    d->run_cache_size, d->run_node_limit));
    } else {
        result = convert_result(get_moves(ml, solv_gs, d->run_cache_size,
                    d->run_timeout));
    }

    return result;
}

bool solved(ms_game_state *gs, ms_rules *r, void *data) {
    user_data *d = (user_data *) data;

    print_sgs(gs, r, d);

    return convert_game_state(gs, r, d).is_solved();
}

ms_settings get_settings(user_data *d) {
    ms_settings s = fetch_default_settings();

    s.seed = (unsigned long) time(0);

    s.run_func = &run_single;
    s.thoughtful_run_func = &thoughtful_run;
    s.solved_func = &solved;

    s.reserved_move_count = 0u;
    s.max_concurrent_threads = 24u;
    s.max_concurrent_games = 5u;

    s.initial_vote_count = 10u;
    s.vote_increase_step = 100u;
    s.max_vote_count = 1000u;
    s.agree_ratio = 80u;

    s.user_data = (void *) d;

    return s;
}

user_data get_user_data() {
    user_data d;

    d.run_cache_size = 1000000u;
    d.run_timeout = 20000u;

    d.deal_seed_file = NULL;

    return d;
}

long filelen(FILE *f) {
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, pos, SEEK_SET);

    return length;
}

int file_str(FILE *f, char *buf, long buflen) {
    fread(buf, 1, buflen, f);
    return 0;
}

enum prim_type {
    PRIM_NUM,
    PRIM_BOOL,
    PRIM_NULL
};

jsmntok_t *next_tok(jsmntok_t *t) {
    return t + 1;
}

enum PRIM_TYPE {
    NUMBER_TYPE, BOOL_TYPE, NULL_TYPE
};

PRIM_TYPE prim_type(jsmntok_t *tok, char *js) {
    switch (js[tok->start]) {
        case '0': case '1': case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9': case '-':
            return NUMBER_TYPE;
        case 't': case 'f':
            return BOOL_TYPE;
        case 'n':
            return NULL_TYPE;
        default:
            assert(false);
    }
}

int assign_rule_prim(jsmntok_t *key, const char *name, PRIM_TYPE type,
        void *rule, char *js) {
    if (!strncmp(&js[key->start], name, key->end - key->start)) {
        jsmntok_t *val = next_tok(key);
        assert(val->type == JSMN_PRIMITIVE);
        PRIM_TYPE pt = prim_type(val, js);
        assert(pt == type);
        unsigned *irule = (unsigned *) rule;
        bool *brule = (bool *) rule;
        switch (pt) {
            case NUMBER_TYPE:
                *irule = atoi(&js[val->start]);
                break;
            case BOOL_TYPE:
                *brule = (js[val->start] == 't');
                break;
            case NULL_TYPE:
            default:
                assert(false);
        }
        return 2;
    } else {
        return 0;
    }
}

int assign_rule_enum(jsmntok_t *key, const char *name, char (*func)(char *),
        char *rule, char *js) {
    long length = key->end - key->start;
    char *key_str = &js[key->start];
    if (!strncmp(key_str, name, length)) {
        jsmntok_t *val = next_tok(key);
        char *buf = (char *) malloc(length + 1);

        char *val_str = &js[val->start];
        long val_length = val->end - val->start;
        strncpy(buf, val_str, val_length);
        buf[val_length] = '\0';
        *rule = func(buf);
        return 2;
    } else {
        return 0;
    }
}

int assign_pile(jsmntok_t *key, const char *pile_name, ms_card_pile *pile,
        char *js) {
    pile->clear();

    if (!strncmp(&js[key->start], pile_name, key->end - key->start)) {
        jsmntok_t *val = next_tok(key);
        assert(val->type == JSMN_ARRAY);
        jsmntok_t *elem = val;
        for (int i = 0; i < val->size; ++i) {
            elem = next_tok(elem);
            assert(elem->type == JSMN_STRING);
            pile->push_back(ms_str_card(&js[elem->start], false));
        }
        return elem - key + 1;
    } else {
        return 0;
    }
}

int assign_pile_group(jsmntok_t *key, const char *group_name,
        std::vector<ms_card_pile> *group, char *js) {
    group->clear();

    if (!strncmp(&js[key->start], group_name, key->end - key->start)) {
        jsmntok_t *piles = next_tok(key);
        assert(piles->type == JSMN_ARRAY);
        jsmntok_t *elem = piles;
        for (int i = 0; i < piles->size; ++i) {
            elem = next_tok(elem);
            assert(elem->type == JSMN_ARRAY);
            std::vector<ms_card> p;
            jsmntok_t *outer = elem;
            for (int j = 0; j < outer->size; ++j) {
                elem = next_tok(elem);
                assert(elem->type == JSMN_STRING);
                p.push_back(ms_str_card(&js[elem->start], false));
            }
            group->push_back(p);
        }
        return elem - key + 1;
    } else {
        return 0;
    }
}

int assign_foundations_base(jsmntok_t *key, ms_rules *r, char *js) {
    long length = key->end - key->start;
    char *key_str = &js[key->start];
    if (!strncmp(key_str, "foundations base", length)
            || !strncmp(key_str, "foundations-base", length)
            || !strncmp(key_str, "foundations_base", length)) {

        jsmntok_t *val = next_tok(key);
        char *val_str = &js[val->start];
        if (!strncmp(val_str, "random", strlen("random"))) {
            r->specific_foundations_base = false;
        } else {
            r->specific_foundations_base = true;
            r->foundations_base = char_rank(val_str[0]);
        }

        return 2;
    } else {
        return 0;
    }
}

void json_settings(ms_settings *s, user_data *d, const char *filename) {
    FILE *f = fopen(filename, "rb");
    long length = filelen(f);
    char *buf = (char *) malloc(length);

    assert(buf);

    file_str(f, buf, length);

    jsmn_parser parser;
    jsmn_init(&parser);
    int tok_count = jsmn_parse(&parser, buf, length, NULL, 0);
    jsmntok_t *tokens = (jsmntok_t *) malloc(tok_count * sizeof(jsmntok_t));

    assert(tokens);

    jsmn_init(&parser);
    auto parse_val = jsmn_parse(&parser, buf, length, tokens, tok_count);

    assert(parse_val != JSMN_ERROR_INVAL);
    assert(parse_val != JSMN_ERROR_NOMEM);
    assert(parse_val != JSMN_ERROR_PART);

    for (int i = 0; i < tok_count;) {
        jsmntok_t *tok = &tokens[i];

        int x = 0;
        switch (tok->type) {
            case JSMN_OBJECT: {
                x = 1;
                break;
            } case JSMN_STRING:
            case JSMN_PRIMITIVE: {
                if (buf[tok->start] == '#') {
                    x = 1;
                    break;
                }

#               define arp(n, t, p) { \
                    x = assign_rule_prim(tok, n, t, p, buf); \
                    if (x) break; \
                }

                arp("max concurrent threads", NUMBER_TYPE,
                        &s->max_concurrent_threads);
                arp("max_concurrent_threads", NUMBER_TYPE,
                        &s->max_concurrent_threads);
                arp("max-concurrent-threads", NUMBER_TYPE,
                        &s->max_concurrent_threads);

                arp("max concurrent games", NUMBER_TYPE,
                        &s->max_concurrent_games);
                arp("max_concurrent_games", NUMBER_TYPE,
                        &s->max_concurrent_games);
                arp("max-concurrent-games", NUMBER_TYPE,
                        &s->max_concurrent_games);

                arp("seed", NUMBER_TYPE, &s->seed);
                arp("solveseed", NUMBER_TYPE, &s->seed);
                arp("solve seed", NUMBER_TYPE, &s->seed);
                arp("solve_seed", NUMBER_TYPE, &s->seed);
                arp("solve-seed", NUMBER_TYPE, &s->seed);

                arp("forever", BOOL_TYPE, &s->forever);
                arp("run forever", BOOL_TYPE, &s->forever);
                arp("run_forever", BOOL_TYPE, &s->forever);
                arp("run-forever", BOOL_TYPE, &s->forever);

                arp("run timeout", NUMBER_TYPE, &d->run_timeout);
                arp("run_timeout", NUMBER_TYPE, &d->run_timeout);
                arp("run-timeout", NUMBER_TYPE, &d->run_timeout);

                arp("run node limit", NUMBER_TYPE, &d->run_timeout);
                arp("run_node_limit", NUMBER_TYPE, &d->run_timeout);
                arp("run-node-limit", NUMBER_TYPE, &d->run_timeout);

                arp("initial vote count", NUMBER_TYPE, &s->initial_vote_count);
                arp("initial_vote_count", NUMBER_TYPE, &s->initial_vote_count);
                arp("initial-vote-count", NUMBER_TYPE, &s->initial_vote_count);

                arp("max vote count", NUMBER_TYPE, &s->max_vote_count);
                arp("max_vote_count", NUMBER_TYPE, &s->max_vote_count);
                arp("max-vote-count", NUMBER_TYPE, &s->max_vote_count);

                arp("vote increase step", NUMBER_TYPE, &s->vote_increase_step);
                arp("vote_increase_step", NUMBER_TYPE, &s->vote_increase_step);
                arp("vote-increase-step", NUMBER_TYPE, &s->vote_increase_step);

                arp("vote increase magnitude", NUMBER_TYPE,
                        &s->vote_increase_magnitude);
                arp("vote_increase_magnitude", NUMBER_TYPE,
                        &s->vote_increase_magnitude);
                arp("vote-increase-magnitude", NUMBER_TYPE,
                        &s->vote_increase_magnitude);

                arp("agree ratio", NUMBER_TYPE, &s->agree_ratio);
                arp("agree_ratio", NUMBER_TYPE, &s->agree_ratio);
                arp("agree-ratio", NUMBER_TYPE, &s->agree_ratio);

            } default: {
                assert(false);
            }
        }
        i += x;
    }

    free(buf);
    free(tokens);
}

ms_rules json_rules(const char *filename, ms_settings *s) {
    user_data *d = (user_data *) s->user_data;

    FILE *f = fopen(filename, "rb");
    long length = filelen(f);
    char *buf = (char *) malloc(length);

    assert(buf);

    file_str(f, buf, length);

    jsmn_parser parser;
    jsmn_init(&parser);
    int tok_count = jsmn_parse(&parser, buf, length, NULL, 0);
    jsmntok_t *tokens = (jsmntok_t *) malloc(tok_count * sizeof(jsmntok_t));

    assert(tokens);

    jsmn_init(&parser);
    auto parse_val = jsmn_parse(&parser, buf, length, tokens, tok_count);

    assert(parse_val != JSMN_ERROR_INVAL);
    assert(parse_val != JSMN_ERROR_NOMEM);
    assert(parse_val != JSMN_ERROR_PART);

    ms_rules r = fetch_default_rules();

    for (int i = 0; i < tok_count;) {
        jsmntok_t *tok = &tokens[i];

        int x = 0;
        switch (tok->type) {
            case JSMN_OBJECT: {
                x = 1;
                break;
            } case JSMN_STRING:
            case JSMN_PRIMITIVE: {
                if (buf[tok->start] == '#') {
                    x = 1;
                    break;
                }

#               define arp(n, t, p) { \
                    x = assign_rule_prim(tok, n, t, p, buf); \
                    if (x) break; \
                }

                arp("tableau size", NUMBER_TYPE, &r.tableau_size);
                arp("tableau-size", NUMBER_TYPE, &r.tableau_size);
                arp("tableau_size", NUMBER_TYPE, &r.tableau_size);

                arp("deck count", NUMBER_TYPE, &r.deck_count);
                arp("deck-count", NUMBER_TYPE, &r.deck_count);
                arp("deck_count", NUMBER_TYPE, &r.deck_count);
                arp("decks", NUMBER_TYPE, &r.deck_count);

                arp("max rank", NUMBER_TYPE, &r.max_rank);
                arp("max-rank", NUMBER_TYPE, &r.max_rank);
                arp("max_rank", NUMBER_TYPE, &r.max_rank);

                arp("hole", BOOL_TYPE, &r.hole_present);
                arp("hole present", BOOL_TYPE, &r.hole_present);
                arp("hole-present", BOOL_TYPE, &r.hole_present);
                arp("hole_present", BOOL_TYPE, &r.hole_present);

                arp("hole build loops", BOOL_TYPE, &d->hole_build_loops);
                arp("hole-build-loops", BOOL_TYPE, &d->hole_build_loops);
                arp("hole_build_loops", BOOL_TYPE, &d->hole_build_loops);

                arp("foundations", BOOL_TYPE, &r.foundations_present);
                arp("foundations present", BOOL_TYPE, &r.foundations_present);
                arp("foundations-present", BOOL_TYPE, &r.foundations_present);
                arp("foundations_present", BOOL_TYPE, &r.foundations_present);

                arp("foundations removable", BOOL_TYPE,
                        &d->foundations_removable);
                arp("foundations-removable", BOOL_TYPE,
                        &d->foundations_removable);
                arp("foundations_removable", BOOL_TYPE,
                        &d->foundations_removable);

                arp("foundations accept only complete piles", BOOL_TYPE,
                        &d->foundations_only_comp_piles);
                arp("foundations-accept-only-complete-piles", BOOL_TYPE,
                        &d->foundations_only_comp_piles);
                arp("foundations_accept_only_complete_piles", BOOL_TYPE,
                        &d->foundations_only_comp_piles);
                arp("foundations only complete pile moves", BOOL_TYPE,
                        &d->foundations_only_comp_piles);
                arp("foundations-only-complete-pile-moves", BOOL_TYPE,
                        &d->foundations_only_comp_piles);
                arp("foundations_only_complete_pile_moves", BOOL_TYPE,
                        &d->foundations_only_comp_piles);

                arp("diagonal deal", BOOL_TYPE, &r.diagonal_deal);
                arp("diagonal-deal", BOOL_TYPE, &r.diagonal_deal);
                arp("diagonal_deal", BOOL_TYPE, &r.diagonal_deal);

                arp("number of cells", NUMBER_TYPE, &r.cells);
                arp("number-of-cells", NUMBER_TYPE, &r.cells);
                arp("number_of_cells", NUMBER_TYPE, &r.cells);

                arp("cells pre-filled", BOOL_TYPE, &r.cells_pre_filled);
                arp("cells-pre-filled", BOOL_TYPE, &r.cells_pre_filled);
                arp("cells_pre-filled", BOOL_TYPE, &r.cells_pre_filled);

                arp("cards in stock", NUMBER_TYPE, &r.stock_size);
                arp("cards-in-stock", NUMBER_TYPE, &r.stock_size);
                arp("cards_in_stock", NUMBER_TYPE, &r.stock_size);

                arp("stock deal count", NUMBER_TYPE, &r.stock_deal_count);
                arp("stock-deal-count", NUMBER_TYPE, &r.stock_deal_count);
                arp("stock_deal_count", NUMBER_TYPE, &r.stock_deal_count);

                arp("stock redeal allowed", BOOL_TYPE, &r.stock_redeal);
                arp("stock-redeal-allowed", BOOL_TYPE, &r.stock_redeal);
                arp("stock_redeal_allowed", BOOL_TYPE, &r.stock_redeal);

                arp("cards in reserve", NUMBER_TYPE, &r.reserve_size);
                arp("cards-in-reserve", NUMBER_TYPE, &r.reserve_size);
                arp("cards_in_reserve", NUMBER_TYPE, &r.reserve_size);

                arp("reserve stacked", BOOL_TYPE, &r.reserve_stacked);
                arp("reserve-stacked", BOOL_TYPE, &r.reserve_stacked);
                arp("reserve_stacked", BOOL_TYPE, &r.reserve_stacked);

                arp("cards in sequence", NUMBER_TYPE, &r.sequence_count);
                arp("cards-in-sequence", NUMBER_TYPE, &r.sequence_count);
                arp("cards_in_sequence", NUMBER_TYPE, &r.sequence_count);

                arp("sequence fixed suit", BOOL_TYPE, &d->sequence_fixed_suit);
                arp("sequence-fixed-suit", BOOL_TYPE, &d->sequence_fixed_suit);
                arp("sequence_fixed_suit", BOOL_TYPE, &d->sequence_fixed_suit);

                /*
                 * FIXME
                 * Doing some really sketchy casting here, but it should work:
                 * enums have to be at least a char in size, and these enum
                 * values should never end up being more than a char in value.
                 * In fact, they shouldn't ever touch the eighth bit (which
                 * could cause signed-casting problems).
                 */
#               define are(n, f, p) { \
                    x = assign_rule_enum(tok, n, (char (*)(char *)) f, \
                            (char *) p, buf); \
                    if (x) break; \
                }

                are("build policy", str_build_policy, &d->build_policy);
                are("build-policy", str_build_policy, &d->build_policy);
                are("build_policy", str_build_policy, &d->build_policy);

                are("spaces policy", str_spaces_policy, &d->spaces_policy);
                are("spaces-policy", str_spaces_policy, &d->spaces_policy);
                are("spaces_policy", str_spaces_policy, &d->spaces_policy);

                are("move built group", str_built_group, &d->move_built_group);
                are("move-built-group", str_built_group, &d->move_built_group);
                are("move_built_group", str_built_group, &d->move_built_group);

                are("group build policy", str_build_policy,
                        &d->built_group_policy);
                are("group-build-policy", str_build_policy,
                        &d->built_group_policy);
                are("group_build_policy", str_build_policy,
                        &d->built_group_policy);

                are("foundations initialised", str_foundations_init,
                        &r.foundations_init_cards);
                are("foundations-initialised", str_foundations_init,
                        &r.foundations_init_cards);
                are("foundations_initialised", str_foundations_init,
                        &r.foundations_init_cards);

                are("stock deal type", str_stock_deal, &r.stock_deal_method);
                are("stock-deal-type", str_stock_deal, &r.stock_deal_method);
                are("stock_deal_type", str_stock_deal, &r.stock_deal_method);

                are("stock deal method", str_stock_deal, &r.stock_deal_method);
                are("stock-deal-method", str_stock_deal, &r.stock_deal_method);
                are("stock_deal_method", str_stock_deal, &r.stock_deal_method);

                are("face up cards", str_face_up_policy, &r.face_up_policy);
                are("face-up-cards", str_face_up_policy, &r.face_up_policy);
                are("face_up_cards", str_face_up_policy, &r.face_up_policy);

                are("face up policy", str_face_up_policy, &r.face_up_policy);
                are("face-up-policy", str_face_up_policy, &r.face_up_policy);
                are("face_up_policy", str_face_up_policy, &r.face_up_policy);

                are("sequence direction", str_direction,
                        &d->sequence_direction);
                are("sequence-direction", str_direction,
                        &d->sequence_direction);
                are("sequence_direction", str_direction,
                        &d->sequence_direction);

                are("sequence build policy", str_build_policy,
                        &d->sequence_build_policy);
                are("sequence-build-policy", str_build_policy,
                        &d->sequence_build_policy);
                are("sequence_build_policy", str_build_policy,
                        &d->sequence_build_policy);

                x = assign_foundations_base(tok, &r, buf);
                if (x) {
                    break;
                }

                debug("%s\n", &buf[tok->start]);
                assert(false);
            } default: {
                assert(false);
            }
        }
        i += x;
    }

    free(buf);
    free(tokens);

    return r;
}

ms_game_state json_game_state(const char *filename) {
    FILE *f = fopen(filename, "rb");
    long length = filelen(f);
    char *buf = (char *) malloc(length);

    assert(buf);

    file_str(f, buf, length);

    jsmn_parser parser;
    jsmn_init(&parser);
    int tok_count = jsmn_parse(&parser, buf, length, NULL, 0);
    jsmntok_t *tokens = (jsmntok_t *) malloc(tok_count * sizeof(jsmntok_t));

    assert(tokens);

    jsmn_init(&parser);
    auto parse_val = jsmn_parse(&parser, buf, length, tokens, tok_count);

    assert(parse_val != JSMN_ERROR_INVAL);
    assert(parse_val != JSMN_ERROR_NOMEM);
    assert(parse_val != JSMN_ERROR_PART);

    ms_game_state gs;

    for (int i = 0; i < tok_count;) {
        jsmntok_t *tok = &tokens[i];

        int x = 0;
        switch (tok->type) {
            case JSMN_OBJECT: {
                x = 1;
                break;
            } case JSMN_STRING: {
#               define _assign_pile(n, p) { \
                    x = assign_pile(tok, n, p, buf); \
                    if (x) break; \
                }

#               define _assign_pile_group(n, p) { \
                    x = assign_pile_group(tok, n, p, buf); \
                    if (x) break; \
                }

                _assign_pile("stock", &gs.stock);
                _assign_pile("waste", &gs.waste);
                _assign_pile("reserve", &gs.reserve);
                _assign_pile_group("tableau", &gs.tableau);
                _assign_pile_group("foundations", &gs.foundations);
                _assign_pile_group("cells", &gs.cells);

                assert(false);
            } default: {
                debug("%d\n", tok->type);
                assert(false);
            }
        }
        i += x;
    }

    free(buf);
    free(tokens);

    return gs;
}

ms_game_state make_game_state(ms_rules *r,
        std::map<std::string, docopt::value> &args) {
    if (args["--game"]) {
        const char *filename = args["--game"].asString().c_str();
        return json_game_state(filename);
    } else {
        long seed;
        if (args["--dealseed"]) {
            seed = args["--dealseed"].asLong();
        } else {
            seed = 0;
        }

        return random_game_state(seed, r);
    }
}

ms_rules make_rules(std::map<std::string, docopt::value> &args,
        ms_settings *s) {
    const char *filename = args["<rules_file>"].asString().c_str();
    return json_rules(filename, s);
}

ms_settings make_settings(std::map<std::string, docopt::value> &args,
        user_data *d) {
    ms_settings s = get_settings(d);

    if (args["--settings"]) {
        const char *filename = args["--settings"].asString().c_str();
        json_settings(&s, d, filename);
    }

    if (args["--dealseedfile"]) {
        d->deal_seed_file = args["--dealseedfile"].asString().c_str();
    }

    if (args["--limitnodes"]) {
        d->run_node_limit = args["--limitnodes"].asLong();
    }

    if (args["--limittime"]) {
        d->run_timeout = args["--limittime"].asLong();
    }

    if (args["--solveseed"]) {
        s.seed = (unsigned long)
            strtoll(args["--solveseed"].asString().c_str(), NULL, 0);
    }

    if (args["--forever"]) {
        s.forever = args["--forever"].asBool();
    }

    s.rng = std::mt19937{ s.seed };
    return s;
}

int main(int argc, char **argv) {
    auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true,
            VERSION_STR);

    user_data d = get_user_data();
    ms_settings s = make_settings(args, &d);
    ms_rules r = make_rules(args, &s);

    if (s.forever) {
        return ms_run_many(&r, &s);
    } else {
        ctpl::thread_pool tp(s.max_concurrent_threads);

        if (d.deal_seed_file) {
            return ms_run_from_file(&r, &s, d.deal_seed_file, &tp);
        } else {
            ms_game_state gs = make_game_state(&r, args);

            return ms_run(&gs, &r, &s, &tp);
        }
    }
}
