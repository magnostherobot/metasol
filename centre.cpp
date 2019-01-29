#include <iostream>
#include <map>
#include <algorithm>

#include "solvitaire/headers/api.h"
#include "docopt/docopt.h"
#include "jsmn/jsmn.h"

#include "metasol.h"

static const char USAGE[] =
R"(centre - a metasol instance.

Usage: centre [options]

Options:
  -h --help  Display this help.
  -r FILE --rules FILE  Specify a .json file containing game rules.
  -g FILE --game FILE  Specify a .json file describing a game layout.
  -s SEED --seed SEED  Specify a seed to use when randomly generating a game
                       (set to 0 to use an unspecified seed).)";

static const char VERSION_STR[] = "Solver in-development";

sol_rules::build_policy convert_build_policy(build_policy_t bp) {
    switch (bp) {
        case NO_BUILD:          return sol_rules::build_policy::NO_BUILD;
        case BUILD_SAME_SUIT:   return sol_rules::build_policy::SAME_SUIT;
        case BUILD_ALTERNATING: return sol_rules::build_policy::RED_BLACK;
        case BUILD_ANY:         return sol_rules::build_policy::ANY_SUIT;
        default: assert(false);
    }
}

sol_rules::spaces_policy convert_spaces_policy(spaces_policy_t sp) {
    switch (sp) {
        case NO_SPACE_FILL:
            return sol_rules::spaces_policy::NO_BUILD;
        case KINGS_FILL_SPACE:
            return sol_rules::spaces_policy::KINGS;
        case ANY_FILL_SPACE:
            return sol_rules::spaces_policy::ANY;
        case AUTO_RESERVE_THEN_WASTE:
            return sol_rules::spaces_policy::AUTO_RESERVE_THEN_WASTE;
        case AUTO_WASTE_THEN_STOCK:
            return sol_rules::spaces_policy::AUTO_WASTE_THEN_STOCK;
        default: assert(false);
    }
}

sol_rules::accordion_policy convert_accordion_policy(accordion_policy_t ap) {
    switch (ap) {
        case ACCORDION_SAME_RANK:
            return sol_rules::accordion_policy::SAME_RANK;
        case ACCORDION_SAME_SUIT:
            return sol_rules::accordion_policy::SAME_SUIT;
        case ACCORDION_ALTERNATE_COLOUR:
            return sol_rules::accordion_policy::RED_BLACK;
        case ACCORDION_ANY_SUIT:
            return sol_rules::accordion_policy::ANY_SUIT;
        default: assert(false);
    }
}

