#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <random>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

#include "src/main/api.h"

#ifndef NDEBUG
# define debug(...) fprintf(stderr, __VA_ARGS__)
#else
# define debug(...)
#endif

enum build_policy_t {
    NO_BUILD, BUILD_SAME_SUIT, BUILD_ALTERNATING, BUILD_ANY
};

sol_rules::build_policy convert_build_policy(build_policy_t bp) {
    switch (bp) {
        case NO_BUILD:          return sol_rules::build_policy::NO_BUILD;
        case BUILD_SAME_SUIT:   return sol_rules::build_policy::SAME_SUIT;
        case BUILD_ALTERNATING: return sol_rules::build_policy::RED_BLACK;
        case BUILD_ANY:         return sol_rules::build_policy::ANY_SUIT;
        default: assert(false);
    }
}

enum spaces_policy_t {
    NO_SPACE_FILL, KINGS_FILL_SPACE, ANY_FILL_SPACE, AUTO_RESERVE_THEN_WASTE,
    AUTO_WASTE_THEN_STOCK
};

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

enum accordion_policy_t {
    ACCORDION_SAME_RANK, ACCORDION_SAME_SUIT, ACCORDION_ALTERNATE_COLOUR,
    ACCORDION_ANY_SUIT
};

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

enum stock_deal_t {
    STOCK_TO_WASTE, STOCK_TO_TABLEAU
};

sol_rules::stock_deal_type convert_stock_deal(stock_deal_t sd) {
    switch (sd) {
        case STOCK_TO_WASTE:   return sol_rules::stock_deal_type::WASTE;
        case STOCK_TO_TABLEAU: return sol_rules::stock_deal_type::TABLEAU_PILES;
        default: assert(false);
    }
}

enum face_up_policy_t {
    ALL_CARDS_FACE_UP, TOP_CARDS_FACE_UP
};

sol_rules::face_up_policy convert_face_up_policy(face_up_policy_t fup) {
    switch (fup) {
        case ALL_CARDS_FACE_UP: return sol_rules::face_up_policy::ALL;
        case TOP_CARDS_FACE_UP: return sol_rules::face_up_policy::TOP_CARDS;
        default: assert(false);
    }
}

enum direction_t {
    LEFT, RIGHT, BOTH
};

sol_rules::direction convert_direction(direction_t d) {
    switch (d) {
        case LEFT:  return sol_rules::direction::LEFT;
        case RIGHT: return sol_rules::direction::RIGHT;
        case BOTH:  return sol_rules::direction::BOTH;
        default: assert(false);
    }
}

enum built_group_t {
    CAN_MOVE_BUILT_GROUP, CANNOT_MOVE_BUILT_GROUP, CAN_MOVE_WHOLE_PILE,
    CAN_MOVE_MAXIMAL_GROUP
};

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

enum foundations_init_t {
    NO_FOUNDATION_INIT, ONE_FOUNDATION_INIT, ALL_FOUNDATIONS_INIT
};

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

typedef card::suit_t card_suit;
typedef card::rank_t card_rank;

typedef struct srules {
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
} srules;

sol_rules srules_rules(srules *sr) {
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

    if (sr->specific_foundations_base)
        r.foundations_base = sr->foundations_base;
    else
        r.foundations_base = boost::optional<card::rank_t>{};

    for (auto &p : sr->accordion_moves)
        r.accordion_moves.push_back(std::pair<sol_rules::direction, uint8_t>
                (convert_direction(p.first), p.second));

    for (auto &x : sr->accordion_policy)
        r.accordion_pol.push_back(convert_accordion_policy(x));

    return r;
}

typedef struct scard {
    card_suit suit;
    card_rank rank;
    bool hidden;
} scard;

typedef std::vector<scard> scard_pile;

typedef struct sgame_state {
    std::vector<scard_pile> foundations;
    scard_pile stock;
    scard_pile waste;
    std::vector<scard_pile> tableau;
    scard_pile hole;
    std::vector<scard_pile> cells;
} sgame_state;

