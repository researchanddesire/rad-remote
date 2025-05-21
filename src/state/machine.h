#include "events.hpp"
#include "actions.hpp"
#include "guards.hpp"
#include "dependencies.hpp"
#include "boost/sml.hpp"

namespace sml = boost::sml;

struct tcp_release
{
    auto operator()() const
    {
        using namespace sml;
        using namespace actions;

        return make_transition_table(
            *"established"_s + event<release> / send_fin = "fin wait 1"_s,
            "fin wait 1"_s = X,
            "fin wait 2"_s + event<fin>[is_valid<fin>] / send_ack<fin> = "timed wait"_s,
            "timed wait"_s + event<timeout> = X);
    }
};