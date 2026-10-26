// admin.h - Admin Panel (users, drivers, all trips, revenue report)
#ifndef ADMIN_H
#define ADMIN_H

#include <map>
#include "booking.h"

class AdminModule {
public:
    static void menu(Database& db) {
        while (true) {
            util::title("ADMIN PANEL");
            std::cout << "  1. View all users\n  2. View all drivers\n  3. View all trips\n"
                         "  4. Revenue & statistics report\n  5. Logout\n";
            int c = util::readInt("\n  Choose: ", 1, 5);
            switch (c) {
                case 1: users(db); break;
                case 2: drivers(db); break;
                case 3: trips(db); break;
                case 4: report(db); break;
                case 5: std::cout << "  Admin logged out.\n"; return;
            }
        }
    }

private:
    static void users(Database& db) {
        util::title("REGISTERED USERS");
        if (db.users.empty()) { std::cout << "  No users registered yet.\n"; return; }
        std::cout << "  " << std::left << std::setw(20) << "Name" << std::setw(14) << "Phone" << std::setw(20) << "Last pickup"
                  << "Last destination\n";
        util::line('-', 74);
        for (auto& u : db.users)
            std::cout << "  " << std::setw(20) << u.name << std::setw(14) << u.phone << std::setw(20)
                      << (u.lastPickup.empty() ? "-" : u.lastPickup) << (u.lastDestination.empty() ? "-" : u.lastDestination) << "\n";
        std::cout << std::right;
    }

    static void drivers(Database& db) {
        util::title("DRIVERS");
        std::cout << "  " << std::left << std::setw(6) << "ID" << std::setw(16) << "Name" << std::setw(8) << "Cab" << std::setw(10)
                  << "Vehicle" << std::setw(20) << "Location" << std::setw(11) << "Status" << "Rating\n";
        util::line('-', 78);
        for (auto& d : db.drivers)
            std::cout << "  " << std::setw(6) << d.id << std::setw(16) << d.name << std::setw(8) << d.cab << std::setw(10) << d.vehicle
                      << std::setw(20) << d.location << std::setw(11) << (d.available ? "Available" : "Busy/Off")
                      << (d.ratingCount ? util::fixed1(d.avgRating()) : "-") << "\n";
        std::cout << std::right;
    }

    static void trips(Database& db) {
        util::title("ALL TRIPS");
        if (db.trips.empty()) { std::cout << "  No trips yet.\n"; return; }
        std::cout << "  " << std::left << std::setw(6) << "ID" << std::setw(14) << "Phone" << std::setw(36) << "Route" << std::setw(7)
                  << "Cab" << std::setw(8) << "Driver" << std::setw(12) << "Fare" << "Status\n";
        util::line('-', 92);
        for (auto& t : db.trips) {
            double fare = t.status == Status::Completed ? t.finalFare : t.estFare;
            std::cout << "  " << std::setw(6) << t.id << std::setw(14) << t.userPhone << std::setw(36) << (t.pickup + " -> " + t.drop)
                      << std::setw(7) << t.cab << std::setw(8) << (t.driverId.empty() ? "-" : t.driverId) << std::setw(12)
                      << util::money(fare) << t.status << "\n";
        }
        std::cout << std::right;
    }

    static void report(Database& db) {
        util::title("REVENUE & STATISTICS");
        int counts[4] = {0, 0, 0, 0};  // booked, ongoing, completed, cancelled
        double revenue = 0, collected = 0;
        std::map<std::string, std::pair<int, double>> byCab;
        for (auto& t : db.trips) {
            if (t.status == Status::Booked) counts[0]++;
            else if (t.status == Status::Ongoing) counts[1]++;
            else if (t.status == Status::Completed) {
                counts[2]++;
                revenue += t.finalFare;
                if (t.paid()) collected += t.finalFare;
                byCab[t.cab].first++;
                byCab[t.cab].second += t.finalFare;
            } else counts[3]++;
        }
        std::ostringstream os;
        os << "  Registered users : " << db.users.size() << "\n";
        os << "  Drivers          : " << db.drivers.size() << "\n";
        os << "  Total bookings   : " << db.trips.size() << "\n";
        os << "    Booked    : " << counts[0] << "\n    Ongoing   : " << counts[1] << "\n    Completed : " << counts[2]
           << "\n    Cancelled : " << counts[3] << "\n";
        os << "  Fare value of completed trips : " << util::money(revenue) << "\n";
        os << "  Payments collected            : " << util::money(collected) << "\n";
        os << "  Revenue by cab type:\n";
        if (byCab.empty()) os << "    (no completed trips)\n";
        for (auto& kv : byCab) os << "    " << std::left << std::setw(7) << kv.first << kv.second.first << " trip(s), " << util::money(kv.second.second) << "\n";
        std::cout << os.str();
        std::ofstream("report.txt") << "CAB BOOKING - ADMIN REPORT (" << util::nowDateTime() << ")\n" << os.str();
        std::cout << "  (Report saved to report.txt)\n";
    }
};

#endif
