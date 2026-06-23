#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>

/* Структура перевода по имени
struct Transaction
{
    std::string from; //string содержится в пространстве имён std, как и vector, map, cout и др.
    std::string to;
    double amount;
};
*/
struct Transaction
{
    int fromId;
    int toId;
    double amount;
};
class Account // абстрактный класс
{
public:
    virtual ~Account() = default; //деструктор - обязателен для полиморфных базовых классов

    Account(int id, std::string owner, double balance)
        :id(id), owner(owner), balance(balance) //инициализация полей до тела конструктора. Особо эффективно с const
    {
        if(balance < 0) throw std::invalid_argument("Balance cannot be negative\n");
        /*
        То же самое, что и :owner(owner), balance(balance). Можно и так, и так
        this -> owner = owner; //this - указатель, нужен для различения поля класса и параметра конструкрора ((*this).owner = owner;)
        this -> balance = balance;
        */
    }
    virtual void print() const{
        std::cout << "ID: "        << id
                  << "\nOwner: "     << owner
                  << "\nBalance: " << balance
                  << '\n';
    }
    virtual std::string getType() const = 0;
    void deposit(double amount){ // пополнение на сумму, одинакого для всех счетов
       if(amount <= 0) throw std::invalid_argument("Amount cannot be negative\n"); 
       balance += amount;
    }

    virtual bool withdraw(double amount) = 0;

    int getId() const{
        return id;
    }

    const std::string& getOwner() const{
        return owner;
    }

    double getBalance() const{
        return balance;
    }

protected:
    int id;            // id (ᵔᗜᵔ)
    std::string owner; // владелец
    double balance;    // баланс
};

class DebitAccount : public Account
{
public:
    DebitAccount(int id, std::string owner, double balance)
        : Account(id, owner, balance)
    {

    }
    bool withdraw(double amount) override{ // снятия со счёта
        if(balance < amount || amount <= 0) return false;
        balance -= amount;
        return true;
    }
    std::string getType() const override{
        return "Debit";
    }

};
class CreditAccount : public Account
{
public:
    CreditAccount(int id, std::string owner, double balance, double creditLimit)
        : Account(id, owner, balance), creditLimit(creditLimit)
    {
        if(creditLimit < 0) throw std::invalid_argument("Credit limit cannot be negative\n");
    }
    bool withdraw(double amount) override{
        if (balance - amount >= -creditLimit){
            balance -= amount;
            return true;
        }
        return false;
    }
    void print() const override{
        Account::print();
        std::cout << "Credit Limit: " << creditLimit
                  << '\n';
    }
    std::string getType() const override{
        return "Credit";
    }
    double getCreditLimit() const{
        return creditLimit;
    }
private:
    double creditLimit;
};
class Bank
{
public:
    void addAccount(std::unique_ptr<Account> acc){
        // accounts.push_back(std::move(acc)); // std::move(acc) разрешает владеть ресурсом, а не владеет им, что важно для умного указателя ()
        accounts.emplace(acc -> getId(), std::move(acc)); //работа с вектором умных указателей
    }

    void printAll() const{
        std::cout<<'\n';
        for (const auto& [id,acc] : accounts){ // auto - компилятор сам понимает тип (умный указатель), & - ссылка(копия), т.к. просто для просмотра
            acc->print(); //вызывается print() из того класса, который подходит
            std::cout<<"-----------------\n";
        }
    }

    /* Перевод между счетами по имени
    bool transfer(const std::string& from, const std::string& to, double amount){
        if(amount <= 0 || from == to) return false;
        Account* sender = findAccount(from);
        Account* receiver = findAccount(to);
        if(!sender || !receiver) return false;
        if(sender -> withdraw(amount)) { // если можно вычесть без проблем, то вычитаем
            receiver -> deposit(amount); // прибавляем к другому счёту
            history.push_back({from, to, amount}); //history.push_back(Transaction{from, to, amount}) - создаём структуру и стазу ее добавляем 
            return true;
        }
        return false;
    }
    */

    bool transfer(int fromId, int toId, double amount){
        if(amount <= 0 || fromId == toId) return false;
        Account* sender = findAccount(fromId);
        Account* receiver = findAccount(toId);
        if(!sender || !receiver) return false;
        if(sender -> withdraw(amount)) { // если можно вычесть без проблем, то вычитаем
            receiver -> deposit(amount); // прибавляем к другому счёту
            history.push_back({fromId, toId, amount}); //history.push_back(Transaction{from, to, amount}) - создаём структуру и стазу ее добавляем 
            return true;
        }
        return false;
    }
    void printHistory() const{ // история не меняется при печати
        std::cout << "\nTransaction history: \n" ;
        for(const auto& tr : history){
            //std::cout << tr.from << " -> " << tr.to << " : " << tr.amount << '\n'; // Вывод истории при переводе по имени
            std::cout << tr.fromId << " -> " << tr.toId << " : " << tr.amount << '\n'; // Вывод истории при переводе по id
        }
    }

