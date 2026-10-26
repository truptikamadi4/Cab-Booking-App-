// driver_module.h - Driver Module (accept/reject rides, view details, update trip status)
#ifndef DRIVER_MODULE_H
#define DRIVER_MODULE_H

#include "booking.h"

class DriverModule {
public:
    static void menu(Database& db, Driver& drv) {
        while (true) {
            util::title("DRIVER PANEL - " + drv.name + " (" + drv.id + ")");
            Trip* cur = currentTrip(db, drv);
            std::cout << "  Location: " << drv.location << " | Status: "
                      << (cur ? "On a ride" : (drv.available ? "Online (available)" : "Offline")) << "\n";
            if (cur) std::cout << "  >> Current ride: Trip " << cur->id << " (" << cur->status << ", reply: " << cur->driverResponse << ")\n";
            std::cout << "\n  1. View ride details\n  2. Accept ride\n  3. Reject ride\n  4. Start trip (passenger picked up)\n"
                         "  5. Complete trip\n  6. My completed trips & earnings\n  7. Go online / offline\n  8. Logout\n";
            int c = util::readInt("\n  Choose: ", 1, 8);
            switch (c) {
                case 1: viewRide(db, drv); break;
                case 2: acceptRide(db, drv); break;
                case 3: rejectRide(db, drv); break;
                case 4: startTrip(db, drv); break;
                case 5: completeTrip(db, drv); break;
                case 6: earnings(db, drv); break;
                case 7: toggle(db, drv); break;
                case 8: std::cout << "  Logged out.\n"; return;
            }
        }
    }

private:
    static Trip* currentTrip(Database& db, Driver& drv) {
        for (auto& t : db.trips)
            if (t.driverId == drv.id && t.active()) return &t;
        return nullptr;
    }

    static void viewRide(Database& db, Driver& drv) {
        Trip* t = currentTrip(db, drv);
        if (!t) { std::cout << "  No ride assigned to you right now.\n"; return; }
        TripPrinter::details(db, *t);
    }

    static void acceptRide(Database& db, Driver& drv) {
        Trip* t = currentTrip(db, drv);
        if (!t || t->status != Status::Booked) { std::cout << "  No new ride request to accept.\n"; return; }
        if (t->driverResponse == "Accepted") { std::cout << "  You have already accepted this ride.\n"; return; }
        t->driverResponse = "Accepted";
        db.saveAll();
        std::cout << "  Ride " << t->id << " accepted. Head to " << t->pickup << " to pick up " << t->userName << ".\n";
    }

    static void rejectRide(Database& db, Driver& drv) {
        Trip* t = currentTrip(db, drv);
        if (!t || t->status != Status::Booked) { std::cout << "  No ride request that can be rejected.\n"; return; }
        std::string ok = util::toLower(util::readLine("  Reject trip " + std::to_string(t->id) + "? (y/n): "));
        if (ok != "y" && ok != "yes") return;
        t->rejectedBy.push_back(drv.id);
        drv.available = true;
        t->driverId.clear();
        if (Dispatcher::allocate(db, *t)) {
            Driver* nd = db.findDriver(t->driverId);
            std::cout << "  Ride rejected. It has been re-assigned to " << (nd ? nd->name : "another driver") << ".\n";
        } else {
            t->status = Status::Cancelled;
            t->driverResponse = "Pending";
            t->note = "No driver available after rejection";
            std::cout << "  Ride rejected. No other driver was available, so the booking was cancelled.\n";
        }
        db.saveAll();
    }

    static void startTrip(Database& db, Driver& drv) {
        Trip* t = currentTrip(db, drv);
        if (!t || t->status != Status::Booked) { std::cout << "  No booked ride to start.\n"; return; }
        if (t->driverResponse != "Accepted") { std::cout << "  ! Accept the ride first.\n"; return; }
        t->status = Status::Ongoing;
        drv.location = t->pickup;
        db.saveAll();
        std::cout << "  Trip " << t->id << " is now ONGOING. Drive safely to " << t->drop << ".\n";
    }

    static void completeTrip(Database& db, Driver& drv) {
        Trip* t = currentTrip(db, drv);
        if (!t || t->status != Status::Ongoing) { std::cout << "  No ongoing trip to complete. Start the trip first.\n"; return; }
        t->waitMin = util::readInt("  Total waiting time during the trip (minutes, 0-180): ", 0, 180);
        const CabType* cab = findCab(t->cab);
        FareBreakdown f = FareCalculator::compute(*cab, t->km, t->night, t->waitMin);
        t->finalFare = f.total;
        t->status = Status::Completed;
        drv.location = t->drop;
        drv.available = true;
        db.saveAll();
        std::cout << "\n  Trip " << t->id << " COMPLETED. Final fare:\n";
        TripPrinter::fareBreakdown(f);
        std::cout << "  The passenger can now pay and rate you.\n";
    }

    static void earnings(Database& db, Driver& drv) {
        util::title("MY TRIPS & EARNINGS");
        int n = 0;
        double total = 0;
        for (auto& t : db.trips) {
            if (t.driverId != drv.id || t.status != Status::Completed) continue;
            n++;
            total += t.finalFare;
            std::cout << "  Trip " << t.id << "  " << t.pickup << " -> " << t.drop << "  " << util::money(t.finalFare)
                      << (t.paid() ? "  [paid]" : "  [unpaid]") << (t.rating ? "  rated " + std::to_string(t.rating) + "/5" : "") << "\n";
        }
        if (!n) std::cout << "  No completed trips yet.\n";
        else std::cout << "\n  Completed trips: " << n << "   Total earnings: " << util::money(total) << "\n";
        std::cout << "  Average rating : " << (drv.ratingCount ? util::fixed1(drv.avgRating()) + "/5 (" + std::to_string(drv.ratingCount) + " ratings)" : "no ratings yet") << "\n";
    }

    static void toggle(Database& db, Driver& drv) {
        if (currentTrip(db, drv)) { std::cout << "  ! You cannot change availability during a ride.\n"; return; }
        drv.available = !drv.available;
        db.saveAll();
        std::cout << "  You are now " << (drv.available ? "ONLINE" : "OFFLINE") << ".\n";
    }
};

#endif
