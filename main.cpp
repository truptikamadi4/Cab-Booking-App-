// Cab Booking Application - Sqrock IT Solutions Internship, Project Phase 1, Task 1
// Concepts: OOP (classes, encapsulation), STL (vector, map, string), file handling (CSV), algorithms.

#include "admin.h"
#include "auth.h"
#include "booking.h"
#include "database.h"
#include "driver_module.h"

static void userMenu(Database& db, User& user) {
    while (true) {
        util::title("PASSENGER MENU - " + user.name);
        BookingModule::notifyPending(db, user);
        std::cout << "  1. Book a cab\n  2. View current booking\n  3. Cancel booking\n  4. Trip history\n"
                     "  5. Pay for completed trips & rate driver\n  6. View / print receipt\n  7. Logout\n";
        int c = util::readInt("\n  Choose: ", 1, 7);
        switch (c) {
            case 1: BookingModule::bookCab(db, user); break;
            case 2: BookingModule::viewCurrent(db, user); break;
            case 3: BookingModule::cancelBooking(db, user); break;
            case 4: BookingModule::history(db, user); break;
            case 5: BookingModule::payAndRate(db, user); break;
            case 6: BookingModule::downloadReceipt(db, user); break;
            case 7: db.saveAll(); std::cout << "  You have been logged out. See you soon, " << user.name << "!\n"; return;
        }
    }
}

int main() {
    Database db("data");
    db.load();

    try {
        while (true) {
            util::title("CAB BOOKING APPLICATION");
            std::cout << "  1. Register (passenger)\n  2. Login (passenger)\n  3. Driver login\n  4. Admin login\n  5. Exit\n";
            int c = util::readInt("\n  Choose: ", 1, 5);
            switch (c) {
                case 1: AuthModule::registerUser(db); break;
                case 2:
                    if (User* u = AuthModule::loginUser(db)) userMenu(db, *u);
                    break;
                case 3:
                    if (Driver* d = AuthModule::loginDriver(db)) DriverModule::menu(db, *d);
                    break;
                case 4:
                    if (AuthModule::loginAdmin()) AdminModule::menu(db);
                    break;
                case 5:
                    db.saveAll();
                    std::cout << "\n  Thank you for using the Cab Booking Application. Goodbye!\n";
                    return 0;
            }
        }
    } catch (const EndOfInput&) {
        db.saveAll();
        std::cout << "\n  (Input closed - data saved. Goodbye!)\n";
    }
    return 0;
}
