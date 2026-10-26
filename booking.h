// booking.h - Booking Module, Driver Allocation, Trip History, Payment & Receipts
#ifndef BOOKING_H
#define BOOKING_H

#include <fstream>
#include <cstdlib>
#include <limits>

#include "database.h"
#include "fare.h"
#include "utils.h"

// ---------------------------------------------------------------------------
// Driver allocation logic: nearest available driver of the requested cab type
// ---------------------------------------------------------------------------
class Dispatcher {
public:
    // Returns true if a driver was assigned to the trip
    static bool allocate(Database& db, Trip& trip) {
        int pi = CityMap::indexOf(trip.pickup);
        if (pi < 0) return false;
        const Location& pickup = CityMap::all()[pi];

        Driver* best = nullptr;
        double bestDist = std::numeric_limits<double>::max();
        for (auto& d : db.drivers) {
            if (!d.available || d.cab != trip.cab) continue;
            if (std::find(trip.rejectedBy.begin(), trip.rejectedBy.end(), d.id) != trip.rejectedBy.end()) continue;
            int di = CityMap::indexOf(d.location);
            if (di < 0) continue;
            double dist = CityMap::roadDistanceKm(CityMap::all()[di], pickup);
            if (dist < bestDist) { bestDist = dist; best = &d; }
        }
        if (!best) return false;
        best->available = false;
        trip.driverId = best->id;
        trip.driverResponse = "Pending";
        return true;
    }

    static int countAvailable(Database& db, const std::string& cab) {
        int n = 0;
        for (auto& d : db.drivers)
            if (d.available && d.cab == cab) n++;
        return n;
    }

    // minutes for the driver to reach pickup, assuming 30 km/h city speed
    static int etaMinutes(Database& db, const Trip& t) {
        Driver* d = db.findDriver(t.driverId);
        if (!d) return 0;
        double km = CityMap::roadDistanceKm(d->location, t.pickup);
        return std::max(1, static_cast<int>(km / 30.0 * 60.0 + 0.5));
    }
};

// ---------------------------------------------------------------------------
// Printing helpers
// ---------------------------------------------------------------------------
class TripPrinter {
public:
    static void details(Database& db, const Trip& t) {
        util::line();
        std::cout << " Trip ID      : " << t.id << "\n";
        std::cout << " Status       : " << t.status << "\n";
        std::cout << " Passenger    : " << t.userName << " (" << t.userPhone << ")\n";
        std::cout << " Route        : " << t.pickup << "  ->  " << t.drop << "  (" << util::fixed1(t.km) << " km)\n";
        std::cout << " Cab type     : " << t.cab << "\n";
        std::cout << " Pickup time  : " << t.pickupTime << (t.night ? "  (night charges apply)" : "") << "\n";
        if (!t.driverId.empty()) {
            Driver* d = db.findDriver(t.driverId);
            if (d)
                std::cout << " Driver       : " << d->name << " | " << d->phone << " | " << d->vehicle << " | Rating "
                          << (d->ratingCount ? util::fixed1(d->avgRating()) : "New") << "\n";
            std::cout << " Driver reply : " << t.driverResponse << "\n";
        }
        if (t.status == Status::Completed) {
            std::cout << " Waiting time : " << t.waitMin << " min\n";
            std::cout << " Final fare   : " << util::money(t.finalFare) << "\n";
            std::cout << " Payment      : " << (t.paid() ? t.payMode + " (" + t.txnId + ")" : "Pending") << "\n";
            if (t.rating) std::cout << " Your rating  : " << t.rating << "/5\n";
        } else {
            std::cout << " Est. fare    : " << util::money(t.estFare) << "\n";
        }
        if (!t.note.empty()) std::cout << " Note         : " << t.note << "\n";
        std::cout << " Booked at    : " << t.bookedAt << "\n";
        util::line();
    }

    static void fareBreakdown(const FareBreakdown& f) {
        std::cout << "   Base fare        : " << util::money(f.base) << "\n";
        std::cout << "   Distance charge  : " << util::money(f.distanceCharge) << "\n";
        if (f.nightCharge > 0) std::cout << "   Night charge(25%): " << util::money(f.nightCharge) << "\n";
        if (f.waitCharge > 0) std::cout << "   Waiting charge   : " << util::money(f.waitCharge) << "\n";
        std::cout << "   TOTAL            : " << util::money(f.total) << "\n";
    }
};

