#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/transaction.hpp>
#include <eosiolib/print.hpp>

using namespace eosio;

class [[eosio::contract("resource_delegating")]] example_contract : public eosio::contract {

public:
  using contract::contract;
  uint64_t autodelay = 120;
  uint64_t max_amount = 1000; // 0,1 EOS
  uint64_t delegation_counters = 0;

 struct [[eosio::table]] request {
    name username;

    uint64_t primary_key() const { return 0; }

    EOSLIB_SERIALIZE( request, (username))
  };

typedef eosio::multi_index<name("request"), request> requests;
  
  example_contract(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

  [[eosio::action]]
  void ask(name _user, uint64_t _CPU, uint64_t _NET) {
    eosio_assert(_CPU + _NET < max_amount, "Maximal allowed delegation exceeded");
    requests reqest_instance(_code, _code.value);
    auto iterator = reqest_instance.find(0);   
    if( iterator == reqest_instance.end() )
    {
      reqest_instance.emplace(_user, [&]( auto& row ) {
        row.username = _user;
      });
      delegate_resources( _user, _CPU, _NET);
      printmsg(_user, "delegated CPU and NET");
    }
    else {
      printmsg(_user, "contract is currently busy/ resources are not delegated");
      return;
    }
  }

  [[eosio::action]]
  void notify(name _user, std::string msg) {
    require_recipient(_user);
  }

  [[eosio::action]]
  void onupdate(name _user) {
    require_auth(get_self());
    printmsg(_user, std::string("Resources automatically undelegated"));
    
    requests reqest_instance(_code, _code.value);
    auto iterator = reqest_instance.find(0);
    eosio_assert(iterator != reqest_instance.end(), "Record does not exist");
    reqest_instance.erase(iterator);
  }

private:

void printmsg(name user, std::string _msg) {
  action(
    permission_level(name(get_self()), name("active")),
    get_self(),
    name("notify"),
    std::make_tuple(user, _msg)
  ).send();
}

void undelegate_resources(name _user, uint64_t _CPU, uint64_t _NET){
  eosio::transaction t{};
  t.delay_sec = autodelay;

  asset CPU_delegated(_CPU, symbol("EOS",4));
  asset NET_delegated(_NET, symbol("EOS",4));

    t.actions.emplace_back(
      permission_level(name(get_self()), name("active")), 
      name("eosio"),
      name("undelegatebw"), 
      std::tuple(get_self(), _user, CPU_delegated, NET_delegated, false));

    t.actions.emplace_back(
      permission_level(name(get_self()), name("active")), 
      name(get_self()),
      name("onupdate"), 
      std::tuple(get_self()));

    t.send(now(), name(get_self()));
  };

void delegate_resources(name _user, uint64_t _CPU, uint64_t _NET){

  asset CPU_to_delegate(_CPU, symbol("EOS",4));
  asset NET_to_delegate(_NET, symbol("EOS",4));
    action(
      permission_level(name(get_self()), name("active")), 
      name("eosio"),
      name("delegatebw"), 
      std::tuple(get_self(), _user, CPU_to_delegate, NET_to_delegate, false)
    ).send();

    undelegate_resources(_user, _CPU, _NET);
    delegation_counters++;

  };
};

EOSIO_DISPATCH( example_contract, (ask)(notify)(onupdate) )
