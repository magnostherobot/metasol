#include <algorithm>
#include <future>
#include <map>
#include <random>

#include <assert.h>
#include <err.h>

#include "ctpl/ctpl_stl.h"

#include "metasol.hpp"
#include "debug.h"

card_suit SUITS[4] = { HEARTS, SPADES, CLUBS, DIAMONDS };

void lower(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

struct dictcmp {
    bool operator()(const char *a, const char *b) const {
        return strcmp(a, b) < 0;
    }
};

static inline unsigned face_down_count_pile(ms_card_pile *p) {
    unsigned result = 0u;
    for (auto &c : *p) {
        result += c.hidden ? 1u : 0u;
    }
    return result;
}

static inline unsigned face_down_count_piles(std::vector<ms_card_pile> *ps) {
    unsigned result = 0u;
    for (auto &p : *ps) {
        result += face_down_count_pile(&p);
    }
    return result;
}

unsigned face_down_count(ms_game_state *gs) {
    unsigned result = 0u;
    result += face_down_count_piles(&gs->foundations);
    result += face_down_count_pile(&gs->stock);
    result += face_down_count_pile(&gs->waste);
    result += face_down_count_piles(&gs->tableau);
    result += face_down_count_pile(&gs->hole);
    result += face_down_count_piles(&gs->cells);
    result += face_down_count_pile(&gs->reserve);
    return result;
}

#define kv_dict(t) std::map<const char *, t, dictcmp>

build_policy_t str_build_policy(char *str) {
    kv_dict(build_policy_t) dict = {
        { "no build", NO_BUILD },
        { "no-build", NO_BUILD },
        { "none", NO_BUILD },

        { "same suit", BUILD_SAME_SUIT },
        { "same-suit", BUILD_SAME_SUIT },
        { "suits", BUILD_SAME_SUIT },

        { "alternating", BUILD_ALTERNATING },
        { "red-black", BUILD_ALTERNATING },

        { "any", BUILD_ANY },
        { "any-suit", BUILD_ANY },
        { "all", BUILD_ANY }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *build_policy_str(build_policy_t bp) {
    const char *const build_policy_arr[] = {
        "NO_BUILD",
        "BUILD_SAME_SUIT",
        "BUILD_ALTERNATING",
        "BUILD_ANY"
    };

    return build_policy_arr[bp];
}

spaces_policy_t str_spaces_policy(char *str) {
    kv_dict(spaces_policy_t) dict = {
        { "none", NO_SPACE_FILL },
        { "no fill", NO_SPACE_FILL },
        { "no-fill", NO_SPACE_FILL },
        { "no_fill", NO_SPACE_FILL },

        { "kings only", KINGS_FILL_SPACE },
        { "kings-only", KINGS_FILL_SPACE },
        { "kings_only", KINGS_FILL_SPACE },
        { "kings", KINGS_FILL_SPACE },

        { "any", ANY_FILL_SPACE },
        { "all", ANY_FILL_SPACE },

        { "reserve first", AUTO_RESERVE_THEN_WASTE },
        { "reserve-first", AUTO_RESERVE_THEN_WASTE },
        { "reserve_first", AUTO_RESERVE_THEN_WASTE },
        { "reserve then waste", AUTO_RESERVE_THEN_WASTE },
        { "reserve-then-waste", AUTO_RESERVE_THEN_WASTE },
        { "reserve_then_waste", AUTO_RESERVE_THEN_WASTE },

        { "waste first", AUTO_WASTE_THEN_STOCK },
        { "waste-first", AUTO_WASTE_THEN_STOCK },
        { "waste_first", AUTO_WASTE_THEN_STOCK },
        { "waste then stock", AUTO_WASTE_THEN_STOCK },
        { "waste-then-stock", AUTO_WASTE_THEN_STOCK },
        { "waste_then_stock", AUTO_WASTE_THEN_STOCK }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *spaces_policy_str(spaces_policy_t bp) {
    const char *const spaces_policy_arr[] = {
        "NO_SPACE_FILL",
        "KINGS_FILL_SPACE",
        "ANY_FILL_SPACE",
        "AUTO_RESERVE_THEN_WASTE",
        "AUTO_WASTE_THEN_STOCK"
    };

    return spaces_policy_arr[bp];
}

accordion_policy_t str_accordion_policy(char *str) {
    kv_dict(accordion_policy_t) dict = {
        { "same rank", ACCORDION_SAME_RANK },
        { "same-rank", ACCORDION_SAME_RANK },
        { "same_rank", ACCORDION_SAME_RANK },
        { "rank", ACCORDION_SAME_RANK },

        { "same suit", ACCORDION_SAME_SUIT },
        { "same-suit", ACCORDION_SAME_SUIT },
        { "same_suit", ACCORDION_SAME_SUIT },
        { "suit", ACCORDION_SAME_SUIT },

        { "alternating colour", ACCORDION_ALTERNATE_COLOUR },
        { "alternating-colour", ACCORDION_ALTERNATE_COLOUR },
        { "alternating_colour", ACCORDION_ALTERNATE_COLOUR },
        { "red-black", ACCORDION_ALTERNATE_COLOUR },

        { "any", ACCORDION_ANY_SUIT }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *accordion_policy_str(accordion_policy_t bp) {
    const char *const accordion_policy_arr[] = {
        "ACCORDION_SAME_RANK",
        "ACCORDION_SAME_SUIT",
        "ACCORDION_ALTERNATE_COLOUR",
        "ACCORDION_ANY_SUIT"
    };

    return accordion_policy_arr[bp];
}

stock_deal_t str_stock_deal(char *str) {
    kv_dict(stock_deal_t) dict = {
        { "waste", STOCK_TO_WASTE },
        { "stock to waste", STOCK_TO_WASTE },
        { "stock-to-waste", STOCK_TO_WASTE },
        { "stock_to_waste", STOCK_TO_WASTE },

        { "tableau", STOCK_TO_TABLEAU },
        { "stock to tableau", STOCK_TO_TABLEAU },
        { "stock-to-tableau", STOCK_TO_TABLEAU },
        { "stock_to_tableau", STOCK_TO_TABLEAU },

        { "hole", STOCK_TO_HOLE },
        { "stock to hole", STOCK_TO_HOLE },
        { "stock-to-hole", STOCK_TO_HOLE },
        { "stock_to_hole", STOCK_TO_HOLE }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *stock_deal_str(stock_deal_t bp) {
    const char *const stock_deal_arr[] = {
        "STOCK_TO_WASTE",
        "STOCK_TO_TABLEAU",
        "STOCK_TO_HOLE"
    };

    return stock_deal_arr[bp];
}

face_up_policy_t str_face_up_policy(char *str) {
    kv_dict(face_up_policy_t) dict = {
        { "all", ALL_CARDS_FACE_UP },

        { "top", TOP_CARDS_FACE_UP },
        { "top only", TOP_CARDS_FACE_UP }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *face_up_policy_str(face_up_policy_t bp) {
    const char *const face_up_policy_arr[] = {
        "ALL_CARDS_FACE_UP",
        "TOP_CARDS_FACE_UP"
    };

    return face_up_policy_arr[bp];
}

direction_t str_direction(char *str) {
    kv_dict(direction_t) dict = {
        { "left", LEFT },

        { "right", RIGHT },

        { "both", BOTH },
        { "lr", BOTH }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *direction_str(direction_t bp) {
    const char *const direction_arr[] = {
        "LEFT",
        "RIGHT",
        "BOTH"
    };

    return direction_arr[bp];
}

built_group_t str_built_group(char *str) {
    kv_dict(built_group_t) dict = {
        { "yes", CAN_MOVE_BUILT_GROUP },

        { "no", CANNOT_MOVE_BUILT_GROUP },
        { "none", CANNOT_MOVE_BUILT_GROUP },

        { "whole pile only", CAN_MOVE_WHOLE_PILE },
        { "whole pile", CAN_MOVE_WHOLE_PILE },

        { "maximal group only", CAN_MOVE_MAXIMAL_GROUP },
        { "maximal", CAN_MOVE_MAXIMAL_GROUP }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *built_group_str(built_group_t bp) {
    const char *const built_group_arr[] = {
        "CAN_MOVE_BUILT_GROUP",
        "CANNOT_MOVE_BUILT_GROUP",
        "CAN_MOVE_WHOLE_PILE",
        "CAN_MOVE_MAXIMAL_GROUP"
    };

    return built_group_arr[bp];
}

foundations_init_t str_foundations_init(char *str) {
    kv_dict (foundations_init_t) dict = {
        { "no", NO_FOUNDATION_INIT },
        { "none", NO_FOUNDATION_INIT },

        { "one", ONE_FOUNDATION_INIT },

        { "all", ALL_FOUNDATIONS_INIT },
        { "yes", ALL_FOUNDATIONS_INIT }
    };

    lower(str);
    auto result = dict.find(str);

    if (result == dict.end()) {
        errx(EXIT_FAILURE, "unknown key '%s'", str);
    }

    return result->second;
}

const char *foundations_init_str(foundations_init_t bp) {
    const char *const foundations_init_arr[] = {
        "NO_FOUNDATION_INIT",
        "ONE_FOUNDATION_INIT",
        "ALL_FOUNDATIONS_INIT"
    };

    return foundations_init_arr[bp];
}

ms_settings fetch_default_settings() {
    ms_settings s;
    s.run_func = NULL;
    s.thoughtful_run_func = NULL;
    s.solved_func = NULL;
    s.reserved_move_count = 1u;
    s.user_data = NULL;
    s.max_concurrent_threads = 1u;
    s.max_concurrent_games = 1u;
    s.vote_count = 100u;
    s.forever = false;
    s.seed = 0u;
    return s;
}

card_rank char_rank(char c) {
    switch (c) {
        case 'A': case 'a': case '1': return 1u;
        case '2': return 2u;
        case '3': return 3u;
        case '4': return 4u;
        case '5': return 5u;
        case '6': return 6u;
        case '7': return 7u;
        case '8': return 8u;
        case '9': return 9u;
        case 'X': case 'x': case 'T': case 't': case '0': return 10u;
        case 'J': case 'j': return 11u;
        case 'Q': case 'q': return 12u;
        case 'K': case 'k': return 13u;
        default: assert(false);
    }
}

card_suit char_suit(char c) {
    switch (c) {
        case 'H': case 'h': return HEARTS;
        case 'S': case 's': return SPADES;
        case 'C': case 'c': return CLUBS;
        case 'D': case 'd': return DIAMONDS;
        default: assert(false);
    }
}

char rank_char(std::uint8_t rank) {
    switch (rank) {
        case  1: return 'A';
        case  2: return '2';
        case  3: return '3';
        case  4: return '4';
        case  5: return '5';
        case  6: return '6';
        case  7: return '7';
        case  8: return '8';
        case  9: return '9';
        case 10: return 'X';
        case 11: return 'J';
        case 12: return 'Q';
        case 13: return 'K';
        default: assert(false);
    }
}

char suit_char(card_suit s) {
    switch (s) {
        case HEARTS:   return 'H';
        case SPADES:   return 'S';
        case CLUBS:    return 'C';
        case DIAMONDS: return 'D';
        default: assert(false);
    }
}

ms_card ms_str_card(char *str, bool hidden = true) {
    ms_card card;
    card.suit = char_suit(str[1]);
    card.rank = char_rank(str[0]);
    card.hidden = hidden;
    return card;
}

void add_hidden(ms_card *c, std::vector<ms_card *> &cs) {
    if (c->hidden) cs.push_back(c);
}

/**
 * Fills a vector with pointers to all hidden cards in the given game state.
 */
int get_hidden_cards(ms_game_state *gs, std::vector<ms_card *> &cs) {
    // TODO: include cells, accordion
    for (auto &c : gs->stock)   add_hidden(&c, cs);
    for (auto &c : gs->waste)   add_hidden(&c, cs);
    for (auto &c : gs->reserve) add_hidden(&c, cs);

    for (ms_card_pile &p : gs->foundations)
        for (ms_card &c : p)
            add_hidden(&c, cs);
    for (auto &p : gs->tableau)
        for (auto &c : p)
            add_hidden(&c, cs);

    return cs.size();
}

unsigned foundation_count(ms_rules *r) {
    if (r->foundations_present) {
        return 4u * r->deck_count;
    } else {
        return 0u;
    }
}

unsigned total_pile_count(ms_rules *r) {
    return r->tableau_size
        + (r->hole_present ? 1 : 0)
        + foundation_count(r)
        + r->cells
        + (r->stock_size > 0 ?
            (r->stock_deal_method == STOCK_TO_WASTE ? 2 : 1) : 0)
        + (r->reserve_size > 0 ? 1 : 0)
    ;

    // TODO: I'm not sure how the sequence, or accordion work;
    // research required!
}

int shuffle_hidden(ms_game_state *gs, ms_settings *s) {
    std::vector<ms_card *> hidden_card_pointers;
    get_hidden_cards(gs, hidden_card_pointers);

    std::vector<ms_card> hidden_cards;
    for (ms_card *p : hidden_card_pointers)
        hidden_cards.push_back(*p);

    std::shuffle(hidden_cards.begin(), hidden_cards.end(), s->rng);

    for (__typeof__(hidden_cards.size()) i = 0; i < hidden_cards.size(); ++i) {
        ms_card *cp = hidden_card_pointers[i];
        ms_card c = hidden_cards[i];
        *cp = c;
    }

    return 0;
}

ms_card_pile *get_pile_by_index(ms_game_state *gs, ms_rules *r, uint8_t i) {
    if (r->hole_present) {
        if (i-- == 0) {
            return &gs->hole;
        }
    }

    if (i < gs->foundations.size()) {
        return &gs->foundations[i];
    } else {
        i -= gs->foundations.size();
    }

    if (r->stock_size) {
        if (i-- == 0) {
            return &gs->stock;
        }

        if (r->stock_deal_method == STOCK_TO_WASTE) {
            if (i-- == 0) {
                return &gs->waste;
            }
        }
    }

    if (r->reserve_size) {
        if (i-- == 0) {
            return &gs->reserve;
        }
    }

    if (i < gs->tableau.size()) {
        return &gs->tableau[i];
    } else {
        i -= gs->tableau.size();
    }

    assert(false);
}

typedef struct {
    ms_game_state *gs;
    ms_rules *r;
    ms_settings *s;
    std::vector<ms_move> *move_buf;
    finished_state *result;
} thread_info;

void run_thread(int id, void *vargp) {
    thread_info *ti = (thread_info *) vargp;
    *ti->result = ti->s->run_func(ti->gs, ti->r, ti->move_buf,
            ti->s->reserved_move_count, ti->s->user_data);
}

bool ms_move_lt(const ms_move &a, const ms_move &b) {
    if (a.stock && b.stock) return false;

    if (a.from < b.from) return true;
    if (a.from > b.from) return false;
    if (a.to < b.to) return true;
    return false;
}

struct votecmp {
    bool operator()(const ms_move &a, const ms_move &b) const {
        return ms_move_lt(a, b);
    }
};

int intlen(int i) {
    if (i == 0) return 1;
    return floor(log10(abs(i))) + 1;
}

bool move_atop(ms_card_pile *f, ms_card_pile *t, unsigned count,
        bool reveal_next) {

    ms_card_pile temp(f->end() - count, f->end());
    f->erase(f->end() - count, f->end());
    t->insert(t->end(), temp.begin(), temp.end());

    if (reveal_next && !f->empty()) {
        bool result = f->back().hidden;
        f->back().hidden = false;
        return result;
    } else {
        return false;
    }
}

int flip_waste(ms_game_state *gs) {
    auto temp = gs->waste;
    gs->waste = gs->stock;
    gs->stock = temp;
    std::reverse(gs->stock.begin(), gs->stock.end());

    return 0;
}

bool go_through_stock(ms_game_state *gs, ms_rules *r) {
    if (gs->stock.size() == 0) {
        assert(r->stock_redeal);
        flip_waste(gs);
    }

    /*
     * FIXME: casting used here, but maybe it can be avoided?
     */
    unsigned turn_over = std::min(r->stock_deal_count,
            (unsigned) gs->stock.size());

    bool result = false;
    switch (r->stock_deal_method) {
        case STOCK_TO_WASTE: {
            for (unsigned i = 0; i < turn_over; ++i) {
                bool reveal = move_atop(&gs->stock, &gs->waste, 1u, true);
                result = result || reveal;
            }
            return result;
        } case STOCK_TO_TABLEAU: {
            for (ms_card_pile &p : gs->tableau) {
                if (gs->stock.empty()) {
                    break;
                }

                bool reveal = gs->stock.back().hidden;
                gs->stock.back().hidden = false;
                move_atop(&gs->stock, &p, 1u, false);
                result = result || reveal;
            }
            return result;
        } default: {
            assert(false);
        }
    }
}

bool make_move(ms_game_state *gs, ms_rules *r, ms_move *m) {
    bool result;

    if (m->stock) {
        result = go_through_stock(gs, r);
    } else {
        ms_card_pile *from = get_pile_by_index(gs, r, m->from);
        ms_card_pile *to = get_pile_by_index(gs, r, m->to);
        result = move_atop(from, to, m->size, true);
    }

    gs->moves_made.push_back(*m);

    return result;
}

char const *finished_state_str(finished_state f) {
    switch (f) {
        case SOLUTION_FOUND: return "SOLUTION_FOUND";
        case NO_SOLUTION:    return "NO_SOLUTION";
        case CANCELLED:      return "CANCELLED";
        default: assert(false);
    }
}

const char *bool_str(bool b) {
    return b ? "true" : "false";
}

bool equal_moves(ms_move *a, ms_move *b) {
    bool result =
        !(ms_move_lt(*a, *b)) &&
        !(ms_move_lt(*b, *a));

    return result;
}

bool opposite_move(ms_move *a, ms_move *b) {
    if (!a || !b) {
        return false;
    } else if (a->stock || b->stock) {
        return false;
    } else if (a->from != b->to) {
        return false;
    } else if (a->to != b->from) {
        return false;
    } else if (a->size != b->size) {
        return false;
    } else {
        return true;
    }
}

void move_decided(ms_game_state *gs, ms_rules *r, ms_move *move, vote *votes,
        unsigned vote_count) {

    /*
     * If at least one vote is cast, the move with the most votes is taken.
     */
    bool reveal = make_move(gs, r, move);

    if (reveal) {

        debug("card revealed; resetting all voters\n");

        /*
         * Revealing a card means that all the voters have to be reset.
         */
        for (unsigned i = 0; i < vote_count; ++i) {
            votes[i].result = CANCELLED;
        }
    } else {

        /*
         * The voters that made the popular vote have their move list
         * trimmed, and to ensure that voters that made the popular vote are
         * the only voters that don't need to recalculate their votes, the
         * other voters are reset.
         */
        for (unsigned i = 0; i < vote_count; ++i) {
            if (votes[i].result == SOLUTION_FOUND) {
                if (equal_moves(move, &votes[i].moves.back())) {
                    votes[i].moves.pop_back();
                } else {
                    debug("voter %d is being reset\n", i);
                    votes[i].result = CANCELLED;
                }
            }
        }
    }
}

bool find_majority_move(vote *v, unsigned vote_count, ms_move *m) {

    std::map<ms_move, unsigned, votecmp> votes;
    for (unsigned i = 0; i < vote_count; ++i) {
        if (v[i].result == SOLUTION_FOUND && !v[i].moves.empty()) {
            ms_move move = v[i].moves.back();

            auto t = votes.find(move);
            if (t == votes.end()) {
                votes[move] = 1;
            } else {
                unsigned x = t->second + 1;

                if (x > vote_count / 2) {
                    *m = t->first;
                    return true;
                } else {
                    votes[move] = x;
                }
            }
        }
    }

    return false;
}

typedef std::vector<std::future<void>> jobs;

void run_new_voters(ms_game_state *gs, ms_rules *r, ms_settings *s,
        ctpl::thread_pool *thpool, ms_game_state *ss, vote *v, thread_info *ti,
        jobs *fs, unsigned n) {

    for (unsigned i = 0; i < n; ++i) {

        /*
         * Move sequences that still represent the game need not be overwritten.
         */
        if (v[i].result != SOLUTION_FOUND || v[i].moves.empty()) {
            ss[i] = *gs;
            shuffle_hidden(&ss[i], s);

            ti[i].gs = &ss[i];
            ti[i].r = r;
            ti[i].s = s;
            ti[i].move_buf = &v[i].moves;
            ti[i].result = &v[i].result;

            fs->push_back(thpool->push(run_thread, &ti[i]));
        }
    }
}

bool find_modal_move(vote *v, unsigned n, ms_move *m) {

    std::map<ms_move, unsigned, votecmp> votes;
    for (unsigned i = 0; i < n; ++i) {
        if (v[i].result == SOLUTION_FOUND) {
            assert(!v[i].moves.empty());

            ms_move move = v[i].moves.back();

            auto t = votes.find(move);
            if (t == votes.end()) {
                votes[move] = 1;
            } else {
                votes[move] = t->second + 1;
            }
        }
    }

    struct { ms_move m; unsigned c; } max;
    max.c = 0;

    for (auto &vote : votes) {
        if (vote.second > max.c) {
            max.m = vote.first;
            max.c = vote.second;
        }
    }

    if (max.c == 0) {
        return false;
    } else {
        *m = max.m;
        return true;
    }
}

bool solved(ms_game_state *gs, ms_rules *r, ms_settings *s) {
    assert(s->solved_func);
    return s->solved_func(gs, r, s->user_data);
}

bool solvable(ms_game_state *gs, ms_rules *r, ms_settings *s) {
    assert(s->thoughtful_run_func);

    finished_state fs = s->thoughtful_run_func(gs, r, s->user_data);
    bool result = fs == SOLUTION_FOUND;

    if (result) {
        debug("there is still a solution\n");
    } else {
        debug("no solution left!\n");
    }

    return result;
}

enum solve_status {
    KEEP_GOING,
    SOLVED,
    UNSOLVABLE
};

solve_status loop_check(ms_game_state *gs, ms_rules *r, ms_settings *s) {
    if (solved(gs, r, s)) {
        return SOLVED;
    } else if (solvable(gs, r, s)) {
        return KEEP_GOING;
    } else {
        return UNSOLVABLE;
    }
}

solve_status run_loop(ms_game_state *gs, ms_rules *r, ms_settings *s,
        ctpl::thread_pool *thpool, vote *votes, thread_info *t_infos,
        ms_game_state *shuffled_states) {

    unsigned vote_count = s->vote_count;
    ms_move m;

    /*
     * If there are no face-down cards, then any voter run should tell us if the
     * deal is solvable. This function is only entered if a voter found a
     * solution, so it can be inferred that a solution is found.
     */
    if (face_down_count(gs) == 0) {
        debug("solution found!\n");
        return SOLVED;
    }

    /*
     * Set up the thread_info structs and add a job to the threadpool for each
     * one.
     */
    jobs fs;
    run_new_voters(gs, r, s, thpool, shuffled_states, votes, t_infos, &fs,
            vote_count);

    /*
     * Wait on all of the voters to complete.
     */
    for (auto &f : fs) {
        assert(f.valid());
        f.wait();
    }

    /*
     * Print information about each voter.
     */
    debug( "%-15s %-10s %3s\n", "state", "move", "moves left");
    for (unsigned i = 0; i < s->vote_count; ++i) {
        if (votes[i].moves.empty()) {
            debug("%-15s\n", finished_state_str(votes[i].result));
        } else {
            ms_move m = votes[i].moves.back();
            debug("%-15s %-10s %3lu\n", finished_state_str(votes[i].result),
                    move_str_buf(&m),
                    votes[i].moves.size());
        }
    }

    /*
     * Find the move with the most votes.
     */
    if (find_modal_move(votes, vote_count, &m)) {
        move_decided(gs, r, &m, votes, vote_count);
    }

    /*
     * If a majority of voters already agree on a move, running other voters is
     * a waste of time.
     */
    while (find_majority_move(votes, vote_count, &m)) {
        debug("majority move found\n");
        move_decided(gs, r, &m, votes, vote_count);
    }

    return loop_check(gs, r, s);
}

int ms_run(ms_game_state *gs, ms_rules *r, ms_settings *s,
        ctpl::thread_pool *tp) {

    std::vector<vote> votes(s->vote_count);
    for (vote &v : votes) {
        v.result = CANCELLED;
    }

    std::vector<thread_info> t_infos(s->vote_count);
    std::vector<ms_game_state> states(s->vote_count);

    solve_status x;
    do {
        x = run_loop(gs, r, s, tp, &votes[0], t_infos.data(), states.data());
    } while (x == KEEP_GOING);

    printf("solveseed: %lu\tdealseed: %lu\tsuccess: %s\n", s->seed, gs->seed,
            bool_str(x == SOLVED));

    return 0;
}

int run_single(ms_rules *r, ms_settings *s, unsigned long seed,
        ctpl::thread_pool *tp) {
    ms_game_state gs = random_game_state(seed, r);
    return ms_run(&gs, r, s, tp);
}

int ms_run_single(ms_rules *r, ms_settings *s, unsigned long seed) {
    ctpl::thread_pool tp(s->max_concurrent_threads);
    return run_single(r, s, seed, &tp);
}

int ms_run_infinite(int tid, ms_rules *r, ms_settings *s, ctpl::thread_pool *tp,
        unsigned n, unsigned i) {
    for (unsigned long id = i; true; id += n) {
        run_single(r, s, id, tp);
    }
    return 0;
}

ctpl::thread_pool *main_thread_pool;
void exit_run_many(int _) {
    main_thread_pool->interrupt(false);
}

int ms_run_many(ms_rules *r, ms_settings *s) {
    unsigned n = s->max_concurrent_games;
    ctpl::thread_pool game_tp(n);

    std::vector<std::future<int>> fs;
    ctpl::thread_pool main_tp(s->max_concurrent_threads);
    for (unsigned i = 1; i <= n; ++i) {
        fs.push_back(game_tp.push(ms_run_infinite, r, s, &main_tp, n, i));
    }

    main_thread_pool = &main_tp;
    /* signal(SIGINT, exit_run_many); */

    /*
     * these will run forever, unless stopped by ^C
     */
    for (auto &f : fs) {
        assert(f.valid());
        f.wait();
    }

    return 0;
}

ms_rules fetch_default_rules() {
    ms_rules dr;

    dr.tableau_size = 8u;
    dr.build_policy = BUILD_ANY;
    dr.spaces_policy = ANY_FILL_SPACE;
    dr.move_built_group = CANNOT_MOVE_BUILT_GROUP;
    dr.built_group_policy = NO_BUILD;

    dr.deck_count = 1u;
    dr.max_rank = 13u;

    dr.hole_present = false;
    dr.hole_build_loops = true;

    dr.foundations_present = true;
    dr.foundations_init_cards = NO_FOUNDATION_INIT;
    dr.specific_foundations_base = true;
    dr.foundations_base = 1u;
    dr.foundations_removable = false;
    dr.foundations_only_comp_piles = false;

    dr.diagonal_deal = false;
    dr.cells = 0u;
    dr.cells_pre_filled = 0u;

    dr.stock_size = 0u;
    dr.stock_deal_method = STOCK_TO_WASTE;
    dr.stock_deal_count = 1u;
    dr.stock_redeal = false;

    dr.reserve_size = 0u;
    dr.reserve_stacked = false;

    dr.face_up_policy = ALL_CARDS_FACE_UP;

    dr.sequence_count = 0u;
    dr.sequence_direction = RIGHT;
    dr.sequence_build_policy = dr.build_policy;
    dr.sequence_fixed_suit = false;

    dr.accordion_size = 0;
    /* dr.accordion_moves is an empty vector */
    /* dr.accordion_policy is an empty vector */

    return dr;
}

void print_rules(ms_rules *r, FILE *fh = stdout) {
    fprintf(fh, "tableau size: %u\n"
            "build policy: %s\n"
            "spaces policy: %s\n"
            "move built group: %s\n"
            "built group policy: %s\n"
            "deck count: %u\n"
            "max rank: %u\n"
            "hole: %s\n"
            "foundations: %s\n"
            "foundations init cards: %s\n"
            "foundations base: %u\n"
            "foundations removable: %s\n"
            "foundations only accept complete piles: %s\n"
            "diagonal deal: %s\n"
            "cell count: %u\n"
            "cells pre-filled: %s\n"
            "cards in stock: %u\n"
            "stock deal method: %s\n"
            "stock deal count: %u\n"
            "stock redeal: %s\n"
            "cards in reserve: %u\n"
            "reserve stacked: %s\n"
            "face up policy: %s\n"
            "sequences: %u\n"
            "sequence direction: %s\n"
            "sequence build policy: %s\n"
            "sequence fixed suit: %s\n"
            "cards in accordion: %u\n"
            "(TODO: accordion moves)",
        r->tableau_size,
        build_policy_str(r->build_policy),
        spaces_policy_str(r->spaces_policy),
        built_group_str(r->move_built_group),
        build_policy_str(r->built_group_policy),
        r->deck_count,
        r->max_rank,
        bool_str(r->hole_present),
        bool_str(r->foundations_present),
        bool_str(r->foundations_init_cards),
        r->foundations_base,
        bool_str(r->foundations_removable),
        bool_str(r->foundations_only_comp_piles),
        bool_str(r->diagonal_deal),
        r->cells,
        bool_str(r->cells_pre_filled),
        r->stock_size,
        stock_deal_str(r->stock_deal_method),
        r->stock_deal_count,
        bool_str(r->stock_redeal),
        r->reserve_size,
        bool_str(r->reserve_stacked),
        face_up_policy_str(r->face_up_policy),
        r->sequence_count,
        direction_str(r->sequence_direction),
        build_policy_str(r->sequence_build_policy),
        bool_str(r->sequence_fixed_suit),
        r->accordion_size);
}

int make_deck(ms_rules *r, ms_card_pile *buf) {
    buf->clear();
    for (unsigned i = 0; i < r->deck_count; ++i) {
        for (auto suit : SUITS) {
            for (unsigned rank = 1; rank <= r->max_rank; ++rank) {
                ms_card c;
                c.rank = rank;
                c.suit = suit;
                c.hidden = true;
                buf->push_back(c);
            }
        }
    }
    return 0;
}

int remove_card_by_rank(ms_card_pile *p, card_rank r, ms_card *buf) {
    auto i = std::find_if(p->begin(), p->end(), [r](auto &c) {
                return c.rank == r;
            });

    if (i == p->end()) {
        return 1;
    } else {
        *buf = *i;
        p->erase(i);
        return 0;
    }
}

int remove_all_by_rank(ms_card_pile *p, card_rank r) {
    auto i = std::remove_if(p->begin(), p->end(), [r](auto &c) {
                return c.rank == r;
            });
    p->erase(i, p->end());

    return 0;
}

ms_game_state random_game_state(long user_seed, ms_rules *r) {
    ms_game_state gs;

    auto seed = user_seed ? user_seed : static_cast<long unsigned>(time(0));
    std::mt19937 rng{ seed };

    ms_card_pile deck;
    make_deck(r, &deck);

    std::shuffle(deck.begin(), deck.end(), rng);

    if (r->foundations_present) {
        gs.foundations.resize(4 * r->deck_count);
        switch (r->foundations_init_cards) {
            case NO_FOUNDATION_INIT: {
                break;
            } case ONE_FOUNDATION_INIT: {
                ms_card c;
                if (r->specific_foundations_base) {
                    remove_card_by_rank(&deck, r->foundations_base, &c);
                } else {
                    c = deck.back();
                    deck.pop_back();
                }
                c.hidden = false;
                gs.foundations[0].push_back(c);
                break;
            } case ALL_FOUNDATIONS_INIT: {
                if (r->specific_foundations_base) {
                    remove_all_by_rank(&deck, r->foundations_base);
                    for (unsigned i = 0; i < r->deck_count; ++i) {
                        for (unsigned j = 0; j < 4; ++j) {
                            ms_card c;
                            c.suit = SUITS[j];
                            c.rank = r->foundations_base;
                            c.hidden = false;
                            gs.foundations[i * 4 + j].push_back(c);
                        }
                    }
                } else {
                    for (auto f : gs.foundations) {
                        ms_card c = deck.back();
                        deck.pop_back();
                        c.hidden = false;
                        f.push_back(c);
                    }
                }
                break;
            } default: {
                assert(false);
            }
        }
    }

    if (r->stock_size) {
        for (unsigned i = 0; i < r->stock_size; ++i) {
            ms_card c = deck.back();
            deck.pop_back();

            /*
             * If stock redeals are allowed, going through the stock once
             * reveals its contents without any other consequences.
             */
            c.hidden = !r->stock_redeal;

            gs.stock.push_back(c);
        }
        gs.stock.back().hidden = false;
    }

    if (r->reserve_size) {
        for (unsigned i = 0; i < r->reserve_size; ++i) {
            ms_card c = deck.back();
            deck.pop_back();
            gs.reserve.push_back(c);
        }
        gs.reserve.back().hidden = false;
    }

    // TODO: cells, accordion

    if (r->tableau_size) {
        gs.tableau.resize(r->tableau_size);

        if (r->diagonal_deal) {
            for (unsigned i = 0; i < r->tableau_size; ++i) {
                for (unsigned j = 0; j <= i; ++j) {
                    ms_card c = deck.back();
                    c.hidden = r->face_up_policy != ALL_CARDS_FACE_UP;
                    deck.pop_back();
                    gs.tableau[i].push_back(c);
                }
                gs.tableau[i].back().hidden = false;
            }
        } else {
            unsigned deep = deck.size() / r->tableau_size;
            for (unsigned i = 0; i < deep; ++i) {
                for (auto &t : gs.tableau) {
                    ms_card c = deck.back();
                    deck.pop_back();
                    c.hidden = i != deep - 1
                        && r->face_up_policy != ALL_CARDS_FACE_UP;
                    t.push_back(c);
                }
            }
        }
    }

    if (!deck.empty() && r->hole_present) {
        ms_card c = deck.back();
        c.hidden = false;
        deck.pop_back();
        gs.hole.push_back(c);
    }

    assert(deck.empty());

    gs.seed = seed;
    return gs;
}