// ---------------------------------------------------------------------------
// Receipt generation (printed and saved to receipts/RCPT_<tripId>.txt)
// ---------------------------------------------------------------------------
class ReceiptModule {
public:
    static std::string build(Database& db, const Trip& t) {
        const CabType* cab = findCab(t.cab);
        FareBreakdown f = FareCalculator::compute(*cab, t.km, t.night, t.waitMin);
        Driver* d = db.findDriver(t.driverId);
        std::ostringstream os;
        os << "==========================================\n";
        os << "          CAB BOOKING - TRIP RECEIPT\n";
        os << "==========================================\n";
        os << "Receipt for Trip #" << t.id << "\n";
        os << "Date        : " << t.bookedAt << "\n";
        os << "Passenger   : " << t.userName << " (" << t.userPhone << ")\n";
        if (d) os << "Driver      : " << d->name << " (" << d->vehicle << ")\n";
        os << "Cab type    : " << t.cab << "\n";
        os << "Route       : " << t.pickup << " -> " << t.drop << "\n";
        os << "Distance    : " << util::fixed1(t.km) << " km\n";
        os << "------------------------------------------\n";
        os << "Base fare        : " << util::money(f.base) << "\n";
        os << "Distance charge  : " << util::money(f.distanceCharge) << "\n";
        if (f.nightCharge > 0) os << "Night charge     : " << util::money(f.nightCharge) << "\n";
        os << "Waiting (" << t.waitMin << " min) : " << util::money(f.waitCharge) << "\n";
        os << "------------------------------------------\n";
        os << "TOTAL PAID       : " << util::money(f.total) << "\n";
        os << "Payment mode     : " << (t.paid() ? t.payMode : "Pending") << "\n";
        if (t.paid()) os << "Transaction ID   : " << t.txnId << "\n";
        os << "==========================================\n";
        os << "       Thank you for riding with us!\n";
        return os.str();
    }

    static void generate(Database& db, const Trip& t, bool print = true) {
        std::string text = build(db, t);
        std::string file = "receipts/RCPT_" + std::to_string(t.id) + ".txt";
        std::ofstream out(file);
        out << text;
        if (print) std::cout << "\n" << text;
        std::cout << "  (Receipt saved to " << file << ")\n";
    }
};

