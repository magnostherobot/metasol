#include <iostream>
#include <map>
#include <algorithm>

#include <err.h>

#include "solvitaire/headers/api.h"
#include "docopt/docopt.h"
#include "jsmn/jsmn.h"

#include "metasol.h"
#include "debug.h"

static const char USAGE[] =
R"(centre - a metasol instance.

Usage: centre [options]

Options:
  -h --help  Display this help.
  -r FILE --rules FILE  Specify a .json file containing game rules.
  -g FILE --game  FILE  Specify a .json file describing a game layout.
  -s SEED --seed  SEED  Specify a seed to use when randomly generating a game
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
    r.hole_build_loops = sr->hole_build_loops;
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

    game_state solv_gs = convert_game_state(gs, r);

    movelist ml;
    finished_state result = convert_result(get_moves(ml, solv_gs,
                d->run_cache_size, d->run_timeout));

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

    s.reserved_move_count = 0u;
    s.max_concurrent_threads = 24u;
    s.max_votes = 10u;

    s.user_data = (void *) d;

    return s;
}

user_data get_user_data() {
    user_data d;

    d.run_cache_size = 1000000u;
    d.run_timeout = 20000u;
    d.thoughtful_run_timeout = 2000000u;

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
        assert(val->type == JSMN_STRING);
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
    if (!strncmp(key_str, "foundations base", length)) {
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

ms_rules json_rules(const char *filename) {
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
            } case JSMN_STRING: {
#               define arp(n, t, p) { \
                    x = assign_rule_prim(tok, n, t, p, buf); \
                    if (x) break; \
                }

                arp("tableau size", NUMBER_TYPE, &r.tableau_size);
                arp("deck count", NUMBER_TYPE, &r.deck_count);
                arp("max rank", NUMBER_TYPE, &r.max_rank);

                arp("hole", BOOL_TYPE, &r.hole_present);
                arp("hole present", BOOL_TYPE, &r.hole_present);

                arp("hole build loops", BOOL_TYPE, &r.hole_build_loops);

                arp("foundations", BOOL_TYPE, &r.foundations_present);
                arp("foundations present", BOOL_TYPE, &r.foundations_present);

                arp("foundations removable", BOOL_TYPE,
                        &r.foundations_removable);

                arp("foundations accept only complete piles", BOOL_TYPE,
                        &r.foundations_only_comp_piles);
                arp("foundations only complete pile moves", BOOL_TYPE,
                        &r.foundations_only_comp_piles);

                arp("diagonal deal", BOOL_TYPE, &r.diagonal_deal);
                arp("number of cells", NUMBER_TYPE, &r.cells);
                arp("cells pre-filled", BOOL_TYPE, &r.cells_pre_filled);
                arp("cards in stock", NUMBER_TYPE, &r.stock_size);
                arp("stock deal count", NUMBER_TYPE, &r.stock_deal_count);
                arp("stock redeal allowed", BOOL_TYPE, &r.stock_redeal);
                arp("cards in reserve", NUMBER_TYPE, &r.reserve_size);
                arp("reserve stacked", BOOL_TYPE, &r.reserve_stacked);
                arp("cards in sequence", NUMBER_TYPE, &r.sequence_count);
                arp("sequence fixed suit", BOOL_TYPE, &r.sequence_fixed_suit);

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

                are("build policy", str_build_policy, &r.build_policy);
                are("spaces policy", str_spaces_policy, &r.spaces_policy);
                are("move built group", str_built_group, &r.move_built_group);
                are("group build policy", str_build_policy,
                        &r.built_group_policy);

                are("foundations initialised", str_foundations_init,
                        &r.foundations_init_cards);

                are("stock deal type", str_stock_deal, &r.stock_deal_method);
                are("stock deal method", str_stock_deal, &r.stock_deal_method);

                are("face up cards", str_face_up_policy, &r.face_up_policy);
                are("face up policy", str_face_up_policy, &r.face_up_policy);
                are("sequence direction", str_direction, &r.sequence_direction);
                are("sequence build policy", str_build_policy,
                        &r.sequence_build_policy);

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
        if (args["--seed"]) {
            seed = args["--seed"].asLong();
        } else {
            seed = 0;
        }

        return random_game_state(seed, r);
    }
}

ms_rules make_rules(std::map<std::string, docopt::value> &args) {
    if (args["--rules"]) {
        const char *filename = args["--rules"].asString().c_str();
        return json_rules(filename);
    } else {
        return canfield();
    }
}

void parse_args(int argc, char **argv, ms_game_state *gs, ms_rules *r) {
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
            { argv + 1, argv + argc }, true, VERSION_STR);

    *r = make_rules(args);
    *gs = make_game_state(r, args);
    /* make_settings(s, args); */
}

int main(int argc, char **argv) {
    ms_rules r;
    ms_settings s;
    ms_game_state gs;

    parse_args(argc, argv, &gs, &r);

    user_data d = get_user_data();
    s = get_settings(&d);

    return ms_run(&gs, &r, &s);
}