    /* Поиск по имени
    Account* findAccount(const std::string& name) const{ //просто ищем, ничего не меняем
        for(const auto& acc : accounts){
            if(acc->getOwner() == name) return acc.get(); //возвращает сырой указателью Мы не можем вернуть unique_ptr, ведь unique_ptr для одного владельца - Банка, передать его значит передать владение
        }
        return nullptr;
    }
    */
    /*Поиск по ID*/
    Account* findAccount(int id) const{ //просто ищем, ничего не меняем
        auto it = accounts.find(id);
        if(it != accounts.end()) return it -> second.get();
        return nullptr;
    }

    void createDebitAccount(const std::string& owner, double balance){
        addAccount(std::make_unique<DebitAccount>(generateId(), owner, balance)); // до этого было bank.addAccount(std::make_unique<CreditAccount>(1000,"Anna", 1000, 5000));
    }
    void createCreditAccount(const std::string& owner, double balance, double creditLimit){
        addAccount(std::make_unique<CreditAccount>(generateId(), owner, balance, creditLimit));
    }

    bool removeAccount(int removeId){
        return accounts.erase(removeId) > 0;
        // for(auto i = accounts.begin(); i != accounts.end(); ++i){ //accounts.end() не указывает на элемент
        //     if((*i) -> getId() == removeId) {
        //         accounts.erase(i);
        //         return true;
        //     }
        // }
        // return false;
    }
    void saveToFile(const std::string& filename){
        std::ofstream file(filename);
        if(!file) throw std::runtime_error("Cannot open file");
        for(const auto& [id, acc] : accounts){
            if(acc->getType() == "Debit"){
                file
                    << acc->getId() << ' '
                    << acc->getOwner() << ' '
                    << acc->getBalance() << ' '
                    << "Debit"
                    << '\n'; 
            }
            else{
                auto* credit = dynamic_cast<CreditAccount*>(acc.get()); //из-за полиморфизма мы не можем на прямую обращаться к getCreditLimit, поэтому как бы заходим во внутрь класса
                file
                    << credit->getId() << ' '
                    << credit->getOwner() << ' '
                    << credit->getBalance() << ' '
                    << credit->getCreditLimit() << ' '
                    << "Credit"
                    << '\n'; 
            }
        }
        
    }
    void loadFromFile(const std::string& filename){
        std::ifstream file(filename);
        if(!file) return;
        accounts.clear(); //очищаем старые данные
        std::string line; //чистраем построчно
        
        while(std::getline(file, line)){
            std:: istringstream iss(line);

            std::vector <std::string> fields; 
            std::string field;

            while(iss >> field){
                fields.push_back(field); // преобразование, т.к. строчки с разным кол-вол элементов
            }

            if(fields.size() == 4){
                int id = std::stoi(fields[0]); //преобразование строки в int
                std::string owner = fields[1];
                double balance = std::stoi(fields[2]);

                accounts.emplace(id, std::make_unique<DebitAccount>(id, owner, balance));
            }
            else if(fields.size() == 5){
                int id = std::stoi(fields[0]); //преобразование строки в int
                std::string owner = fields[1];
                double balance = std::stoi(fields[2]);
                double creditLimit = std::stod(fields[3]);

                accounts.emplace(id, std::make_unique<CreditAccount>(id, owner, balance, creditLimit));
            }

        }
    }

private:
    std::unordered_map<int, std::unique_ptr<Account>> accounts; // вектор умных указателей на аккаунт (создаём аккаунты, которые потом сами удаляться)
    std::vector<Transaction> history; // история операций
    int nextId = 1000;

    int generateId(){
        return nextId++;
    }

};
  
int main()
{
    Bank bank;
    // bank.createCreditAccount("Anna", 1000, 5000);
    // bank.createCreditAccount("Tim", 7000, 3000);
    // bank.createDebitAccount("Roma", 7000);
    // bank.createDebitAccount("Tom", 300);
    // bank.saveToFile("accounts.txt");

    // bank.removeAccount(1002);
    // bank.printAll();

    bank.loadFromFile("accounts.txt");
    bank.printAll();
    return 0;
}