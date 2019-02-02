#include <algorithm>
#include <map>
#include <random>

#include <assert.h>

#include "thpool/thpool.h"

#include "metasol.h"

#ifndef NDEBUG
# define debug(...) fprintf(stderr, __VA_ARGS__)
#else
# define debug(...)
#endif

card_suit SUITS[4] = { HEARTS, SPADES, CLUBS, DIAMONDS };

void lower(char *str) {
    for (; *str; ++str) *str = tolower(*str);
}

struct dictcmp {
    bool operator()(const char *a, const char *b) {
        return strcmp(a, b) < 0;
    }
};

#define kv_dict(t) std::map<const char *, t, dictcmp>

build_policy_t str_build_policy(char *str) {
    kv_dict(build_policy_t) dict = {
        { "no build", NO_BUILD },
        { "none", NO_BUILD },

        { "same suit", BUILD_SAME_SUIT },
        { "suits", BUILD_SAME_SUIT },

        { "alternating", BUILD_ALTERNATING },
        { "red-black", BUILD_ALTERNATING },

        { "any", BUILD_ANY },
        { "all", BUILD_ANY }
    };

    lower(str);
    return dict[str];
}

spaces_policy_t str_spaces_policy(char *str) {
    kv_dict(spaces_policy_t) dict = {
        { "none", NO_SPACE_FILL },
        { "no fill", NO_SPACE_FILL },

        { "kings only", KINGS_FILL_SPACE },
        { "kings", KINGS_FILL_SPACE },

        { "any", ANY_FILL_SPACE },
        { "all", ANY_FILL_SPACE },

        { "reserve first", AUTO_RESERVE_THEN_WASTE },
        { "reserve then waste", AUTO_RESERVE_THEN_WASTE },

        { "waste first", AUTO_WASTE_THEN_STOCK },
        { "waste then stock", AUTO_WASTE_THEN_STOCK }
    };

    lower(str);
    return dict[str];
}

accordion_policy_t str_accordion_policy(char *str) {
    kv_dict(accordion_policy_t) dict = {
        { "same rank", ACCORDION_SAME_RANK },
        { "rank", ACCORDION_SAME_RANK },

        { "same suit", ACCORDION_SAME_SUIT },
        { "suit", ACCORDION_SAME_SUIT },

        { "alternating colour", ACCORDION_ALTERNATE_COLOUR },
        { "red-black", ACCORDION_ALTERNATE_COLOUR },

        { "any", ACCORDION_ANY_SUIT }
    };

    lower(str);
    return dict[str];
}

stock_deal_t str_stock_deal(char *str) {
    kv_dict(stock_deal_t) dict = {
        { "waste", STOCK_TO_WASTE },
        { "stock to waste", STOCK_TO_WASTE },

        { "tableau", STOCK_TO_TABLEAU },
        { "stock to tableau", STOCK_TO_TABLEAU }
    };

    lower(str);
    return dict[str];
}

face_up_policy_t str_face_up_policy(char *str) {
    kv_dict(face_up_policy_t) dict = {
        { "all", ALL_CARDS_FACE_UP },

        { "top", TOP_CARDS_FACE_UP },
        { "top only", TOP_CARDS_FACE_UP }
    };

    lower(str);
    return dict[str];
}

direction_t str_direction(char *str) {
    kv_dict(direction_t) dict = {
        { "left", LEFT },

        { "right", RIGHT },

        { "both", BOTH },
        { "lr", BOTH }
    };

    lower(str);
    return dict[str];
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
    return dict[str];
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
    return dict[str];
}