// ---------------------------------------------------------------------------
// Booking module used by the logged-in passenger
// ---------------------------------------------------------------------------
class BookingModule {
public:
    static void bookCab(Database& db, User& user) {
        util::title("BOOK A CAB");
        if (Trip* a = db.activeTripOfUser(user.phone)) {
            std::cout << "  ! You already have an active booking (Trip " << a->id
                      << "). Complete or cancel it before booking another.\n";
            return;
        }

        std::string pickup = chooseLocation("PICKUP location", user.lastPickup, "");
        if (pickup.empty()) return;
        std::string drop = chooseLocation("DROP location", user.lastDestination, pickup);
        if (drop.empty()) return;

        std::string when;
        while (true) {
            when = util::readLine("Pickup time HH:MM (24h, Enter = now): ");
            if (when.empty()) { when = util::nowHM(); break; }
            if (util::validTime(when)) break;
            std::cout << "  ! Use 24-hour format, e.g. 09:30 or 23:15.\n";
        }
        bool night = FareCalculator::isNight(when);
        double km = CityMap::roadDistanceKm(pickup, drop);

        std::cout << "\n  Distance: " << util::fixed1(km) << " km" << (night ? "   |   Night charges (+25%) apply" : "") << "\n\n";
        std::cout << "  #  Cab     Seats  Fare estimate   Drivers free\n";
        util::line('-', 52);
        int i = 1;
        for (const auto& c : cabTypes()) {
            FareBreakdown f = FareCalculator::compute(c, km, night);
            std::cout << "  " << i++ << "  " << std::left << std::setw(8) << c.name << std::setw(7) << c.seats
                      << std::setw(16) << util::money(f.total) << Dispatcher::countAvailable(db, c.name) << "\n";
        }
        std::cout << std::right;
        int choice = util::readInt("\nSelect cab type (1-3, 0 = cancel): ", 0, 3);
        if (choice == 0) { std::cout << "  Booking aborted.\n"; return; }
        const CabType& cab = cabTypes()[choice - 1];

        if (Dispatcher::countAvailable(db, cab.name) == 0) {
            std::cout << "  ! Sorry, no " << cab.name << " cab is available right now. Try another type.\n";
            return;
        }
        FareBreakdown est = FareCalculator::compute(cab, km, night);
        std::cout << "\n  Fare estimate for " << cab.name << ":\n";
        TripPrinter::fareBreakdown(est);
        std::cout << "  (Waiting charge: " << util::money(cab.waitPerMin) << "/min after "
                  << FareCalculator::FREE_WAIT_MINUTES << " free minutes, added at the end of the trip)\n";
        std::string ok = util::toLower(util::readLine("\nConfirm booking? (y/n): "));
        if (ok != "y" && ok != "yes") { std::cout << "  Booking not confirmed.\n"; return; }

        Trip t;
        t.id = db.nextTripId++;
        t.userPhone = user.phone;
        t.userName = user.name;
        t.cab = cab.name;
        t.pickup = pickup;
        t.drop = drop;
        t.km = km;
        t.estFare = est.total;
        t.night = night;
        t.pickupTime = when;
        t.bookedAt = util::nowDateTime();
        if (!Dispatcher::allocate(db, t)) {
            std::cout << "  ! Could not allocate a driver. Please try again.\n";
            db.nextTripId--;
            return;
        }
        user.lastPickup = pickup;
        user.lastDestination = drop;
        db.trips.push_back(t);
        db.saveAll();

        std::cout << "\n  Booking confirmed! Driver allocated - waiting for the driver to accept.\n";
        TripPrinter::details(db, db.trips.back());
        std::cout << "  Driver is about " << Dispatcher::etaMinutes(db, db.trips.back()) << " min away from your pickup.\n";
    }

    static void viewCurrent(Database& db, User& user) {
        util::title("CURRENT BOOKING");
        Trip* t = db.activeTripOfUser(user.phone);
        if (!t) { std::cout << "  You have no active booking.\n"; return; }
        TripPrinter::details(db, *t);
        if (t->status == Status::Booked && t->driverResponse == "Accepted")
            std::cout << "  Your driver has accepted and is on the way.\n";
        else if (t->status == Status::Booked)
            std::cout << "  Waiting for the driver to accept your ride...\n";
    }

    static void cancelBooking(Database& db, User& user) {
        util::title("CANCEL BOOKING");
        Trip* t = db.activeTripOfUser(user.phone);
        if (!t) { std::cout << "  You have no active booking to cancel.\n"; return; }
        if (t->status == Status::Ongoing) {
            std::cout << "  ! Trip " << t->id << " is already ongoing and cannot be cancelled.\n";
            return;
        }
        TripPrinter::details(db, *t);
        std::string ok = util::toLower(util::readLine("Cancel this booking? (y/n): "));
        if (ok != "y" && ok != "yes") { std::cout << "  Your booking is still active.\n"; return; }
        t->status = Status::Cancelled;
        t->note = "Cancelled by passenger";
        if (Driver* d = db.findDriver(t->driverId)) d->available = true;
        db.saveAll();
        std::cout << "  Booking " << t->id << " cancelled. No charges applied.\n";
    }

    static void history(Database& db, User& user) {
        util::title("TRIP HISTORY");
        bool any = false;
        std::cout << "  ID    Date              Route                                 Cab    Fare          Status\n";
        util::line('-', 100);
        for (auto it = db.trips.rbegin(); it != db.trips.rend(); ++it) {
            if (it->userPhone != user.phone) continue;
            any = true;
            double fare = it->status == Status::Completed ? it->finalFare : it->estFare;
            std::string route = it->pickup + " -> " + it->drop;
            std::cout << "  " << std::left << std::setw(6) << it->id << std::setw(18) << it->bookedAt << std::setw(38) << route
                      << std::setw(7) << it->cab << std::setw(14) << util::money(fare) << it->status << "\n";
        }
        std::cout << std::right;
        if (!any) std::cout << "  No trips yet. Book your first ride!\n";
    }