sol_rules::stock_deal_type convert_stock_deal(stock_deal_t sd) {
    switch (sd) {
        case STOCK_TO_WASTE:   return sol_rules::stock_deal_type::WASTE;
        case STOCK_TO_TABLEAU: return sol_rules::stock_deal_type::TABLEAU_PILES;
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

sol_rules::direction convert_direction(direction_t d) {
    switch (d) {
        case LEFT:  return sol_rules::direction::LEFT;
        case RIGHT: return sol_rules::direction::RIGHT;
        case BOTH:  return sol_rules::direction::BOTH;
        default: assert(false);
    }
}

sol_rules::built_group_type convert_built_group(built_group_t bg) {
    switch (bg) {
        case CAN_MOVE_BUILT_GROUP:
            return sol_rules::built_group_type::YES;
        case CANNOT_MOVE_BUILT_GROUP:
            return sol_rules::built_group_type::NO;
        case CAN_MOVE_WHOLE_PILE:
            return sol_rules::built_group_type::WHOLE_PILE;
        case CAN_MOVE_MAXIMAL_GROUP:
            return sol_rules::built_group_type::MAXIMAL_GROUP;
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

sol_rules convert_rules(ms_rules *sr) {
    sol_rules r;

    r.tableau_pile_count = sr->tableau_size;
    r.build_pol = convert_build_policy(sr->build_policy);
    r.spaces_pol = convert_spaces_policy(sr->spaces_policy);
    r.move_built_group = convert_built_group(sr->move_built_group);
    r.built_group_pol = convert_build_policy(sr->built_group_policy);
    r.max_rank = sr->max_rank;
    r.hole = sr->hole_present;
    r.foundations_present = sr->foundations_present;
    r.foundations_init_cards =
        convert_foundations_init(sr->foundations_init_cards);
    r.foundations_removable = sr->foundations_removable;
    r.foundations_only_comp_piles = sr->foundations_only_comp_piles;
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
    r.sequence_direction = convert_direction(sr->sequence_direction);
    r.sequence_build_pol = convert_build_policy(sr->sequence_build_policy);
    r.sequence_fixed_suit = sr->sequence_fixed_suit;
    r.accordion_size = sr->accordion_size;

    assert(sr->deck_count <= 2);
    r.two_decks = sr->deck_count == 2;

    if (sr->specific_foundations_base) {
        r.foundations_base = sr->foundations_base;
    } else {
        r.foundations_base = boost::none;
    }

    for (auto &p : sr->accordion_moves) {
        r.accordion_moves.push_back(std::pair<sol_rules::direction, uint8_t>
                (convert_direction(p.first), p.second));
    }

    for (auto &x : sr->accordion_policy) {
        r.accordion_pol.push_back(convert_accordion_policy(x));
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
        case move::mtype::dominance:            return "dominance";
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

game_state convert_game_state(ms_game_state *sgs, ms_rules *r) {
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

    game_state gs(convert_rules(r), suits, ranks, face_down);
    return gs;
}

typedef struct user_data {
    unsigned run_cache_size;
    unsigned run_timeout;
    unsigned thoughtful_run_timeout;
} user_data;

void print_sgs(ms_game_state *gs, ms_rules *r) {
    game_state solv_gs = convert_game_state(gs, r);
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

unsigned get_waste_index(ms_rules *r) {
    assert(r->stock_deal_method == STOCK_TO_WASTE);

    unsigned i = 1;
    if (r->hole_present) {
        ++i;
    }

    if (r->foundations_present) {
        i += 4 * r->deck_count;
    }

    i += r->cells;

    return i;
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

/**
 * The function that is called by every voter, to get its vote.
 */
finished_state run_single(ms_game_state *gs, ms_rules *r,
        std::vector<ms_move> *moves, unsigned move_count, void *data) {
    user_data *d = (user_data *) data;

    game_state solv_gs = convert_game_state(gs, r);

    movelist ml;
    finished_state result = convert_result(get_moves(ml, solv_gs,
                d->run_cache_size, d->run_timeout));

    if (result == SOLUTION_FOUND) {
        unsigned n = std::min(move_count, (unsigned) ml.size());
        moves->clear();
        moves->resize(n);
        int skip_first = is_dud_move(&ml[0]) ? 1 : 0;
        std::transform(ml.begin() + skip_first,
                ml.begin() + skip_first + move_count,
                moves->begin(),
                [gs, r](move m) -> ms_move {
                    ms_move out;
                    convert_move(gs, r, &m, &out);
                    return out;
                });
        std::reverse(moves->begin(), moves->end());
    }

    return result;
}

finished_state thoughtful_run(ms_game_state *gs, ms_rules *r,
        void *data) {
    user_data *d = (user_data *) data;

    game_state solv_gs = convert_game_state(gs, r);
    movelist ml;
    return convert_result(get_moves(ml, solv_gs, d->run_cache_size,
                d->thoughtful_run_timeout));
}

ms_rules fortunes_favor() {
    // default rules
    ms_rules r = fetch_default_rules();

    // rules specific to fortunes favor
    r.tableau_size = 12u;
    r.build_policy = BUILD_SAME_SUIT;
    r.spaces_policy = AUTO_WASTE_THEN_STOCK;
    r.foundations_init_cards = ALL_FOUNDATIONS_INIT;
    r.stock_size = 36u;

    return r;
}

ms_rules klondike() {
    ms_rules r = fetch_default_rules();

    r.tableau_size = 7u;
    r.build_policy = BUILD_ALTERNATING;
    r.spaces_policy = KINGS_FILL_SPACE;
    r.move_built_group = CAN_MOVE_BUILT_GROUP;
    r.built_group_policy = r.build_policy;

    r.diagonal_deal = true;
    r.face_up_policy = TOP_CARDS_FACE_UP;

    r.foundations_removable = true;

    r.stock_size = 24u;
    r.stock_deal_count = 3u;
    r.stock_redeal = true;

    return r;
}

ms_rules canfield() {
    ms_rules r = fetch_default_rules();

    r.tableau_size = 4u;
    r.build_policy = BUILD_ALTERNATING;
    r.move_built_group = CAN_MOVE_WHOLE_PILE;
    r.spaces_policy = AUTO_RESERVE_THEN_WASTE;
    r.built_group_policy = r.build_policy;

    r.foundations_init_cards = ONE_FOUNDATION_INIT;
    r.specific_foundations_base = false;

    r.stock_size = 34u;
    r.stock_deal_count = 3u;
    r.stock_redeal = true;

    r.reserve_size = 13u;
    r.reserve_stacked = true;

    return r;
}

ms_rules simple_canfield() {
    ms_rules r = fetch_default_rules();

    r.tableau_size = 3u;
    r.build_policy = BUILD_ALTERNATING;
    r.move_built_group = CAN_MOVE_WHOLE_PILE;
    r.spaces_policy = AUTO_RESERVE_THEN_WASTE;
    r.built_group_policy = r.build_policy;

    r.foundations_init_cards = ONE_FOUNDATION_INIT;
    r.specific_foundations_base = false;

    r.stock_size = 6u;
    r.stock_deal_count = 3u;
    r.stock_redeal = true;

    r.reserve_size = 2;
    r.reserve_stacked = true;

    r.max_rank = 3u;

    return r;
}

bool solved(ms_game_state *gs, ms_rules *r, void *d) {
    print_sgs(gs, r);

    return convert_game_state(gs, r).is_solved();
}

ms_settings get_settings(user_data *d) {
    ms_settings s = fetch_default_settings();

    s.run_func = &run_single;
    s.thoughtful_run_func = &thoughtful_run;
    s.solved_func = &solved;

    s.reserved_move_count = 10u;
    s.max_concurrent_threads = 24u;
    s.max_votes = 1000u;

    s.user_data = (void *) d;

    return s;
}

user_data get_user_data() {
    user_data d;

    d.run_cache_size = 1024u;
    d.run_timeout = 20000u;
    d.thoughtful_run_timeout = 20000u;

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

int json_game_state(const char *filename, ms_game_state *gs) {
    FILE *f = fopen(filename, "rb");
    long length = filelen(f);
    char *buf = (char *) malloc(length);

    if (!buf) {
        return 1;
    }

    file_str(f, buf, length);

    jsmn_parser parser;
    jsmn_init(&parser);
    int tok_count = jsmn_parse(&parser, buf, length, NULL, 0);
    jsmntok_t *tokens = (jsmntok_t *) malloc(tok_count * sizeof(jsmntok_t));

    if (!tokens) {
        return 2;
    }

    jsmn_init(&parser);
    auto parse_val = jsmn_parse(&parser, buf, length, tokens, tok_count);
    switch (parse_val) {
        case JSMN_ERROR_INVAL:
        case JSMN_ERROR_NOMEM:
        case JSMN_ERROR_PART:
            return 3;
    }

    for (int i = 0; i < tok_count;) {
        jsmntok_t *tok = &tokens[i];

        int x = 0;
        switch (tok->type) {
            case JSMN_OBJECT: {
                x = 1;
                break;
            } case JSMN_STRING: {
                x = assign_pile(tok, "stock", &gs->stock, buf);
                if (x) {
                    break;
                }

                x = assign_pile(tok, "waste", &gs->waste, buf);
                if (x) {
                    break;
                }

                x = assign_pile(tok, "reserve", &gs->reserve, buf);
                if (x) {
                    break;
                }

                x = assign_pile_group(tok, "tableau", &gs->tableau, buf);
                if (x) {
                    break;
                }

                x = assign_pile_group(tok, "foundations", &gs->foundations,
                        buf);
                if (x) {
                    break;
                }

                x = assign_pile_group(tok, "cells", &gs->cells, buf);
                if (x) {
                    break;
                }

                assert(false);
            } default: {
                fprintf(stderr, "%d\n", tok->type);
                assert(false);
            }
        }
        i += x;
    }

    free(buf);
    free(tokens);

    return 0;
}

int make_game_state(ms_game_state *gs, ms_rules *r,
        std::map<std::string, docopt::value> &args) {
    if (args["--game"]) {
        const char *filename = args["--game"].asString().c_str();
        return json_game_state(filename, gs);
    } else {
        long seed;
        if (args["--seed"]) {
            seed = args["--seed"].asLong();
        } else {
            seed = 0;
        }

        return random_game_state(seed, gs, r);
    }
}

void parse_args(int argc, char **argv, ms_game_state *gs, ms_rules *r,
        ms_settings *s) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
            { argv + 1, argv + argc }, true, VERSION_STR);

    *r = simple_canfield();
    make_game_state(gs, r, args);
    /* make_rules(r, args); */
    /* make_settings(s, args); */
}

int main(int argc, char **argv) {
    ms_rules r;
    ms_settings s;
    ms_game_state gs;

    parse_args(argc, argv, &gs, &r, &s);

    user_data d = get_user_data();
    s = get_settings(&d);

    return ms_run(&gs, &r, &s);
}