ms_settings fetch_default_settings() {
    ms_settings s;
    s.run_func = NULL;
    s.thoughtful_run_func = NULL;
    s.solved_func = NULL;
    s.reserved_move_count = 1u;
    s.user_data = NULL;
    s.max_concurrent_threads = 4u;
    s.max_votes = 100u;
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

std::random_device rd;
auto seed = static_cast<long unsigned int>(time(0));
auto rng = std::default_random_engine{seed};
int shuffle_hidden(ms_game_state *gs) {
    std::vector<ms_card *> hidden_card_pointers;
    get_hidden_cards(gs, hidden_card_pointers);

    std::vector<ms_card> hidden_cards;
    for (ms_card *p : hidden_card_pointers)
        hidden_cards.push_back(*p);

    std::shuffle(hidden_cards.begin(), hidden_cards.end(), rng);

    for (typeof(hidden_cards.size()) i = 0; i < hidden_cards.size(); ++i) {
        ms_card *cp = hidden_card_pointers[i];
        ms_card c = hidden_cards[i];
        *cp = c;
    }

    return 0;
}

ms_card_pile *get_pile_by_index(ms_game_state *gs, ms_rules *r, uint8_t i) {
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

void *run_thread(void *vargp) {
    struct thread_info *ti = (struct thread_info *) vargp;
    *ti->result = ti->s->run_func(ti->gs, ti->r, ti->move_buf,
            ti->s->reserved_move_count, ti->s->user_data);
    return NULL;
}

// TODO: this shouldn't be implemented as an operator - it should be its own
// function.
bool operator< (const ms_move &a, const ms_move &b) {
    // FIXME: if we want to group all moves that go through the stock together,
    // it's necessary to make them "not less than" one another.
    if (a.stock && b.stock) return false;

    if (a.from < b.from) return true;
    if (a.from > b.from) return false;
    if (a.to < b.to) return true;
    return false;
}

int intlen(int i) {
    if (i == 0) return 1;
    return floor(log10(abs(i))) + 1;
}

int move_atop(ms_card_pile *f, ms_card_pile *t, unsigned count, bool *reveal) {
    ms_card_pile temp(f->end() - count, f->end());
    f->erase(f->end() - count, f->end());
    t->insert(t->end(), temp.begin(), temp.end());

    if (!f->empty()) {
        if (reveal) {
            *reveal = *reveal || f->back().hidden;
        }
        f->back().hidden = false;
    }

    return 0;
}

int flip_waste(ms_game_state *gs) {
    auto temp = gs->waste;
    gs->waste = gs->stock;
    gs->stock = temp;
    std::reverse(gs->stock.begin(), gs->stock.end());

    return 0;
}

int go_through_stock(ms_game_state *gs, unsigned n, bool *reveal) {
    // going through the stock is treated strangely, so recalculations are
    // always required
    *reveal = true;

    if (gs->stock.size() == 0) {
        flip_waste(gs);
    }

    unsigned turn_over = std::min(n, (unsigned) gs->stock.size());

    for (unsigned i = 0; i < turn_over; ++i) {
        move_atop(&gs->stock, &gs->waste, 1u, reveal);
    }

    return 0;
}

int make_move(ms_game_state *gs, ms_rules *r, ms_move *m, bool *reveal) {
    *reveal = false;

    if (m->stock) {
        go_through_stock(gs, r->stock_deal_count, reveal);
    } else {
        ms_card_pile *from = get_pile_by_index(gs, r, m->from);
        ms_card_pile *to = get_pile_by_index(gs, r, m->to);
        move_atop(from, to, m->size, reveal);
    }
    return 0;
}

char const *finished_state_str(finished_state f) {
    switch (f) {
        case SOLUTION_FOUND: return "SOLUTION_FOUND";
        case NO_SOLUTION:    return "NO_SOLUTION";
        case TIMEOUT:        return "TIMEOUT";
        case SOLVER_OOM:     return "SOLVER_OOM";
        case CANCELLED:      return "CANCELLED";
        default: assert(false);
    }
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

void run_loop(ms_game_state *gs, ms_rules *r, ms_settings *s,
        std::vector<ms_move> *votes, finished_state *vote_results,
        ms_move *previous_move, bool *card_revealed_last_move,
        threadpool *thpool, thread_info *t_infos, ms_game_state *shuffled_states
        ) {

    for (unsigned i = 0; i < s->max_votes; ++i) {
        if (vote_results[i] == SOLUTION_FOUND
                && !votes[i].empty()
                && !*card_revealed_last_move) {
            continue;
        }

        shuffled_states[i] = *gs;
        shuffle_hidden(&shuffled_states[i]);

        t_infos[i].gs = &shuffled_states[i];
        t_infos[i].r = r;
        t_infos[i].s = s;
        t_infos[i].move_buf = &votes[i];
        t_infos[i].result = &vote_results[i];

        thpool_add_work(*thpool, (void (*)(void *)) &run_thread,
                (void *) &t_infos[i]);
    }

    thpool_wait(*thpool);

    std::map<ms_move, int> tallied_votes;
    for (unsigned i = 0; i < s->max_votes; ++i) {
        if (vote_results[i] == SOLUTION_FOUND) {
            assert(!votes[i].empty());

            ms_move vote = votes[i].back();
            votes[i].pop_back();

            auto t = tallied_votes.find(vote);
            if (t == tallied_votes.end()) {
                tallied_votes[vote] = 1;
            } else {
                tallied_votes[vote] = t->second + 1;
            }
        }
    }

    ms_move *max_move;
    int max_votes = 0;
    for (auto &v : tallied_votes) {
        ms_move move = v.first;
        int vote = v.second;

        debug("%u->%u(%u) has %d votes\n", move.from, move.to, move.size, vote);

        if (vote > max_votes) {
            max_move = &move;
            max_votes = vote;
        }
    }

    if (max_votes == 0) {
        debug("no votes cast!\n");
        std::map<finished_state, int> res;

        for (unsigned i = 0; i < s->max_votes; ++i) {
            finished_state f = vote_results[i];
            auto c = res.find(f);
            res[f] = (c == res.end()) ? 1 : (c->second + 1);
        }

        debug("results:\n");
        for (auto &p : res) {
            debug("%s: %d\n", finished_state_str(p.first), p.second);
        }
    } else {
        make_move(gs, r, max_move, card_revealed_last_move);
        *previous_move = *max_move;
    }
}

bool solvable(ms_game_state *gs, ms_rules *r, ms_settings *s) {
    finished_state fs = s->thoughtful_run_func(gs, r, s->user_data);
    bool result = fs == SOLUTION_FOUND;

    if (result) {
        debug("there is still a solution\n");
    } else {
        debug("no solution left!\n");
    }

    return result;
}

int ms_run(ms_game_state *gs, ms_rules *r, ms_settings *s) {
#define gimme_mem(t) ((t *) malloc(sizeof(t) * s->max_votes))
    std::vector<ms_move> *votes = gimme_mem(std::vector<ms_move>);
    finished_state *vote_results = gimme_mem(finished_state);
    thread_info *t_infos = gimme_mem(thread_info);
    ms_game_state *states = gimme_mem(ms_game_state);

    threadpool thpool = thpool_init(s->max_concurrent_threads);

    bool reveal = true;
    ms_move previous_move;
    previous_move.stock = true;
    while (!s->solved_func(gs, r, s->user_data)) {
        if (s->thoughtful_run_func && !solvable(gs, r, s)) {
            return 1;
        }

        run_loop(gs, r, s, votes, vote_results, &previous_move,
                &reveal, &thpool, t_infos, states);
    }
    debug("successful seed: %lu\n", seed);
    return 0;
}

ms_rules fetch_default_rules() {
    ms_rules dr;

    dr.tableau_size = 8u;
    dr.build_policy = BUILD_ANY;
    dr.spaces_policy = ANY_FILL_SPACE;
    dr.move_built_group = CANNOT_MOVE_BUILT_GROUP;
    dr.built_group_policy = dr.build_policy;

    dr.deck_count = 1u;
    dr.max_rank = 13u;

    dr.hole_present = false;

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
    // dr.accordion_moves is an empty vector
    // dr.accordion_policy is an empty vector

    return dr;
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

    std::random_device rd;
    auto seed = user_seed ? user_seed : static_cast<long unsigned>(time(0));
    auto rng = std::default_random_engine{seed};

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
                    c.hidden = i != deep - 1;
                    t.push_back(c);
                }
            }
        }
    }

    assert(deck.empty());
    return gs;
}