typedef struct smove {
    bool stock;
    scard_pile *to;
    scard_pile *from;
} smove;

typedef finished_state thoughtful_run_func_t(sgame_state *,
        srules *, void *);
typedef finished_state run_func_t(sgame_state *, srules *, smove *,
        unsigned, void *);

typedef struct settings {
    run_func_t *run_func;
    thoughtful_run_func_t *thoughtful_run_func;
    std::size_t move_size;
    unsigned reserved_move_count;
    void *user_data;
} settings;

settings fetch_default_settings() {
    settings s;
    s.run_func = NULL;
    s.thoughtful_run_func = NULL;
    s.move_size = 0u;
    s.reserved_move_count = 1u;
    s.user_data = NULL;
    return s;
}

settings DEFAULT_SETTINGS = fetch_default_settings();

typedef std::vector<std::string> card_pile;

std::string SUITS[4] = { "S", "C", "D", "H" };
std::string RANKS[13] = {
    "A", "2", "3", "4", "5", "6", "7", "8", "9", "X", "J", "Q", "K"
};

card_pile& make_deck(card_pile& space, int deck_count);
card_pile& make_deck(card_pile& space, int deck_count = 1) {
    space.clear();
    for (int i = 0; i < deck_count; ++i) {
        for (std::string suit : SUITS) {
            for (std::string rank : RANKS) {
                space.push_back(rank + suit);
            }
        }
    }
    return space;
}

std::random_device rd;
auto rng = std::default_random_engine{rd()};
card_pile& shuffle(card_pile& cards) {
    std::shuffle(std::begin(cards), std::end(cards), rng);
    return cards;
}

std::vector<card_pile>& split(card_pile& in, std::vector<card_pile>& out,
        std::vector<std::size_t> numbers) {
    out.clear();
    out.resize(numbers.size());

    std::size_t acc = 0;
    auto i = 0;
    for (auto& n : numbers) {
        card_pile cp(in.begin() + acc, in.begin() + acc + n);
        out[i] = cp;
        acc += n;
        ++i;
    }

    assert(acc == in.size());
    return out;
}

std::vector<card_pile>& split_even(card_pile& in, std::vector<card_pile>& out,
        std::size_t n_piles);
std::vector<card_pile>& split_even(card_pile& in, std::vector<card_pile>& out,
        std::size_t n_piles) {
    std::vector<std::size_t> nums;

    auto t = in.size();
    for (int i = 0; i < (int)in.size(); ++i) {
        auto w = t / (n_piles - i);
        nums.push_back(w);
        t -= w;
    }

    return split(in, out, nums);
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
        case 'H': case 'h': return card::suit::Hearts;
        case 'S': case 's': return card::suit::Spades;
        case 'C': case 'c': return card::suit::Clubs;
        case 'D': case 'd': return card::suit::Diamonds;
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
    }

    assert(false);
}

std::string rank_compat_str(card_rank rank) {
    switch (rank) {
        case  1: return  "A";
        case  2: return  "2";
        case  3: return  "3";
        case  4: return  "4";
        case  5: return  "5";
        case  6: return  "6";
        case  7: return  "7";
        case  8: return  "8";
        case  9: return  "9";
        case 10: return "10";
        case 11: return  "J";
        case 12: return  "Q";
        case 13: return  "K";
    }

    assert(false);
}

char suit_char(card_suit s) {
    if      (s == card::suit::Hearts  ) return 'H';
    else if (s == card::suit::Spades  ) return 'S';
    else if (s == card::suit::Clubs   ) return 'C';
    else if (s == card::suit::Diamonds) return 'D';

    assert(false);
}

scard str_scard(std::string str) {
    scard card;
    card.suit = char_suit(str[1]);
    card.rank = char_rank(str[0]);
    card.hidden = true;
    return card;
}

bool strs_scards(card_pile &cp, scard_pile &out) {
    std::transform(cp.begin(), cp.end(), out.begin(), str_scard);
    return true;
}

