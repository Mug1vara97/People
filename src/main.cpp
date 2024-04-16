#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <map>
#include <random>
#include <string>

enum class Event
{
    CapableEngaging,
    BirthEngageDisengage,
    GetPregnant,
    ChildrenCount,
    TimeChildren,
    Die
};

class IDistribution {
public:
    virtual double Sample() = 0;
};

class ContinuousUniform : public IDistribution {
public:
    double Sample() override {
        return (double)rand() / RAND_MAX;
    }
};

class Poisson : public IDistribution {
private:
    int lambda;
public:
    Poisson(int lambda) : lambda(lambda) {}
    double Sample() override {
        std::poisson_distribution<int> distribution(lambda);
        return distribution(generator);
    }
private:
    std::default_random_engine generator;
};

class Normal : public IDistribution {
private:
    double mean;
    double stddev;
public:
    Normal(double mean, double stddev) : mean(mean), stddev(stddev) {}
    double Sample() override {
        std::normal_distribution<double> distribution(mean, stddev);
        return distribution(generator);
    }
private:
    std::default_random_engine generator;
};

class Exponential : public IDistribution {
private:
    double lambda;
public:
    Exponential(double lambda) : lambda(lambda) {}
    double Sample() override {
        std::exponential_distribution<double> distribution(lambda);
        return distribution(generator);
    }
private:
    std::default_random_engine generator;
};

class Male;
class Female;

class Individual {
public:
    int Age;
    int RelationAge;
    int LifeTime;
    double TimeChildren;
    Individual* Couple;

    Individual(int age) : Age(age), Couple(nullptr) {}

    bool SuitableRelation() {
        return Age >= RelationAge && Couple == nullptr;
    }

    bool SuitablePartner(Individual* individual);

    bool Engaged() {
        return Couple != nullptr;
    }

    void Disengage() {
        Couple->Couple = nullptr;
        Couple = nullptr;
        TimeChildren = 0;
    }

    bool EndRelation(std::map<Event, IDistribution*> distributions);

    void FindPartner(std::vector<Individual*> population, int currentTime, std::map<Event, IDistribution*> distributions);

    virtual std::string ToString() {
        return "Age: " + std::to_string(Age) + " Lifetime " + std::to_string(LifeTime);
    }
};

class Male : public Individual {
public:
    Male(int age) : Individual(age) {}

    std::string ToString() override {
        return Individual::ToString() + " Male";
    }
};

class Female : public Individual {
public:
    bool IsPregnant;
    double PregnancyAge;
    double ChildrenCount;

    Female(int age) : Individual(age) {}

    bool SuitablePregnancy(int currentTime);

    Individual* GiveBirth(std::map<Event, IDistribution*> distributions, int currentTime);

    std::string ToString() override {
        return Individual::ToString() + " Female";
    }
};

bool Individual::SuitablePartner(Individual* individual) {
    return ((dynamic_cast<Male*>(individual) && dynamic_cast<Female*>(this)) ||
        (dynamic_cast<Female*>(individual) && dynamic_cast<Male*>(this))) && abs(individual->Age - Age) <= 5;
}

bool Individual::EndRelation(std::map<Event, IDistribution*> distributions) {
    double sample = distributions.find(Event::BirthEngageDisengage)->second->Sample();
    if (Age >= 14 && Age <= 20 && sample <= 0.7)
        return true;
    if (Age >= 21 && Age <= 28 && sample <= 0.5)
        return true;
    if (Age >= 29 && sample <= 0.2)
        return true;
    return false;
}

void Individual::FindPartner(std::vector<Individual*> population, int currentTime, std::map<Event, IDistribution*> distributions) {
    for (auto candidate : population) {
        if (SuitablePartner(candidate) &&
            candidate->SuitableRelation() &&
            distributions.find(Event::BirthEngageDisengage)->second->Sample() <= 0.5) {
            candidate->Couple = this;
            Couple = candidate;
            double childTime = distributions.find(Event::TimeChildren)->second->Sample() * 100;
            candidate->TimeChildren = currentTime + childTime;
            TimeChildren = currentTime + childTime;
            break;
        }
    }
}

bool Female::SuitablePregnancy(int currentTime) {
    return Age >= PregnancyAge && currentTime <= TimeChildren && ChildrenCount > 0;
}

Individual* Female::GiveBirth(std::map<Event, IDistribution*> distributions, int currentTime) {
    double sample = distributions.find(Event::BirthEngageDisengage)->second->Sample();
    Individual* child = (sample > 0.5) ? static_cast<Individual*>(new Male(0)) : static_cast<Individual*>(new Female(0));
    ChildrenCount--;
    child->LifeTime = dynamic_cast<Poisson*>(distributions.find(Event::Die)->second)->Sample();
    child->RelationAge = dynamic_cast<Poisson*>(distributions.find(Event::CapableEngaging)->second)->Sample();
    if (dynamic_cast<Female*>(child) != nullptr) {
        dynamic_cast<Female*>(child)->PregnancyAge = dynamic_cast<Normal*>(distributions.find(Event::GetPregnant)->second)->Sample();
        dynamic_cast<Female*>(child)->ChildrenCount = dynamic_cast<Normal*>(distributions.find(Event::ChildrenCount)->second)->Sample();
    }
    if (Engaged() && ChildrenCount > 0) {
        TimeChildren = currentTime + dynamic_cast<Exponential*>(distributions.find(Event::TimeChildren)->second)->Sample();
        Couple->TimeChildren = TimeChildren;
    }
    else {
        TimeChildren = 0;
    }
    IsPregnant = false;
    return child;
}

