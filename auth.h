// auth.h - Authentication Module (register / login for users, drivers and admin)
#ifndef AUTH_H
#define AUTH_H

#include "database.h"
#include "utils.h"

class AuthModule {
public:
    static const std::string ADMIN_USER;
    static const std::string ADMIN_PASS;

    static void registerUser(Database& db) {
        util::title("USER REGISTRATION");
        std::string name = util::readNonEmpty("Full name           : ");
        std::string phone;
        while (true) {
            phone = util::readLine("Phone (10 digits)   : ");
            if (!util::isDigits(phone) || phone.size() != 10) {
                std::cout << "  ! Phone number must be exactly 10 digits.\n";
                continue;
            }
            if (db.findUser(phone)) {
                std::cout << "  ! This phone number is already registered. Please login.\n";
                return;
            }
            break;
        }
        std::string pw;
        while (true) {
            pw = util::readLine("Password (min 4)    : ");
            if (pw.size() >= 4 && pw.find(',') == std::string::npos) break;
            std::cout << "  ! Password must be at least 4 characters and must not contain a comma.\n";
        }
        User u;
        u.name = name;
        u.phone = phone;
        u.passHash = util::hashPassword(pw);
        db.users.push_back(u);
        db.saveAll();
        std::cout << "\n  Registration successful! You can now login, " << name << ".\n";
    }

    // returns nullptr on failure
    static User* loginUser(Database& db) {
        util::title("USER LOGIN");
        std::string phone = util::readLine("Phone    : ");
        std::string pw = util::readLine("Password : ");
        User* u = db.findUser(phone);
        if (!u || u->passHash != util::hashPassword(pw)) {
            std::cout << "  ! Invalid phone number or password.\n";
            return nullptr;
        }
        std::cout << "\n  Welcome back, " << u->name << "!\n";
        return u;
    }

    static Driver* loginDriver(Database& db) {
        util::title("DRIVER LOGIN");
        std::string id = util::readLine("Driver ID : ");
        std::string pw = util::readLine("Password  : ");
        Driver* d = db.findDriver(id);
        if (!d || d->passHash != util::hashPassword(pw)) {
            std::cout << "  ! Invalid driver ID or password.\n";
            return nullptr;
        }
        std::cout << "\n  Welcome, " << d->name << " (" << d->cab << ", " << d->vehicle << ")\n";
        return d;
    }

    static bool loginAdmin() {
        util::title("ADMIN LOGIN");
        std::string u = util::readLine("Username : ");
        std::string p = util::readLine("Password : ");
        if (u != ADMIN_USER || p != ADMIN_PASS) {
            std::cout << "  ! Invalid admin credentials.\n";
            return false;
        }
        return true;
    }
};

inline const std::string AuthModule::ADMIN_USER = "admin";
inline const std::string AuthModule::ADMIN_PASS = "admin123";

#endif
