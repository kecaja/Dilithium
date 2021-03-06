//
// Created by Jacek on 20.02.2022.
//
#include "interface.h"

/////////////////////////////////////////////////////////////////////////////////////////////
///////////// Implementation

///////////// Transaction
Transaction::Transaction(Seller *_seller, Buyer *_buyer, int _quantity, int _price):
        seller(_seller), buyer(_buyer), quantity(_quantity), price(_price), finalized(false) {}

void Transaction::finalize() {finalized = true;}

int Transaction::get_quantity() const {return quantity;}

int Transaction::get_price() const {return price;}

int Transaction::get_total_price() const {
    return quantity*price;
}

///////////// Person
Person::Person(std::string _name, int _dilithium, int _money):
        name(_name), dilithium(_dilithium), money(_money){}

void Person::print_statistics() {
    std::cout<<std::endl<<this->name<< " posiada dilithium: "<<this->dilithium<<" oraz pieniadze: "<< this->money;
}
///////////// Buyer
Buyer::Buyer(std::string _name, int _dilithium, int _money):
        Person(_name, _dilithium, _money), needed_dilithium(0){};

bool Buyer::need_buy(int limit) {
    if(this->dilithium < limit) {
        this->needed_dilithium = limit - this->dilithium;
        return true;
    }
    else return false;
}

bool Buyer::buy_from_seller(Seller *seller, Bank *bank) {
    int price = seller->get_price();
    if(!this->accept_price(price) || !seller->enough_dilithium(this->needed_dilithium))
        return false;

    auto transaction = new Transaction(seller, this, this->needed_dilithium, price);
    int id = seller->register_transaction(transaction, bank); //sprzedawca zostawia dilithium w banku
    this->deposit_money(transaction, bank);

    if(!this->execute_transaction(transaction, id, bank))
        return false;

    std::cout<<this->name<<" zakupil: "<<this->needed_dilithium<<" dilithium, po cenie: "<<transaction->get_price()<<std::endl;
    seller->get_earned_money(transaction, bank);  //tutaj w banku jeszcze bede musial zmienic;
    return true;
}
//this->needed_dilithium * price
void Buyer::deposit_money(Transaction* transaction, Bank *bank) {
    this->money -= transaction->get_total_price();
    bank->receive_money(transaction);
}

void Buyer::receive_dilithium(int quantity) {
    this->dilithium += quantity;
}

bool Buyer::execute_transaction(Transaction* transaction, int id, Bank* bank){
    if(!bank->accept_transaction(transaction, id))
        return false;
    this->dilithium += transaction->get_quantity();
    bank->return_dilithium(transaction);
    return true;
}

void Buyer::buy_dilithium(std::vector<Seller *> sellers, Bank* bank, int limit ) {
    if(!this->need_buy(limit))
        return;
    bool success = false;
    for(int i=0; i<sellers.size() && !success;i++)
        success = this->buy_from_seller(sellers[i],  bank);
}

///////////// Enterprise
Enterprise::Enterprise(std::string _name, int _dilithium, int _money):
        Buyer(_name, _dilithium, _money){};

bool Enterprise::accept_price(int price) {
    return this->needed_dilithium * price < this->money;};

///////////// Seller
int Seller::register_transaction(Transaction *transaction, Bank *bank) {
    this->dilithium -= transaction->get_quantity();

    return bank->register_transaction(transaction);
}

void Seller::get_earned_money(Transaction *transaction, Bank *bank) {
    int earning = bank->return_money(transaction);
    this->money += earning;
}

Seller::Seller(std::string _name, int _dilithium, int _money, int _price):
        Person(_name, _dilithium, _money), price(_price){};

int Seller::get_price() const {
    return price;
}

///////////// Kiligoni
Kiligoni::Kiligoni(std::string _name, int _dilithium, int _money, int _price):
        Seller(_name, _dilithium, _money, _price){};

bool Kiligoni::enough_dilithium(int quntity) {
    return quntity <= this->dilithium;
}

///////////// Ferengi
Ferengi::Ferengi(std::string _name, int _dilithium, int _money, int _price):
        Seller(_name, _dilithium, _money, _price), friends(), price_for_friend(1){};

void Ferengi::add_friend(Ferengi *firend) {
    friends.push_back(firend);};