class Simulation {
private:
    int Time;
    int CurrentTime;
    std::map<Event, IDistribution*> Distributions;
public:
    std::vector<Individual*> Population;
    Simulation(std::vector<Individual*> population, int time) : Population(population), Time(time), CurrentTime(0) {
        Distributions = {
            { Event::CapableEngaging, new Poisson(18) },
            { Event::BirthEngageDisengage, new ContinuousUniform() },
            { Event::GetPregnant, new Normal(28, 8) },
            { Event::ChildrenCount, new Normal(2, 6) },
            { Event::TimeChildren, new Exponential(8) },
            { Event::Die, new Poisson(70) }
        };

        for (auto individual : Population) {
            individual->LifeTime = dynamic_cast<Poisson*>(Distributions.find(Event::Die)->second)->Sample();
            individual->RelationAge = dynamic_cast<Poisson*>(Distributions.find(Event::CapableEngaging)->second)->Sample();
            if (dynamic_cast<Female*>(individual) != nullptr) {
                dynamic_cast<Female*>(individual)->PregnancyAge = dynamic_cast<Normal*>(Distributions.find(Event::GetPregnant)->second)->Sample();
                dynamic_cast<Female*>(individual)->ChildrenCount = dynamic_cast<Normal*>(Distributions.find(Event::ChildrenCount)->second)->Sample();
            }
        }
    }

    void Execute() {
        while (CurrentTime < Time) {
            for (size_t i = 0; i < Population.size(); i++) {
                Individual* individual = Population[i];
                if (dynamic_cast<Female*>(individual) && dynamic_cast<Female*>(individual)->IsPregnant) {
                    Population.push_back(dynamic_cast<Female*>(individual)->GiveBirth(Distributions, CurrentTime));
                }
                if (individual->SuitableRelation()) {
                    individual->FindPartner(Population, CurrentTime, Distributions);
                }
                if (individual->Engaged()) {
                    if (individual->EndRelation(Distributions)) {
                        individual->Disengage();
                    }
                    if (dynamic_cast<Female*>(individual) && dynamic_cast<Female*>(individual)->SuitablePregnancy(CurrentTime)) {
                        dynamic_cast<Female*>(individual)->IsPregnant = true;
                    }
                }
                if (individual->Age == individual->LifeTime) {
                    if (individual->Engaged()) {
                        individual->Disengage();
                    }
                    Population.erase(Population.begin() + i);
                    i--;
                }
                individual->Age++;
                CurrentTime++;
            }
        }
    }
};
class Database {
private:
    pqxx::connection connection;
public:
    Database(const std::string& dbname, const std::string& user, const std::string& password, const std::string& host)
        : connection("dbname=" + dbname + " user=" + user + " password=" + password + " host=" + host) {}

    void SaveIndividual(Individual* individual) {
        pqxx::work txn(connection);

        pqxx::result result = txn.exec(
            "INSERT INTO individuals (age, relation_age, lifetime, gender, pregnancy_age, children_count) VALUES (" +
txn.quote(individual->Age) + ", " +
txn.quote(individual->RelationAge) + ", " +
txn.quote(individual->LifeTime) + ", " +
txn.quote(dynamic_cast<Female*>(individual) != nullptr ? "Female" : "Male") + ", " +
((dynamic_cast<Female*>(individual) != nullptr) ? txn.quote(dynamic_cast<Female*>(individual)->PregnancyAge) : "NULL") + ", " +
((dynamic_cast<Female*>(individual) != nullptr) ? txn.quote(dynamic_cast<Female*>(individual)->ChildrenCount) : "NULL") + ")"

        );

        txn.commit();
    }

    std::vector<Individual*> LoadPopulation() {
        std::vector<Individual*> population;

        pqxx::work txn(connection);

        pqxx::result result = txn.exec("SELECT * FROM individuals");

for (const auto& row : result) {
    int age = row["age"].as<int>();
    int relationAge = row["relation_age"].as<int>();
    int lifeTime = row["lifetime"].as<int>();

    if (row["gender"].as<std::string>() == "Male") {
        population.push_back(new Male(age));
    } else {
        population.push_back(new Female(age));
        dynamic_cast<Female*>(population.back())->PregnancyAge = row["pregnancy_age"].as<double>();
        dynamic_cast<Female*>(population.back())->ChildrenCount = row["children_count"].as<double>();
    }

    population.back()->RelationAge = relationAge;
    population.back()->LifeTime = lifeTime;
}

        return population;
    }
};
int main() {

    std::string dbname = "Test";
    std::string user = "postgres";
    std::string password = "1000-7";
    std::string host = "localhost";

    Database db(dbname, user, password, host);

    std::vector<Individual*> population = db.LoadPopulation();

    Simulation sim(population, 1000);
    sim.Execute();

    for (auto individual : sim.Population) {
        db.SaveIndividual(individual);
    }

    for (auto individual : sim.Population) {
        std::cout << "Individual " << individual->ToString() << std::endl;
    }

    return 0;
}