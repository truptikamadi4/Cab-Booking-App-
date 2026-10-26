// database.h - File Management Module (CSV based persistence)
#ifndef DATABASE_H
#define DATABASE_H

#include <fstream>
#include <sys/stat.h>
#include <algorithm>

#include "models.h"
#include "utils.h"

class Database {
public:
    std::vector<User> users;
    std::vector<Driver> drivers;
    std::vector<Trip> trips;
    int nextTripId = 1001;

    explicit Database(const std::string& dir = "data") : dir_(dir) {}

    // ---------- lifecycle ----------
    void load() {
        makeDir(dir_);
        makeDir("receipts");
        loadUsers();
        loadDrivers();
        loadTrips();
    }

    void saveAll() {
        saveUsers();
        saveDrivers();
        saveTrips();
    }

    // ---------- lookups ----------
    User* findUser(const std::string& phone) {
        for (auto& u : users)
            if (u.phone == phone) return &u;
        return nullptr;
    }
    Driver* findDriver(const std::string& id) {
        std::string want = util::toLower(id);
        for (auto& d : drivers)
            if (util::toLower(d.id) == want) return &d;
        return nullptr;
    }
    Trip* findTrip(int id) {
        for (auto& t : trips)
            if (t.id == id) return &t;
        return nullptr;
    }
    Trip* activeTripOfUser(const std::string& phone) {
        for (auto& t : trips)
            if (t.userPhone == phone && t.active()) return &t;
        return nullptr;
    }

private:
    std::string dir_;

    static void makeDir(const std::string& d) { ::mkdir(d.c_str(), 0755); }
    std::string path(const std::string& f) const { return dir_ + "/" + f; }

    static bool readFile(const std::string& p, std::vector<std::vector<std::string>>& rows) {
        std::ifstream in(p);
        if (!in) return false;
        std::string ln;
        bool first = true;
        while (std::getline(in, ln)) {
            if (first) { first = false; continue; }  // header
            if (util::trim(ln).empty()) continue;
            rows.push_back(util::split(ln, ','));
        }
        return true;
    }

    static std::string field(const std::vector<std::string>& r, size_t i) { return i < r.size() ? r[i] : ""; }
    static int toInt(const std::string& s) { return util::isDigits(s) ? std::stoi(s) : 0; }
    static double toDouble(const std::string& s) {
        try { return s.empty() ? 0.0 : std::stod(s); } catch (...) { return 0.0; }
    }

    // ---------- users ----------
    void loadUsers() {
        users.clear();
        std::vector<std::vector<std::string>> rows;
        if (!readFile(path("users.csv"), rows)) return;
        for (auto& r : rows) {
            User u;
            u.name = field(r, 0); u.phone = field(r, 1); u.passHash = field(r, 2);
            u.lastPickup = field(r, 3); u.lastDestination = field(r, 4);
            users.push_back(u);
        }
    }
    void saveUsers() {
        std::ofstream out(path("users.csv"));
        out << "name,phone,passHash,lastPickup,lastDestination\n";
        for (auto& u : users)
            out << u.name << ',' << u.phone << ',' << u.passHash << ',' << u.lastPickup << ',' << u.lastDestination << '\n';
    }

    // ---------- drivers ----------
    void seedDrivers() {
        auto mk = [&](const char* id, const char* name, const char* phone, const char* cab, const char* veh, const char* loc) {
            Driver d;
            d.id = id; d.name = name; d.phone = phone; d.cab = cab; d.vehicle = veh; d.location = loc;
            d.passHash = util::hashPassword("driver123");
            drivers.push_back(d);
        };
        mk("D101", "Ramesh Kumar", "9000000101", "Mini", "CAB-1001", "Central Station");
        mk("D102", "Suresh Patil", "9000000102", "Mini", "CAB-1002", "City Mall");
        mk("D103", "Anil Sharma", "9000000103", "Sedan", "CAB-2001", "Airport");
        mk("D104", "Imran Khan", "9000000104", "Sedan", "CAB-2002", "Tech Park");
        mk("D105", "Vikram Rao", "9000000105", "SUV", "CAB-3001", "University Campus");
        mk("D106", "Deepak Joshi", "9000000106", "SUV", "CAB-3002", "Old Market");
    }
    void loadDrivers() {
        drivers.clear();
        std::vector<std::vector<std::string>> rows;
        if (!readFile(path("drivers.csv"), rows) || rows.empty()) {
            seedDrivers();
            saveDrivers();
            return;
        }
        for (auto& r : rows) {
            Driver d;
            d.id = field(r, 0); d.name = field(r, 1); d.phone = field(r, 2); d.cab = field(r, 3);
            d.vehicle = field(r, 4); d.location = field(r, 5); d.available = field(r, 6) == "1";
            d.passHash = field(r, 7); d.ratingSum = toInt(field(r, 8)); d.ratingCount = toInt(field(r, 9));
            drivers.push_back(d);
        }
    }
    void saveDrivers() {
        std::ofstream out(path("drivers.csv"));
        out << "id,name,phone,cab,vehicle,location,available,passHash,ratingSum,ratingCount\n";
        for (auto& d : drivers)
            out << d.id << ',' << d.name << ',' << d.phone << ',' << d.cab << ',' << d.vehicle << ',' << d.location << ','
                << (d.available ? 1 : 0) << ',' << d.passHash << ',' << d.ratingSum << ',' << d.ratingCount << '\n';
    }

    // ---------- trips ----------
    void loadTrips() {
        trips.clear();
        nextTripId = 1001;
        std::vector<std::vector<std::string>> rows;
        if (!readFile(path("trips.csv"), rows)) return;
        for (auto& r : rows) {
            Trip t;
            t.id = toInt(field(r, 0)); t.userPhone = field(r, 1); t.userName = field(r, 2); t.cab = field(r, 3);
            t.pickup = field(r, 4); t.drop = field(r, 5); t.km = toDouble(field(r, 6));
            t.estFare = toDouble(field(r, 7)); t.finalFare = toDouble(field(r, 8)); t.status = field(r, 9);
            t.driverId = field(r, 10); t.driverResponse = field(r, 11);
            if (!field(r, 12).empty()) t.rejectedBy = util::split(field(r, 12), ';');
            t.rating = toInt(field(r, 13)); t.payMode = field(r, 14); t.txnId = field(r, 15);
            t.bookedAt = field(r, 16); t.pickupTime = field(r, 17); t.waitMin = toInt(field(r, 18));
            t.night = field(r, 19) == "1"; t.note = field(r, 20);
            trips.push_back(t);
            nextTripId = std::max(nextTripId, t.id + 1);
        }
    }
    void saveTrips() {
        std::ofstream out(path("trips.csv"));
        out << "id,userPhone,userName,cab,pickup,drop,km,estFare,finalFare,status,driverId,driverResponse,"
               "rejectedBy,rating,payMode,txnId,bookedAt,pickupTime,waitMin,night,note\n";
        for (auto& t : trips)
            out << t.id << ',' << t.userPhone << ',' << t.userName << ',' << t.cab << ',' << t.pickup << ',' << t.drop << ','
                << t.km << ',' << t.estFare << ',' << t.finalFare << ',' << t.status << ',' << t.driverId << ','
                << t.driverResponse << ',' << util::join(t.rejectedBy, ';') << ',' << t.rating << ',' << t.payMode << ','
                << t.txnId << ',' << t.bookedAt << ',' << t.pickupTime << ',' << t.waitMin << ',' << (t.night ? 1 : 0) << ','
                << t.note << '\n';
    }
};

#endif