    // Payment simulation + driver rating for completed, unpaid trips
    static void payAndRate(Database& db, User& user) {
        util::title("PAY & RATE");
        std::vector<Trip*> due;
        for (auto& t : db.trips)
            if (t.userPhone == user.phone && t.status == Status::Completed && !t.paid()) due.push_back(&t);
        if (due.empty()) { std::cout << "  No pending payments.\n"; return; }

        for (Trip* t : due) {
            std::cout << "\n  Trip " << t->id << ": " << t->pickup << " -> " << t->drop << "   Amount due: "
                      << util::money(t->finalFare) << "\n";
            std::cout << "  1. Cash   2. UPI (simulated)   3. Card (simulated)   0. Pay later\n";
            int m = util::readInt("  Payment mode: ", 0, 3);
            if (m == 0) continue;
            static const char* modes[] = {"", "Cash", "UPI", "Card"};
            t->payMode = modes[m];
            if (m == 1) {
                t->txnId = "CASH" + std::to_string(t->id);
            } else {
                std::srand(static_cast<unsigned>(std::time(nullptr)) + t->id);
                t->txnId = "TXN" + std::to_string(t->id) + std::to_string(10000 + std::rand() % 90000);
                std::cout << "  Processing payment... done.\n";
            }
            std::cout << "  Payment of " << util::money(t->finalFare) << " received via " << t->payMode << ". Ref: " << t->txnId << "\n";

            int r = util::readInt("  Rate your driver (1-5, 0 = skip): ", 0, 5);
            if (r > 0) {
                t->rating = r;
                if (Driver* d = db.findDriver(t->driverId)) { d->ratingSum += r; d->ratingCount++; }
                std::cout << "  Thanks for rating!\n";
            }
            ReceiptModule::generate(db, *t);
            db.saveAll();
        }
    }

    static void downloadReceipt(Database& db, User& user) {
        util::title("TRIP RECEIPT");
        int id = util::readInt("Enter Trip ID: ", 0, 99999999);
        Trip* t = db.findTrip(id);
        if (!t || t->userPhone != user.phone) { std::cout << "  ! Trip not found in your history.\n"; return; }
        if (t->status != Status::Completed) { std::cout << "  ! Receipts are available for completed trips only.\n"; return; }
        ReceiptModule::generate(db, *t);
    }

    static void notifyPending(Database& db, User& user) {
        int due = 0;
        for (auto& t : db.trips)
            if (t.userPhone == user.phone && t.status == Status::Completed && !t.paid()) due++;
        if (due) std::cout << "  >> You have " << due << " completed trip(s) waiting for payment (menu option 5).\n";
        if (Trip* a = db.activeTripOfUser(user.phone))
            std::cout << "  >> Active trip " << a->id << " is currently: " << a->status << "\n";
    }

private:
    // Pick a place from the catalogue. Returns "" if the user aborts.
    static std::string chooseLocation(const std::string& label, const std::string& lastUsed, const std::string& exclude) {
        const auto& places = CityMap::all();
        std::cout << "\n  Select " << label << ":\n";
        for (size_t i = 0; i < places.size(); i++) {
            if (places[i].name == exclude) continue;
            std::cout << "   " << std::setw(2) << (i + 1) << ". " << places[i].name << "\n";
        }
        bool hasLast = !lastUsed.empty() && lastUsed != exclude && CityMap::indexOf(lastUsed) >= 0;
        if (hasLast) std::cout << "    0. Use last one (" << lastUsed << ")\n";
        else std::cout << "    0. Abort booking\n";
        while (true) {
            int n = util::readInt("  Your choice: ", 0, static_cast<int>(places.size()));
            if (n == 0) return hasLast ? lastUsed : "";
            if (places[n - 1].name == exclude) {
                std::cout << "  ! Pickup and drop cannot be the same place.\n";
                continue;
            }
            return places[n - 1].name;
        }
    }
};

#endif
