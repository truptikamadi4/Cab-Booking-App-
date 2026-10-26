// models.h - plain data classes (OOP entities) used across the application
#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <vector>
#include <cmath>

// A named place with simulated GPS coordinates
struct Location {
    std::string name;
    double lat;
    double lon;
};

// Static catalogue of places + distance calculation (GPS simulation)
class CityMap {
public:
    static const std::vector<Location>& all() {
        static const std::vector<Location> places = {
            {"Central Station", 21.1458, 79.0882}, {"Airport", 21.0922, 79.0472},
            {"City Mall", 21.1389, 79.0630},       {"University Campus", 21.1250, 79.0500},
            {"Tech Park", 21.0950, 79.0780},       {"Old Market", 21.1480, 79.1130},
            {"Hospital Road", 21.1590, 79.0830},   {"Lake View", 21.1580, 79.0480},
            {"Bus Stand", 21.1463, 79.0849},       {"Stadium", 21.1190, 79.0770}};
        return places;
    }

    static int indexOf(const std::string& name) {
        const auto& p = all();
        for (size_t i = 0; i < p.size(); i++)
            if (p[i].name == name) return static_cast<int>(i);
        return -1;
    }

    // Great-circle distance, multiplied by 1.25 to imitate real road distance
    static double roadDistanceKm(const Location& a, const Location& b) {
        const double R = 6371.0, rad = 3.14159265358979 / 180.0;
        double dLat = (b.lat - a.lat) * rad, dLon = (b.lon - a.lon) * rad;
        double h = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(a.lat * rad) * std::cos(b.lat * rad) * std::sin(dLon / 2) * std::sin(dLon / 2);
        double d = 2 * R * std::asin(std::sqrt(h));
        return d * 1.25;
    }

    static double roadDistanceKm(const std::string& a, const std::string& b) {
        int i = indexOf(a), j = indexOf(b);
        if (i < 0 || j < 0) return 0;
        return roadDistanceKm(all()[i], all()[j]);
    }
};

struct CabType {
    std::string name;
    double baseFare;
    double perKm;
    double waitPerMin;
    int seats;
};

inline const std::vector<CabType>& cabTypes() {
    static const std::vector<CabType> t = {{"Mini", 40, 10, 1.5, 4}, {"Sedan", 60, 13, 2.0, 4}, {"SUV", 90, 18, 3.0, 6}};
    return t;
}

inline const CabType* findCab(const std::string& name) {
    for (const auto& c : cabTypes())
        if (c.name == name) return &c;
    return nullptr;
}

struct User {
    std::string name;
    std::string phone;  // unique ID of a user
    std::string passHash;
    std::string lastPickup;
    std::string lastDestination;
};

struct Driver {
    std::string id;
    std::string name;
    std::string phone;
    std::string cab;      // Mini / Sedan / SUV
    std::string vehicle;  // vehicle number
    std::string location; // current simulated location
    bool available = true;
    std::string passHash;
    int ratingSum = 0;
    int ratingCount = 0;

    double avgRating() const { return ratingCount ? static_cast<double>(ratingSum) / ratingCount : 0.0; }
};

// Trip statuses required by the assignment
namespace Status {
const std::string Booked = "Booked";
const std::string Ongoing = "Ongoing";
const std::string Completed = "Completed";
const std::string Cancelled = "Cancelled";
}  // namespace Status

struct Trip {
    int id = 0;
    std::string userPhone;
    std::string userName;
    std::string cab;
    std::string pickup;
    std::string drop;
    double km = 0;
    double estFare = 0;
    double finalFare = 0;
    std::string status = Status::Booked;
    std::string driverId;
    std::string driverResponse = "Pending";  // Pending / Accepted
    std::vector<std::string> rejectedBy;
    int rating = 0;          // 0 = not rated
    std::string payMode;     // empty = unpaid
    std::string txnId;
    std::string bookedAt;
    std::string pickupTime;  // HH:MM requested pickup time
    int waitMin = 0;
    bool night = false;
    std::string note;        // e.g. cancellation reason

    bool paid() const { return !payMode.empty(); }
    bool active() const { return status == Status::Booked || status == Status::Ongoing; }
};

#endif