std::string scard_compat_str(scard &c) {
    std::string rank_str = rank_compat_str(c.rank);
    char suit_c = suit_char(c.suit);
    return rank_str + suit_c;
}

int scard_str(scard *c, char *buf) {
    char rank_c = rank_char(c->rank);
    char suit_c = suit_char(c->suit);
    buf[0] = rank_c;
    buf[1] = suit_c;
    buf[2] = '\0';
    return 0;
}

void add_hidden(scard *c, std::vector<scard *> &cs) {
    if (c->hidden) cs.push_back(c);
}

/**
 * Fills a vector with pointers to all hidden cards in the given game state.
 */
int get_hidden_cards(sgame_state *gs, std::vector<scard *> &cs) {
    for (scard &c : gs->stock)   add_hidden(&c, cs);
    for (scard &c : gs->waste)   add_hidden(&c, cs);
    /* for (scard &c : gs->reserve) add_hidden(&c, cs); */

    for (scard_pile &p : gs->foundations)
        for (scard &c : p)
            add_hidden(&c, cs);
    for (auto &p : gs->tableau)
        for (auto &c : p)
            add_hidden(&c, cs);

    return cs.size();
}

bool moves_equal(move &a, move &b) {
    bool res = a.from  == b.from
        && a.to == b.to
        && a.count == b.count;

    if (res) {
        // if the above values are the same,
        // the following values are necessarily equal:
        assert(a.reveal_move == b.reveal_move);
        assert(a.flip_waste == b.flip_waste);
    }

    return res;
}

typedef std::vector<movelist> paths;
typedef std::vector<paths> grouped_paths;

/*
 * Returns false if no place to put new path
 */