void Ferengi::internal_transaction(Ferengi * friend_seller, int quantity) {
    if(quantity > friend_seller->dilithium) quantity = friend_seller->dilithium;
    if((quantity * friend_seller->price_for_friend) > this->money) quantity = this->money / friend_seller->price_for_friend;

    int total_price = quantity * friend_seller->price_for_friend;
    this->dilithium += quantity;
    friend_seller->dilithium -= quantity;
    this->money -= total_price;
    friend_seller->money += total_price;
}

bool Ferengi::enough_dilithium(int quantity){
    for(int i=0;this->dilithium<quantity && i<friends.size();i++){
        this->internal_transaction(friends[i],quantity - this->dilithium );
    }
    return quantity <= this->dilithium;
}

void Ferengi::print_friends() {
    std::cout<<this->name<<" ma przyjaciol: ";
    for (auto ferengi_friend: this->friends) std::cout<<ferengi_friend->name<< ", ";
    std::cout<<std::endl;
}

///////////// Bank
Bank::~Bank() {
    for(auto current: current_transactions) delete current.second;
    for(auto archived: archive) delete archived;
}

int Bank::register_transaction(Transaction * transaction) {
    this->current_transactions[++current_id] = transaction;
    this->dilithium += transaction->get_quantity();
    return this->current_id;
}

bool Bank::accept_transaction(Transaction * transaction, int id) {
    Transaction* transaction_to_accept = current_transactions[id];
    if(transaction_to_accept != transaction) return false;
    current_transactions.erase(id);
    archive.push_back(transaction);
    return true;
}

void Bank::receive_money(Transaction * transaction) {
    this->money += transaction->get_total_price();
}

void Bank::receive_dilithium(Transaction * transaction) {
    this->dilithium += transaction->get_quantity();
}

int Bank::return_money(Transaction * transaction) {
    int total_price = transaction->get_total_price();
    this->money -= total_price;
    return total_price;
}

int Bank::return_dilithium(Transaction *transaction) {
    int dilithium_to_return = transaction->get_quantity();
    this->dilithium -= dilithium_to_return;
    return dilithium_to_return;
}

void Bank::print_statistics() {
    std::cout<<"Current ID: "<<current_id<<", dilithium: "<<dilithium<<", money: "<<money<<std::endl;
}

///////////// Universe
Universe::Universe(int buyers_number, int sellers_number, int _days_num): seller_list(), buyer_list(), days_num(_days_num), bank(new Bank()){
    for (int i=0;i<buyers_number;i++)
        buyer_list.push_back(new Enterprise("Buyer" + std::to_string(i), rand()%10 * 100 + 100, rand()%10 * 100 + 100));

   int kiligoni = 0;
   int ferengi = 0;
   std::vector<Ferengi*> friends;
    for (int i=0;i<sellers_number;i++){
        if(rand()%2 == 0)
            seller_list.push_back(new Kiligoni("Sprzedajacy_Kiligoni" + std::to_string(kiligoni++), rand()%10 * 100 + 100, rand()%30 * 100 + 100, rand()%3+1));
        else{
            auto new_ferengi = new Ferengi("Sprzedajacy_Ferengi"+ std::to_string(ferengi++), rand()%20 * 100 + 100, rand()%20 * 100 + 100, rand()%3+1);
            seller_list.push_back(new_ferengi);
            friends.push_back(new_ferengi);
        }
    }
    //adding friends of Ferengi (3)
    for(int i=0;i<ferengi;i++) {
        int multiplier = i / 3;

        for (int j = multiplier * 3; j<ferengi && j < multiplier * 3 + 3; j++){
            if (i != j) friends[i]->add_friend(friends[j]);
        }
    }
}

Universe::~Universe() {
    for(auto seller: seller_list) delete seller;
    for(auto buyer: buyer_list) delete buyer;
}

void Universe::update() {
    std::cout<<"Poczatek symulacji"<<std::endl<<std::endl<<"Buyers: ";
    for(auto buyer: buyer_list) buyer->print_statistics();
    std::cout<<std::endl<<std::endl<<"Sellers: ";
    for(auto seller: seller_list) seller->print_statistics();
    std::cout<<std::endl;

    for(int i=0;i<days_num;i++){
        for(auto buyer: this->buyer_list){
            buyer->buy_dilithium(this->seller_list, this->bank, (rand()%15)*100);
        }
    }

    std::cout<<std::endl<<std::endl<<"Koniec symulacji"<<std::endl<<std::endl<<"Buyers: ";

    for(auto buyer: buyer_list) buyer->print_statistics();
    std::cout<<std::endl<<std::endl<<"Sellers: ";
    for(auto seller: seller_list) seller->print_statistics();

}


























