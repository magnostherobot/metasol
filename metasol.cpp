#include <algorithm>
#include <map>
#include <random>

#include <assert.h>

#include "metasol.h"

#ifndef NDEBUG
# define debug(...) fprintf(stderr, __VA_ARGS__)
#else
# define debug(...)
#endif

card_suit SUITS[4] = { HEARTS, SPADES, CLUBS, DIAMONDS };

ms_settings fetch_default_settings() {
    ms_settings s;
    s.run_func = NULL;
    s.thoughtful_run_func = NULL;
    s.solved_func = NULL;
    s.reserved_move_count = 1u;
    s.user_data = NULL;
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

ms_card ms_str_card(std::string str) {
    ms_card card;
    card.suit = char_suit(str[1]);
    card.rank = char_rank(str[0]);
    card.hidden = true;
    return card;
}

void add_hidden(ms_card *c, std::vector<ms_card *> &cs) {
    if (c->hidden) cs.push_back(c);
}

/**
 * Fills a vector with pointers to all hidden cards in the given game state.
 */
int get_hidden_cards(ms_game_state *gs, std::vector<ms_card *> &cs) {
    // TODO: include reserve, cells, accordion
    for (auto &c : gs->stock)   add_hidden(&c, cs);
    for (auto &c : gs->waste)   add_hidden(&c, cs);
    /* for (ms_card &c : gs->reserve) add_hidden(&c, cs); */

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
    ;

    // TODO: I'm not sure how the reserve, sequence, or accordion work;
    // research required!
}

std::random_device rd;
auto rng = std::default_random_engine{rd()};

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

ms_card_pile *get_pile_by_index(ms_game_state *gs, uint8_t i) {
    if (i < gs->foundations.size()) {
        return &gs->foundations[i];
    } else {
        i -= gs->foundations.size();
    }

    if (i-- == 0) {
        return &gs->stock;
    }

    if (i-- == 0) {
        return &gs->waste;
    }

    assert (i < gs->tableau.size());
    return &gs->tableau[i];
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

int move_atop(ms_card_pile *f, ms_card_pile *t) {
    ms_card c = f->back();

    f->pop_back();

    if (!f->empty()) {
        f->back().hidden = false;
    }

    t->push_back(c);

    return 0;
}

int go_through_stock(ms_game_state *gs, int n) {
    for (int i = 0; i < n; ++i)
        move_atop(&gs->stock, &gs->waste);

    return 0;
}

int make_move(ms_game_state *gs, ms_move *m) {
    if (m->stock) {
        go_through_stock(gs, 1);
    } else {
        move_atop(m->from, m->to);
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

void run_loop(ms_game_state *gs, ms_rules *r, ms_settings *s, ms_move *votes,
        finished_state *vote_results, pthread_t *votes_tid) {
    struct thread_info t_infos[MAX_VOTES];

#ifdef NOTHREAD
    for (int i = 0; i < MAX_VOTES; ++i) {
        t_infos[i].gs = gs;
        t_infos[i].r = r;
        t_infos[i].s = s;
        t_infos[i].move_buf = &votes[i];
        t_infos[i].result = &vote_results[i];

        run_thread(&t_infos[i]);
    }
#else

    for (int i = 0; i < MAX_VOTES; ++i) {
        t_infos[i].gs = gs;
        t_infos[i].r = r;
        t_infos[i].s = s;
        t_infos[i].move_buf = &votes[i];
        t_infos[i].result = &vote_results[i];

        pthread_create(&votes_tid[i], NULL, &run_thread, &t_infos[i]);
    }

    for (int i = 0; i < MAX_VOTES; ++i)
        pthread_join(votes_tid[i], NULL);
#endif

    std::map<ms_move, int> tallied_votes;
    for (int i = 0; i < MAX_VOTES; ++i) {
        if (vote_results[i] == SOLUTION_FOUND) {
            ms_move vote = votes[i];
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

        if (vote > max_votes) {
            max_move = &move;
            max_votes = vote;
        }
    }

    if (max_votes == 0) {
        debug("no votes cast!\n");
        std::map<finished_state, int> res;

        for (int i = 0; i < MAX_VOTES; ++i) {
            finished_state f = vote_results[i];
            auto c = res.find(f);
            res[f] = (c == res.end()) ? 1 : (c->second + 1);
        }

        debug("results:\n");
        for (auto &p : res) {
            debug("%s: %d\n", finished_state_str(p.first), p.second);
        }
    } else {
        make_move(gs, max_move);
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

void ms_run(ms_game_state *gs, ms_rules *r, ms_settings *s) {
    ms_move votes[MAX_VOTES];
    finished_state vote_results[MAX_VOTES];
    pthread_t votes_tid[MAX_VOTES];

    while (!s->solved_func(gs, r, s->user_data)) {
        if (s->thoughtful_run_func && !solvable(gs, r, s)) {
            break;
        }

        run_loop(gs, r, s, &votes[0], &vote_results[0], &votes_tid[0]);
    }
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

int make_deck(ms_rules *r, ms_settings *s, ms_card_pile *buf) {
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

ms_game_state random_game_state(ms_rules *r, ms_settings *s) {
    ms_game_state gs;

    ms_card_pile deck;
    make_deck(r, s, &deck);

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

    gs.tableau.resize(r->tableau_size);
    for (auto &t : gs.tableau) {
        ms_card c = deck.back();
        deck.pop_back();
        c.hidden = false;
        t.push_back(c);
    }

    if (r->stock_size) {
        for (unsigned i = 0; i < r->stock_size; ++i) {
            ms_card c = deck.back();
            deck.pop_back();
            gs.stock.push_back(c);
        }
        gs.stock.back().hidden = false;
    }

    // TODO: cells, reserve, accordion

    assert(deck.empty());

    return gs;
}