bool add_group_by_first_move(grouped_paths &groups, movelist &path) {
    assert(!path.empty());

    bool found = false;
    for (auto &group : groups) {
        movelist first = group[0];
        assert(!first.empty());

        if (group.empty() || moves_equal(path[0], first[0])) {
            group.push_back(path);
            found = true;
            break;
        }
    }

    return found;
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

bool goes_through_stock(move *a) {
    return a->type == move::mtype::stock_k_plus
        && a->count != 0;
}

void remove_cards(card_pile &cp, std::vector<std::string> cs) {
    cp.erase(std::remove_if(cp.begin(), cp.end(),
                [cs](std::string c){
                    return std::find(cs.begin(), cs.end(), c) != cs.end();
                }), cp.end());
}

int sgs_pile(scard_pile &s, card_pile &p) {
    p.resize(s.size());
    std::transform(s.begin(), s.end(), p.begin(), scard_compat_str);

    return 0;
}

auto foundation_count(srules *r) {
    if (r->foundations_present) {
        return 4u * r->deck_count;
    } else {
        return 0u;
    }
}

auto total_pile_count(srules *r) {
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

void do_pile(scard_pile *p,
        std::vector<std::vector<card_suit>> *suits,
        std::vector<std::vector<card_rank>> *ranks,
        std::vector<std::vector<bool>> *face_down,
        bool force_face_up = false) {
    suits->push_back({});
    ranks->push_back({});
    face_down->push_back({});

    for (auto &c : *p) {
        suits->back().push_back(c.suit);
        ranks->back().push_back(c.rank);
        face_down->back().push_back(force_face_up ? false : c.hidden);
    }
}

void do_pile_set(std::vector<scard_pile> *set,
        std::vector<std::vector<card_suit>> *suits,
        std::vector<std::vector<card_rank>> *ranks,
        std::vector<std::vector<bool>> *face_down,
        bool force_face_up = false) {
    for (auto &p : *set) {
        do_pile(&p, suits, ranks, face_down, force_face_up);
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

game_state sgs_gs(sgame_state *sgs, srules *r) {
    std::vector<std::vector<card_suit>> suits;
    std::vector<std::vector<card_rank>> ranks;
    std::vector<std::vector<bool>> face_down;

    if (r->hole_present) {
        do_pile(&sgs->hole, &suits, &ranks, &face_down);
    }

    do_pile_set(&sgs->foundations, &suits, &ranks, &face_down);
    do_pile_set(&sgs->cells, &suits, &ranks, &face_down);
    if (r->stock_size) {
        // solvitaire requires that the stock be entirely face-up
        do_pile(&sgs->stock, &suits, &ranks, &face_down, true);
        if (r->stock_deal_method == STOCK_TO_WASTE) {
            do_pile(&sgs->waste, &suits, &ranks, &face_down);
        }
    }
    // TODO: how does the reserve work? leaving empty for now
    // TODO: how does the accordion work? leaving empty for now
    do_pile_set(&sgs->tableau, &suits, &ranks, &face_down);
    // TODO: how does the sequence work? leaving empty for now

    game_state gs(srules_rules(r), suits, ranks, face_down);
    return gs;
}

typedef struct user_data {
    unsigned run_cache_size;
    unsigned run_timeout;
    unsigned thoughtful_run_timeout;
} user_data;

void print_sgs(sgame_state *gs, srules *r) {
    game_state solv_gs = sgs_gs(gs, r);
    std::cerr << solv_gs << std::endl;
}

int shuffle_hidden(sgame_state *gs) {
    std::vector<scard *> hidden_card_pointers;
    get_hidden_cards(gs, hidden_card_pointers);

    std::vector<scard> hidden_cards;
    for (scard *p : hidden_card_pointers)
        hidden_cards.push_back(*p);

    std::shuffle(hidden_cards.begin(), hidden_cards.end(), rng);

    for (typeof(hidden_cards.size()) i = 0; i < hidden_cards.size(); ++i) {
        scard *cp = hidden_card_pointers[i];
        scard c = hidden_cards[i];
        *cp = c;
    }

    return 0;
}

bool is_dud_move(move *m) {
    if (m->to == 255u && m->from == 255u) return true;
    return false;
}

scard_pile *get_pile_by_index(sgame_state *gs, uint8_t i) {
    if (i < gs->foundations.size())
        return &gs->foundations[i];
    else
        i -= gs->foundations.size();

    if (i-- == 0)
        return &gs->stock;

    if (i-- == 0)
        return &gs->waste;

    assert (i < gs->tableau.size());
    return &gs->tableau[i];
}

int move_smove(sgame_state *gs, move *m, smove *sm) {
    sm->stock = goes_through_stock(m);

    if (m->type == move::mtype::stock_k_plus) {
        sm->from = &gs->waste;
    } else {
        sm->from = get_pile_by_index(gs, m->from);
    }

    sm->to = get_pile_by_index(gs, m->to);

    return 0;
}

/**
 * The function that is called by every voter, to get its vote.
 */
finished_state run_single(sgame_state *gs, srules *r,
        smove *first_move, unsigned move_count, void *data) {
    // TODO: this function can't currently handle producing multiple moves at
    // once
    assert(move_count == 1u);

    user_data *d = (user_data *) data;

    game_state solv_gs = sgs_gs(gs, r);

    movelist ml;
    finished_state result = get_moves(ml, solv_gs, d->run_cache_size,
            d->run_timeout);

    if (result == SOLUTION_FOUND) {
        if (!is_dud_move(&ml[0])) {
            move_smove(gs, &ml[0], first_move);
        } else {
            move_smove(gs, &ml[1], first_move);
        }
    }

    return result;
}

struct thread_info {
    sgame_state *gs;
    srules *r;
    settings *s;
    smove *move_buf;
    finished_state *result;
};

void *run_thread(void *vargp) {
    struct thread_info *ti = (struct thread_info *) vargp;
    *ti->result = ti->s->run_func(ti->gs, ti->r, ti->move_buf,
            ti->s->reserved_move_count, ti->s->user_data);
    return NULL;
}

// TODO: this shouldn't be implemented as an operator - it should be its own
// function.
bool operator< (const smove &a, const smove &b) {
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

void debug_pile(scard_pile *p) {
    char buf[3];

    for (auto c : *p) {
        scard_str(&c, &buf[0]);
        debug("%s ", &buf[0]);
    }

    debug("\n");
}

char *debug_scard_buf(scard *c) {
    static char buf[3];
    scard_str(c, &buf[0]);
    return &buf[0];
}

int move_atop(scard_pile *f, scard_pile *t) {
    scard c = f->back();
    debug("moving card %s\n", debug_scard_buf(&c));

    f->pop_back();
    // XXX: I'm not 100% sure that face-down cards at the top of piles should be
    // flipped face-up, but I'm going with it for now:
    if (!f->empty())
        f->back().hidden = false;

    t->push_back(c);

    return 0;
}

int go_through_stock(sgame_state *gs, int n) {
    for (int i = 0; i < n; ++i)
        move_atop(&gs->stock, &gs->waste);

    return 0;
}

int make_move(sgame_state *gs, smove *m) {
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

const int MAX_VOTES = 100;
const int MIN_VOTES = 50;

int total_piles(sgame_state *gs) {
    int t = 0;
    t += gs->foundations.size();
    t += 1; // stock
    t += 1; // waste
    t += gs->tableau.size();
    return t;
}

void run_loop(sgame_state *gs, srules *r, settings *s, smove *votes,
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

    std::map<smove, int> tallied_votes;
    for (int i = 0; i < MAX_VOTES; ++i) {
        if (vote_results[i] == SOLUTION_FOUND) {
            smove vote = votes[i];
            auto t = tallied_votes.find(vote);
            if (t == tallied_votes.end()) {
                tallied_votes[vote] = 1;
            } else {
                tallied_votes[vote] = t->second + 1;
            }
        }
    }

    smove *max_move;
    int max_votes = 0;
    for (auto &v : tallied_votes) {
        smove move = v.first;
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
        print_sgs(gs, r);
    }
}

bool solved(sgame_state *gs, srules *r) {
    return sgs_gs(gs, r).is_solved();
}

finished_state thoughtful_run(sgame_state *gs, srules *r,
        void *data) {
    game_state solv_gs = sgs_gs(gs, r);
    movelist ml;
    return get_moves(ml, solv_gs, 1024u, 10000u);
}

bool solvable(sgame_state *gs, srules *r, settings *s) {
    finished_state fs = s->thoughtful_run_func(gs, r, s->user_data);
    bool result = fs == SOLUTION_FOUND;

    if (result) {
        debug("there is still a solution\n");
    } else {
        debug("no solution left!\n");
    }

    return result;
}

void run(sgame_state *gs, srules *r, settings *s) {
    smove votes[MAX_VOTES];
    finished_state vote_results[MAX_VOTES];
    pthread_t votes_tid[MAX_VOTES];

    while (!solved(gs, r)) {
        if (s->thoughtful_run_func) {
            if (!solvable(gs, r, s)) {
                break;
            }
        }

        run_loop(gs, r, s, &votes[0], &vote_results[0], &votes_tid[0]);
    }
    print_sgs(gs, r);
}

srules fetch_default_rules() {
    srules dr;

    dr.tableau_size = 8u;
    dr.build_policy = BUILD_ANY;
    dr.spaces_policy = ANY_FILL_SPACE;
    dr.diagonal_deal = false;
    dr.move_built_group = CANNOT_MOVE_BUILT_GROUP;
    dr.built_group_policy = dr.build_policy;
    dr.face_up_policy = ALL_CARDS_FACE_UP;
    dr.max_rank = 13u;

    dr.foundations_present = true;
    dr.foundations_init_cards = NO_FOUNDATION_INIT;
    dr.specific_foundations_base = true;
    dr.foundations_base = 1u;
    dr.foundations_removable = false;
    dr.foundations_only_comp_piles = false;

    dr.hole_present = false;

    dr.cells = 0u;

    dr.stock_size = 0u;
    dr.stock_deal_method = STOCK_TO_WASTE;
    dr.stock_deal_count = 1u;
    dr.stock_redeal = false;

    dr.reserve_size = 0u;
    dr.reserve_stacked = false;

    dr.accordion_size = 0;
    // dr.accordion_moves is an empty vector
    // dr.accordion_policy is an empty vector

    return dr;
}

srules DEFAULT_RULES = fetch_default_rules();

srules fortunes_favor() {
    // default rules
    srules r = DEFAULT_RULES;

    // rules specific to fortunes favor
    r.tableau_size = 12u;
    r.build_policy = BUILD_SAME_SUIT;
    r.spaces_policy = AUTO_WASTE_THEN_STOCK;
    r.foundations_init_cards = ALL_FOUNDATIONS_INIT;
    r.stock_size = 36u;

    return r;
}

settings get_settings(user_data *d) {
    settings s = DEFAULT_SETTINGS;

    s.run_func = &run_single;
    s.thoughtful_run_func = &thoughtful_run;

    s.move_size = sizeof(move);
    s.reserved_move_count = 1u;

    s.user_data = d;

    return s;
}

user_data get_user_data() {
    user_data d;

    d.run_cache_size = 1024u;
    d.run_timeout = 3000u;
    d.thoughtful_run_timeout = 10000u;

    return d;
}

int main() {
    srules r = fortunes_favor();
    user_data d = get_user_data();
    settings s = get_settings(&d);

    card_pile cp;
    make_deck(cp);
    remove_cards(cp, { "AS", "AD", "AH", "AC" });
    shuffle(cp);

    sgame_state gs;
    gs.foundations.resize(4);
    gs.foundations[0].push_back(str_scard("AS"));
    gs.foundations[1].push_back(str_scard("AD"));
    gs.foundations[2].push_back(str_scard("AH"));
    gs.foundations[3].push_back(str_scard("AD"));

    gs.tableau.resize(r.tableau_size);

    gs.stock.resize(cp.size());
    strs_scards(cp, gs.stock);

    // Aces and top stock card are face-up
    for (auto &f : gs.foundations)
        f.back().hidden = false;
    gs.stock.back().hidden = false;

    run(&gs, &r, &s);
}

/* int main() { */
/*     auto rules = rules_parser::from_preset("fortunes-favor"); */
/*     /1* auto rules = rules_parser::from_preset("canfield"); *1/ */

/*     card_pile cp; */
/*     std::vector<card_pile> t; */

/*     make_deck(cp); */
/*     shuffle(cp); */

/*     remove_cards(cp, { "AS", "AD", "AH", "AC" }); */

/*     piles ps = piles{ */
/*         {"AS"}, {"AD"}, {"AH"}, {"AC"}, // Foundations */
/*         { cp }, {}, // Stock-waste */
/*         {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {} // Tableau */
/*     }; */

/*     movelist ml; */

/*     game_state gs(rules, ps); */
/*     finished_state result = get_moves(ml, gs, 1024u, 3000u); */

/*     printf("%d\n", result); */

/*     switch (result) { */
/*         case SOLUTION_FOUND: { */
/*                 printf("no. of moves: %d\n", ml.size()); */

/*                 size_t bufsize = 10; */
/*                 char buf[bufsize]; */
/*                 bool first = true; */
/*                 for (auto &m : ml) { */
/*                     int required = move_as_str(m, &buf[0], bufsize); */
/*                     assert(required < bufsize); */

/*                     printf("%s\n", buf); */

/*                     if (!first) gs.make_move(m); */
/*                     first = false; */
/*                 } */

/*                 std::cerr << "Final layout:" << std::endl << gs << std::endl; */
/*                 // printf("final layout:\n%s\n", gs.to_string()); */
/*                 break; */
/*             } */
/*         case NO_SOLUTION: */
/*             printf("no solution\n"); */
/*             break; */
/*         case TIMEOUT: */
/*             printf("solver ran out of time\n"); */
/*             break; */
/*         case SOLVER_OOM: */
/*             printf("solver ran out of allocated memory\n"); */
/*             break; */
/*         case CANCELLED: */
/*             printf("solver was interrupted\n"); */
/*             break; */
/*         default: */
/*             assert(false); */
/*     } */

/*     return 0; */
/* } */
